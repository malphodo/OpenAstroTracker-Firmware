#pragma once

#include "../Configuration.hpp"
#include "EPROMStore.hpp"

#if DISPLAY_TYPE > 0
    #if USE_GPS == 0

        #if USES_ROTARY_ENCODER == 1
// Rotary encoder model for HA menu:
//   Fields cycle: 0=H, 1=M, 2=Save, 3=Exit
static const int HA_FIELD_SAVE = 2;
static const int HA_FIELD_EXIT = 3;
static bool haEditing          = false;
        #endif

bool processHAKeys()
{
    lcdButton_t key;
    bool waitForRelease = false;

        #if USES_ROTARY_ENCODER == 1
    if (lcdButtons.keyChanged(&key))
    {
        waitForRelease = true;
        if (!haEditing)
        {
            if (key == btnLEFT)
            {
                HAselect = adjustWrap(HAselect, -1, 0, HA_FIELD_EXIT);
            }
            else if (key == btnRIGHT)
            {
                HAselect = adjustWrap(HAselect, 1, 0, HA_FIELD_EXIT);
            }
            else if (key == btnSELECT)
            {
                if (HAselect == HA_FIELD_EXIT)
                {
                    requestBackToTop = true;
                    HAselect         = 0;
                }
                else if (HAselect == HA_FIELD_SAVE)
                {
                    EEPROMStore::storeHATime(mount.HA());
                    lcdMenu.printMenu("Stored.");
                    mount.delay(500);
            #if SUPPORT_GUIDED_STARTUP == 1
                    if (startupState == StartupWaitForHACompletion)
                    {
                        startupState = StartupHAConfirmed;
                        inStartup    = true;
                    }
            #endif
                    mount.startSlewing(TRACKING);
                }
                else
                {
                    haEditing = true;
                }
            }
        }
        else
        {
            if (key == btnLEFT || key == btnRIGHT)
            {
                int delta = (key == btnRIGHT) ? 1 : -1;
                DayTime ha(mount.HA());
                if (HAselect == 0)
                {
                    ha.addHours(delta);
                }
                else if (HAselect == 1)
                {
                    ha.addMinutes(delta);
                }
                mount.setHA(ha);
            }
            else if (key == btnSELECT)
            {
                haEditing = false;
            }
        }
    }
        #else
    if (lcdButtons.currentState() == btnUP)
    {
        DayTime ha(mount.HA());
        if (HAselect == 0)
            ha.addHours(1);
        if (HAselect == 1)
            ha.addMinutes(1);
        mount.setHA(ha);
        mount.delay(200);
    }
    else if (lcdButtons.currentState() == btnDOWN)
    {
        DayTime ha(mount.HA());
        if (HAselect == 0)
            ha.addHours(-1);
        if (HAselect == 1)
            ha.addMinutes(-1);
        mount.setHA(ha);
        mount.delay(200);
    }
    else if (lcdButtons.keyChanged(&key))
    {
        waitForRelease = true;
        switch (key)
        {
            case btnLEFT:
                HAselect = adjustWrap(HAselect, 1, 0, 1);
                break;

            case btnSELECT:
                EEPROMStore::storeHATime(mount.HA());
                lcdMenu.printMenu("Stored.");
                mount.delay(500);

            #if SUPPORT_GUIDED_STARTUP == 1
                if (startupState == StartupWaitForHACompletion)
                {
                    startupState = StartupHAConfirmed;
                    inStartup    = true;
                }
            #endif
                mount.startSlewing(TRACKING);
                break;

            case btnRIGHT:
            #if SUPPORT_GUIDED_STARTUP == 1
                if (startupState != StartupWaitForHACompletion)
            #endif
                {
                    lcdMenu.setNextActive();
                }
                break;

            default:
                break;
        }
    }
        #endif

    return waitForRelease;
}

void printHASubmenu()
{
        #if USES_ROTARY_ENCODER == 1
    if (HAselect == HA_FIELD_SAVE)
    {
        lcdMenu.printMenu(">Save & Track");
        return;
    }
    if (HAselect == HA_FIELD_EXIT)
    {
        lcdMenu.printMenu(">Exit");
        return;
    }
        #endif
    char scratchBuffer[20];
    sprintf(scratchBuffer, " %02dh %02dm", mount.HA().getHours(), mount.HA().getMinutes());
        #if USES_ROTARY_ENCODER == 1
    scratchBuffer[HAselect * 4] = haEditing ? '*' : '>';
        #else
    scratchBuffer[HAselect * 4] = '>';
        #endif
    lcdMenu.printMenu(scratchBuffer);
}

    #endif
#endif
