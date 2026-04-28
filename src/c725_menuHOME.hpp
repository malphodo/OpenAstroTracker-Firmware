#pragma once

#include "b_setup.hpp"

#if DISPLAY_TYPE > 0
byte subGoIndex = 0;

    #if USES_ROTARY_ENCODER == 1
// Rotary encoder model: 0=Home, 1=Park/Unpark, 2=Exit
static const int HOME_EXIT_INDEX = 2;
    #endif

bool processHomeKeys()
{
    lcdButton_t key;
    bool waitForRelease = false;

    #if USES_ROTARY_ENCODER == 1
    if (lcdButtons.keyChanged(&key))
    {
        waitForRelease = true;
        if (key == btnLEFT)
        {
            subGoIndex = static_cast<byte>(adjustWrap(subGoIndex, -1, 0, HOME_EXIT_INDEX));
        }
        else if (key == btnRIGHT)
        {
            subGoIndex = static_cast<byte>(adjustWrap(subGoIndex, 1, 0, HOME_EXIT_INDEX));
        }
        else if (key == btnSELECT)
        {
            if (subGoIndex == HOME_EXIT_INDEX)
            {
                requestBackToTop = true;
                subGoIndex       = 0;
            }
            else if (subGoIndex == 0)
            {
                mount.startSlewingToHome();
            }
            else if (mount.isSlewingTRK())
            {
                mount.park();
            }
            else
            {
                mount.startSlewing(TRACKING);
            }
        }
    }
    #else
    if (lcdButtons.keyChanged(&key))
    {
        waitForRelease = true;
        switch (key)
        {
            case btnSELECT:
                {
                    if (subGoIndex == 0)
                    {
                        mount.startSlewingToHome();
                    }
                    else if (mount.isSlewingTRK())
                    {
                        mount.park();
                    }
                    else
                    {
                        mount.startSlewing(TRACKING);
                    }
                }
                break;

            case btnUP:
            case btnDOWN:
            case btnLEFT:
                subGoIndex = 1 - subGoIndex;
                break;

            case btnRIGHT:
                lcdMenu.setNextActive();
                break;

            default:
                break;
        }
    }
    #endif

    return waitForRelease;
}

void printHomeSubmenu()
{
    #if USES_ROTARY_ENCODER == 1
    lcdMenu.setCursor(0, 1);
    lcdMenu.printMenu(String((subGoIndex == 0) ? ">" : " ") + String(TR_HOME));

    lcdMenu.setCursor(0, 2);
    lcdMenu.printMenu(String((subGoIndex == 1) ? ">" : " ") + String(mount.isSlewingTRK() ? TR_PARK : TR_UNPARK));

    lcdMenu.setCursor(0, 3);
    lcdMenu.printMenu(String((subGoIndex == HOME_EXIT_INDEX) ? ">" : " ") + String(TR_EXIT));

    lcdMenu.setCursor(0, 4);
    lcdMenu.printMenu("");
    #else
    char scratchBuffer[16];
    if (mount.isSlewingTRK())
    {
        strcpy(scratchBuffer, " Home  Park");
    }
    else
    {
        strcpy(scratchBuffer, " Home  Unpark");
    }
    scratchBuffer[subGoIndex * 6] = '>';
    lcdMenu.printMenu(scratchBuffer);
    #endif
}

#endif
