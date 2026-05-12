#pragma once

#include "Arduino.h"
#include "LcdButtons.hpp"
#include "LcdMenu.hpp"
#include "b_setup.hpp"
#include "c65_startup.hpp"
#include "c70_menuRA.hpp"
#include "c71_menuDEC.hpp"
#include "c722_menuPOI.hpp"
#if USE_GPS == 0
    #include "c72_menuHA.hpp"
#else
    #include "c72_menuHA_GPS.hpp"
#endif
#include "c75_menuCTRL.hpp"
#include "c76_menuCAL.hpp"
#include "c77_menuFOC.hpp"
#include "c78_menuINFO.hpp"
#include "c79_menuCFG.hpp"

#if SUPPORT_SERIAL_CONTROL == 1
    #include "f_serial.hpp"
#endif

#if DISPLAY_TYPE > 0
    #if LCD_BUTTON_TEST == 1
lcdButton_t lastKey = btnNONE;
    #endif

lcdButton_t lcd_key;
unsigned long lastTrackingStatusPrint = 0;

void loop()
{
    #if LCD_BUTTON_TEST == 1
    int adc_key_in;

    lcdMenu.setCursor(0, 0);
    lcdMenu.printMenu("Key Diagnostic");
    lcd_key      = lcdButtons.currentState();
    int key      = lcdButtons.currentKey();
    bool changed = lcdButtons.keyChanged(&lastKey);

    adc_key_in = lcdButtons.currentAnalogState();

    lcdMenu.setCursor(0, 1);
    char buf[128];
    sprintf(buf, "A:%4d %d ", adc_key_in, key);
    String state = String(buf);
    switch (lcd_key)
    {
        case btnNONE:
            state += "None";
            break;
        case btnSELECT:
            state += "Select";
            break;
        case btnLEFT:
            state += "Left";
            break;
        case btnRIGHT:
            state += "Right";
            break;
        case btnUP:
            state += "Up";
            break;
        case btnDOWN:
            state += "Down";
            break;
        default:
            state += "Invalid";
            break;
    }

    lcdMenu.printMenu(state);
    if (changed)
    {
        Serial.println(lastKey);
    }

    return;

    #endif

    // Give the mount a time slice to do its thing...
    mount.loop();
    lcdMenu.tickMini12864Effects(millis());

    // Update the LCD display
    unsigned long now = millis();
    if (!inSerialControl && okToUpdateMenu && !inStartup && !mount.isSlewingRAorDEC())
    {
    #if USES_ROTARY_ENCODER == 1
        // Page-by-page rotary mode does not use the top menu bar.
    #else
        // Main menu display
        lcdMenu.updateDisplay();
    #endif
    }

    // Tracking marker
    if ((mount.isBootComplete()) && (now - lastTrackingStatusPrint > 200))
    {
    #if USES_ROTARY_ENCODER == 1
        // Keep row 0 free from top-bar artifacts in rotary page mode.
    #else
        lcdMenu.printAt(15, 0, mount.isSlewingTRK() ? '&' : '`');
    #endif
        lastTrackingStatusPrint = now;
    }

    lcdMenu.setCursor(0, 1);

    #if SUPPORT_SERIAL_CONTROL == 1
    // Always poll serial commands so desktop clients can connect
    // even when not in the interactive "serial control" menu mode.
    processSerialData();
    if (inSerialControl)
    {
        if (lcdButtons.keyChanged(&lcd_key))
        {
            if (lcd_key == btnSELECT)
            {
                quitSerialOnNextButtonRelease = true;
            }
            else if ((lcd_key == btnNONE) && quitSerialOnNextButtonRelease)
            {
                MeadeCommandProcessor::instance()->processCommand(":Qq#");
                quitSerialOnNextButtonRelease = false;
            }
        }
        serialLoop();
    }
    else
    #endif
    {
        bool waitForButtonRelease = false;
        bool pageNavigatedByRotary = false;
        bool valueEditingActive = false;
    #if USES_ROTARY_ENCODER == 1
        static bool configTopLogoRendered = false;
        static bool infoTopLogoRendered = false;
        static bool raTopLogoRendered = false;
    #endif

    #if USES_ROTARY_ENCODER == 1
        auto resetRotaryEditModes = []() {
            raEditing  = false;
            decEditing = false;
    #if USE_GPS == 1
            haGpsEditing = false;
    #else
            haEditing = false;
    #endif
            cfgEditing = false;
            lcdMenu.setMini12864EditBlinkActive(false);
    #if (FOCUS_STEPPER_TYPE != STEPPER_TYPE_NONE)
            if (focState == FOCUS_ADJUSTMENT)
            {
                focState = HIGHLIGHT_FOCUS_ADJUSTMENT;
            }
    #endif
        };
    #endif
    #if USES_ROTARY_ENCODER == 1
        valueEditingActive = cfgEditing || raEditing || decEditing;
    #if (FOCUS_STEPPER_TYPE != STEPPER_TYPE_NONE)
        valueEditingActive = valueEditingActive || (focState == FOCUS_ADJUSTMENT);
    #endif
    #if USE_GPS == 1
        valueEditingActive = valueEditingActive || haGpsEditing;
    #else
        valueEditingActive = valueEditingActive || haEditing;
    #endif
    #endif

    // Handle the keys
    #if SUPPORT_GUIDED_STARTUP == 1
        if (inStartup)
        {
            waitForButtonRelease = processStartupKeys();
        }
        else
    #endif
        {
    #if USES_ROTARY_ENCODER == 1
            // Rotary interaction has two explicit modes:
            //  - topLevelMenuNav=true  -> wheel cycles top-level pages, click enters page
            //  - topLevelMenuNav=false -> wheel/click handled by active sub-menu only
            // This removes conflicts where a field-edit click/turn unexpectedly changes page.
            lcdButton_t rotaryEvent;
            // Minimum interval between top-level page-change events.
            // Prevents encoder bounces from firing a forward then a backward event
            // in rapid succession (screen flashes and returns to previous page).
            static unsigned long lastTopNavEventMs = 0;
            static const unsigned long TOP_NAV_HOLDOFF_MS = 150UL;
            // Set to true when a sub-page exits via SELECT.
            // Blocks top-level SELECT entry until the button is physically released,
            // detected by polling currentState() each frame — immune to keyChanged
            // edge-detection races and works regardless of how long the button is held.
            static bool topLevelWaitForSelectRelease = false;
            // Absolute time-based holdoff: prevents re-entering any sub-menu for
            // SUB_EXIT_SELECT_HOLDOFF_MS after any requestBackToTop exit, regardless
            // of button debounce state or keyChanged edge-detection race conditions.
            static unsigned long lastSubMenuExitMs = 0;
            static const unsigned long SUB_EXIT_SELECT_HOLDOFF_MS = 500UL;

            // Poll physical state: if we are waiting for release, check each frame.
            if (topLevelWaitForSelectRelease && lcdButtons.currentState() != btnSELECT)
            {
                // Button has been physically released; arm bounce holdoff from this moment.
                topLevelWaitForSelectRelease = false;
                lastTopNavEventMs = millis();
            }

            // IMPORTANT: only consume keyChanged() at this level while browsing top-level pages.
            // In page mode (topLevelMenuNav=false), sub-menus must receive the event themselves.
            if (topLevelMenuNav && !valueEditingActive && lcdButtons.keyChanged(&rotaryEvent))
            {
                if (rotaryEvent == btnLEFT || rotaryEvent == btnRIGHT)
                {
                    const unsigned long navNow = millis();
                    if (navNow - lastTopNavEventMs >= TOP_NAV_HOLDOFF_MS)
                    {
                        if (rotaryEvent == btnLEFT)
                        {
                            lcdMenu.setPrevActive();
                        }
                        else
                        {
                            lcdMenu.setNextActive();
                        }
                        lastTopNavEventMs = navNow;
                    }
                    waitForButtonRelease = true;
                    pageNavigatedByRotary = true;
                }
                else if (rotaryEvent == btnSELECT)
                {
                    // Only enter the page if:
                    //  1. Not waiting for a post-exit button release
                    //  2. Normal bounce holdoff has expired
                    //  3. At least SUB_EXIT_SELECT_HOLDOFF_MS has passed since last sub-menu exit
                    //     (absolute time guard — immune to all button debounce race conditions)
                    const unsigned long navNow = millis();
                    if (!topLevelWaitForSelectRelease
                        && (navNow - lastTopNavEventMs >= TOP_NAV_HOLDOFF_MS)
                        && (navNow - lastSubMenuExitMs >= SUB_EXIT_SELECT_HOLDOFF_MS))
                    {
                        topLevelMenuNav = false;
                        waitForButtonRelease = true;
                        pageNavigatedByRotary = true;
                        resetRotaryEditModes();
                        lastTopNavEventMs = navNow;
                    }
                }
            }
            // When still in top-level navigation, never dispatch sub-menu key handlers.
            if (topLevelMenuNav)
            {
                pageNavigatedByRotary = true;
            }
    #endif
            if (!pageNavigatedByRotary)
            {
            switch (lcdMenu.getActive())
            {
                case RA_Menu:
                    waitForButtonRelease = processRAKeys();
                    break;
                case DEC_Menu:
                    waitForButtonRelease = processDECKeys();
                    break;
    #if SUPPORT_POINTS_OF_INTEREST == 1
                case POI_Menu:
                    waitForButtonRelease = processPOIKeys();
                    break;
    #else
                case Home_Menu:
                    waitForButtonRelease = processHomeKeys();
                    break;
    #endif

                case HA_Menu:
                    waitForButtonRelease = processHAKeys();
                    break;

    #if SUPPORT_CALIBRATION == 1
                case Calibration_Menu:
                    waitForButtonRelease = processCalibrationKeys();
                    break;
    #endif

    #if FOCUS_STEPPER_TYPE != STEPPER_TYPE_NONE
                case Focuser_Menu:
                    waitForButtonRelease = processFocuserKeys();
                    break;
    #endif

    #if SUPPORT_MANUAL_CONTROL == 1
                case Control_Menu:
                    waitForButtonRelease = processControlKeys();
                    break;
    #endif

    #if SUPPORT_INFO_DISPLAY == 1
                case Status_Menu:
                    waitForButtonRelease = processStatusKeys();
                    break;
    #endif
                case Config_Menu:
                    waitForButtonRelease = processConfigKeys();
                    break;
            }
            }

    #if USES_ROTARY_ENCODER == 1
            // Exit from any sub-menu returns to top-level carousel mode.
            if (requestBackToTop)
            {
                requestBackToTop = false;
                topLevelMenuNav  = true;
                // Flag: refuse SELECT at top level until the button is physically released.
                // This is the correct fix for the '*' persistence bug: a time-based holdoff
                // fails when the user holds the button longer than the holdoff duration.
                topLevelWaitForSelectRelease = (lcdButtons.currentState() == btnSELECT);
                lastTopNavEventMs = millis();
                lastSubMenuExitMs = millis();  // arme le holdoff 500 ms — garde absolue
                resetRotaryEditModes();
                // Force title row repaint (rotary "in page" suffix '*') — printMenu skips when
                // _lastDisplay matches even if pixels were out of sync (e.g. after CFG exit).
                lcdMenu.invalidateCachedDisplayRow(0);
                // Keep top-level row clean while returning from deep pages.
                lcdMenu.setCursor(0, 1);
                lcdMenu.printMenu("");
                lcdMenu.setCursor(0, 2);
                lcdMenu.printMenu("");
                lcdMenu.setCursor(0, 3);
                lcdMenu.printMenu("");
                lcdMenu.setCursor(0, 4);
                lcdMenu.printMenu("");
            }
    #endif
        }

    #if USES_ROTARY_ENCODER == 0
        if (waitForButtonRelease)
        {
            if (lcdButtons.currentState() != btnNONE)
            {
                do
                {
                    lcdButton_t waitKey;
                    if (lcdButtons.keyChanged(&waitKey))
                    {
                        if (waitKey == btnNONE)
                        {
                            break;
                        }
                    }

                    // Make sure tracker can still run while fiddling with menus....
                    mount.loop();
                } while (true);
            }
        }
    #endif

        // Input handled, do output
        lcdMenu.setCursor(0, 1);

    #if SUPPORT_GUIDED_STARTUP == 1
        if (inStartup)
        {
            printStartupMenu();
        }
        else
    #endif
        {
            if (!inSerialControl
                )
            {
    #if USES_ROTARY_ENCODER == 1
                // In rotary page mode, show a page title on line 0.
                const int activeMenu = lcdMenu.getActive();
                const char *pageTitle = "";
                if (activeMenu == RA_Menu)
                {
                    pageTitle = TR_RA;
                }
                else if (activeMenu == DEC_Menu)
                {
                    pageTitle = TR_DEC;
                }
    #if SUPPORT_POINTS_OF_INTEREST == 1
                else if (activeMenu == POI_Menu)
                {
                    pageTitle = TR_GO;
                }
    #else
                else if (activeMenu == Home_Menu)
                {
                    pageTitle = TR_HOME;
                }
    #endif
                else if (activeMenu == HA_Menu)
                {
                    pageTitle = TR_HA;
                }
    #if SUPPORT_MANUAL_CONTROL == 1
                else if (activeMenu == Control_Menu)
                {
                    pageTitle = TR_CTRL;
                }
    #endif
    #if SUPPORT_CALIBRATION == 1
                else if (activeMenu == Calibration_Menu)
                {
                    pageTitle = TR_CAL;
                }
    #endif
    #if (FOCUS_STEPPER_TYPE != STEPPER_TYPE_NONE)
                else if (activeMenu == Focuser_Menu)
                {
                    pageTitle = TR_FOC;
                }
    #endif
    #if SUPPORT_INFO_DISPLAY == 1
                else if (activeMenu == Status_Menu)
                {
                    pageTitle = TR_INFO;
                }
    #endif
                else if (activeMenu == Config_Menu)
                {
                    pageTitle = TR_CONFIG;
                }
                lcdMenu.setCursor(0, 0);
                // Keep the config icon only in top-level navigation. In page mode, reserve
                // the full title width so the trailing '*' always remains visible.
                const char *pageIcon = (topLevelMenuNav && activeMenu == Config_Menu) ? "} " : (topLevelMenuNav && activeMenu == Status_Menu) ? "i " : "";
                lcdMenu.printMenu(String(pageIcon) + pageTitle + (topLevelMenuNav ? "" : "*"));
                lcdMenu.setCursor(0, 1);

    #if SUPPORT_INFO_DISPLAY == 1
                // INFO, CFG, RA, DEC, FOC and GO submenus use multiple rows; clear leftovers only for single-row pages.
                if (activeMenu != Status_Menu && activeMenu != Config_Menu && activeMenu != RA_Menu && activeMenu != DEC_Menu
    #if (FOCUS_STEPPER_TYPE != STEPPER_TYPE_NONE)
                    && activeMenu != Focuser_Menu
    #endif
    #if SUPPORT_POINTS_OF_INTEREST == 1
                    && activeMenu != POI_Menu
    #else
                    && activeMenu != Home_Menu
    #endif
                )
                {
                    lcdMenu.setCursor(0, 2);
                    lcdMenu.printMenu("");
                    lcdMenu.setCursor(0, 3);
                    lcdMenu.printMenu("");
                    lcdMenu.setCursor(0, 4);
                    lcdMenu.printMenu("");
                    lcdMenu.setCursor(0, 1);
                }
    #endif

    #if USES_ROTARY_ENCODER == 1
                if (configTopLogoRendered && !(topLevelMenuNav && activeMenu == Config_Menu))
                {
                    // Leaving Configuration top-level: clear stale logo pixels once.
                    lcdMenu.clearConfigLogoLarge();
                    lcdMenu.setCursor(0, 1);
                    configTopLogoRendered = false;
                }
                if (infoTopLogoRendered && !(topLevelMenuNav && activeMenu == Status_Menu))
                {
                    // Leaving INFO top-level: clear stale logo pixels once.
                    lcdMenu.clearConfigLogoLarge();
                    lcdMenu.setCursor(0, 1);
                    infoTopLogoRendered = false;
                }
                if (raTopLogoRendered && !(topLevelMenuNav && activeMenu == RA_Menu))
                {
                    // Leaving RA top-level: clear stale logo pixels once.
                    lcdMenu.clearConfigLogoLarge();
                    lcdMenu.setCursor(0, 1);
                    raTopLogoRendered = false;
                }

                if (topLevelMenuNav && activeMenu == Config_Menu && !configTopLogoRendered)
                {
                    lcdMenu.drawConfigLogoLarge();
                    configTopLogoRendered = true;
                    infoTopLogoRendered   = false;
                    raTopLogoRendered     = false;
                }
                if (topLevelMenuNav && activeMenu == Status_Menu && !infoTopLogoRendered)
                {
                    lcdMenu.drawInfoLogoLarge();
                    infoTopLogoRendered   = true;
                    configTopLogoRendered = false;
                    raTopLogoRendered     = false;
                }
                if (topLevelMenuNav && activeMenu == RA_Menu && !raTopLogoRendered)
                {
                    lcdMenu.drawRaLogoLarge();
                    raTopLogoRendered     = true;
                    infoTopLogoRendered   = false;
                    configTopLogoRendered = false;
                }
    #endif
    #endif
    #if USES_ROTARY_ENCODER == 1
                // In top-level nav mode the sub-menu renderers must not run:
                // rows 1-4 were cleared on exit and should stay blank until
                // the user enters a page again.
                if (!topLevelMenuNav)
                {
    #endif
                // For some strange reason, a switch statement here causes a crash and reboot....
                if (activeMenu == RA_Menu)
                {
                    printRASubmenu();
                }
                else if (activeMenu == DEC_Menu)
                {
                    printDECSubmenu();
                }
    #if SUPPORT_POINTS_OF_INTEREST == 1
                else if (activeMenu == POI_Menu)
                {
                    printPOISubmenu();
                }
    #else
                else if (activeMenu == Home_Menu)
                {
                    printHomeSubmenu();
                }
    #endif
                else if (activeMenu == HA_Menu)
                {
                    printHASubmenu();
                }
    #if SUPPORT_MANUAL_CONTROL == 1
                else if (activeMenu == Control_Menu)
                {
                    printControlSubmenu();
                }
    #endif
    #if SUPPORT_CALIBRATION == 1
                else if (activeMenu == Calibration_Menu)
                {
                    printCalibrationSubmenu();
                }
    #endif

    #if (FOCUS_STEPPER_TYPE != STEPPER_TYPE_NONE)
                else if (activeMenu == Focuser_Menu)
                {
                    printFocusSubmenu();
                }
    #endif

    #if SUPPORT_INFO_DISPLAY == 1
                else if (activeMenu == Status_Menu)
                {
                    printStatusSubmenu();
                }
    #endif
                else if (activeMenu == Config_Menu)
                {
                    printConfigSubmenu();
                }
    #if USES_ROTARY_ENCODER == 1
                } // !topLevelMenuNav
    #endif
            }
        }
    }
}

#else

// No display present.
void loop()
{
    serialLoop();
}

#endif
