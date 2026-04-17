#pragma once

#include "LcdButtons.hpp"
#include "b_setup.hpp"
#include "c65_startup.hpp"
#include "c70_menuRA.hpp"
#include "c71_menuDEC.hpp"
#include "c722_menuPOI.hpp"
#include "c72_menuHA.hpp"
#include "c72_menuHA_GPS.hpp"
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
        static unsigned long lastRotaryPageNavMs = 0;
        valueEditingActive = cfgEditing || raEditing || decEditing || (focState == FOCUS_ADJUSTMENT);
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
            // Page-by-page rotary browsing mode:
            // turning the wheel switches the active menu/page directly.
            const unsigned long nowNav = millis();
            const lcdButton_t rotaryState = lcdButtons.currentState();
            if (!valueEditingActive && (rotaryState == btnLEFT) && ((nowNav - lastRotaryPageNavMs) >= 90))
            {
                lcdMenu.setPrevActive();
                lastRotaryPageNavMs  = nowNav;
                waitForButtonRelease = true;
                pageNavigatedByRotary = true;
                topLevelMenuNav = false;
                // Clear page content rows to avoid stale text while switching pages.
                lcdMenu.setCursor(0, 1);
                lcdMenu.printMenu("");
                lcdMenu.setCursor(0, 2);
                lcdMenu.printMenu("");
                lcdMenu.setCursor(0, 3);
                lcdMenu.printMenu("");
                lcdMenu.setCursor(0, 4);
                lcdMenu.printMenu("");
            }
            else if (!valueEditingActive && (rotaryState == btnRIGHT) && ((nowNav - lastRotaryPageNavMs) >= 90))
            {
                lcdMenu.setNextActive();
                lastRotaryPageNavMs  = nowNav;
                waitForButtonRelease = true;
                pageNavigatedByRotary = true;
                topLevelMenuNav = false;
                // Clear page content rows to avoid stale text while switching pages.
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
            // A sub-menu signaled Exit -> hand control back to the top-level carousel.
            if (requestBackToTop)
            {
                requestBackToTop     = false;
                topLevelMenuNav      = false;
                waitForButtonRelease = true;
            }
    #endif
        }

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
                    pageTitle = "RA";
                }
                else if (activeMenu == DEC_Menu)
                {
                    pageTitle = "DEC";
                }
    #if SUPPORT_POINTS_OF_INTEREST == 1
                else if (activeMenu == POI_Menu)
                {
                    pageTitle = "GO";
                }
    #else
                else if (activeMenu == Home_Menu)
                {
                    pageTitle = "HOME";
                }
    #endif
                else if (activeMenu == HA_Menu)
                {
                    pageTitle = "HA";
                }
    #if SUPPORT_MANUAL_CONTROL == 1
                else if (activeMenu == Control_Menu)
                {
                    pageTitle = "CTRL";
                }
    #endif
    #if SUPPORT_CALIBRATION == 1
                else if (activeMenu == Calibration_Menu)
                {
                    pageTitle = "CAL";
                }
    #endif
    #if (FOCUS_STEPPER_TYPE != STEPPER_TYPE_NONE)
                else if (activeMenu == Focuser_Menu)
                {
                    pageTitle = "FOC";
                }
    #endif
    #if SUPPORT_INFO_DISPLAY == 1
                else if (activeMenu == Status_Menu)
                {
                    pageTitle = "INFO";
                }
    #endif
                else if (activeMenu == Config_Menu)
                {
                    pageTitle = "CFG";
                }
                lcdMenu.setCursor(0, 0);
                lcdMenu.printMenu(String(pageTitle));
                lcdMenu.setCursor(0, 1);

    #if SUPPORT_INFO_DISPLAY == 1
                // INFO may use multiple rows; clear leftovers when showing other pages.
                if (activeMenu != Status_Menu)
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
