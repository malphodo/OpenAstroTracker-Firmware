#pragma once

#if (DISPLAY_TYPE > 0) && (FOCUS_STEPPER_TYPE != STEPPER_TYPE_NONE)

// HIGHLIGHT states allow you to pick one of the sub functions.
enum FocusMenuItem
{
    HIGHLIGHT_FOCUS_FIRST      = 1,
    HIGHLIGHT_FOCUS_ADJUSTMENT = 1,
    HIGHLIGHT_FOCUS_RATE,
    HIGHLIGHT_FOCUS_EXIT,
    HIGHLIGHT_FOCUS_LAST = HIGHLIGHT_FOCUS_EXIT,

    FOCUS_ADJUSTMENT,
    FOCUS_RATE_ADJUSTMENT,
};

FocusMenuItem focState = HIGHLIGHT_FOCUS_FIRST;
byte rateIndex         = 3;

bool processFocuserKeys()
{
    lcdButton_t key;
    bool waitForRelease    = false;
    bool checkForKeyChange = true;

    lcdButton_t currentButtonState = lcdButtons.currentState();

    if (focState == FOCUS_ADJUSTMENT)
    {
        if (currentButtonState == btnUP)
        {
            if (!mount.isRunningFocus())
            {
                mount.focusContinuousMove(FOCUS_BACKWARD);
            }
        }
        else if (currentButtonState == btnDOWN)
        {
            if (!mount.isRunningFocus())
            {
                mount.focusContinuousMove(FOCUS_FORWARD);
            }
        }
    }

    if (currentButtonState == btnNONE)
    {
        if (mount.isRunningFocus())
        {
            mount.focusStop();
        }
    }

    if (checkForKeyChange && lcdButtons.keyChanged(&key))
    {
        waitForRelease = true;

        switch (focState)
        {
            case HIGHLIGHT_FOCUS_ADJUSTMENT:
                if (key == btnSELECT)
                {
                    focState = FOCUS_ADJUSTMENT;
                }
                else if (key == btnLEFT)
                {
                    focState = static_cast<FocusMenuItem>(adjustWrap(focState, -1, HIGHLIGHT_FOCUS_FIRST, HIGHLIGHT_FOCUS_LAST));
                }
                else if (key == btnRIGHT)
                {
                    focState = static_cast<FocusMenuItem>(adjustWrap(focState, 1, HIGHLIGHT_FOCUS_FIRST, HIGHLIGHT_FOCUS_LAST));
                }
                else if (key == btnUP)
                {
                    focState = static_cast<FocusMenuItem>(adjustWrap(focState, -1, HIGHLIGHT_FOCUS_FIRST, HIGHLIGHT_FOCUS_LAST));
                }
                else if (key == btnDOWN)
                {
                    focState = static_cast<FocusMenuItem>(adjustWrap(focState, 1, HIGHLIGHT_FOCUS_FIRST, HIGHLIGHT_FOCUS_LAST));
                }

                break;

            case HIGHLIGHT_FOCUS_RATE:
                if (key == btnSELECT)
                {
                    focState = FOCUS_RATE_ADJUSTMENT;
                }
                else if (key == btnLEFT)
                {
                    focState = static_cast<FocusMenuItem>(adjustWrap(focState, -1, HIGHLIGHT_FOCUS_FIRST, HIGHLIGHT_FOCUS_LAST));
                }
                else if (key == btnRIGHT)
                {
                    focState = static_cast<FocusMenuItem>(adjustWrap(focState, 1, HIGHLIGHT_FOCUS_FIRST, HIGHLIGHT_FOCUS_LAST));
                }
                else if (key == btnUP)
                {
                    focState = static_cast<FocusMenuItem>(adjustWrap(focState, -1, HIGHLIGHT_FOCUS_FIRST, HIGHLIGHT_FOCUS_LAST));
                }
                else if (key == btnDOWN)
                {
                    focState = static_cast<FocusMenuItem>(adjustWrap(focState, 1, HIGHLIGHT_FOCUS_FIRST, HIGHLIGHT_FOCUS_LAST));
                }

                break;

            case HIGHLIGHT_FOCUS_EXIT:
                if (key == btnSELECT)
                {
    #if USES_ROTARY_ENCODER == 1
                    requestBackToTop = true;
    #else
                    lcdMenu.setNextActive();
    #endif
                    focState         = HIGHLIGHT_FOCUS_ADJUSTMENT;
                }
                else if (key == btnLEFT)
                {
                    focState = static_cast<FocusMenuItem>(adjustWrap(focState, -1, HIGHLIGHT_FOCUS_FIRST, HIGHLIGHT_FOCUS_LAST));
                }
                else if (key == btnRIGHT)
                {
                    focState = static_cast<FocusMenuItem>(adjustWrap(focState, 1, HIGHLIGHT_FOCUS_FIRST, HIGHLIGHT_FOCUS_LAST));
                }
                else if (key == btnUP)
                {
                    focState = static_cast<FocusMenuItem>(adjustWrap(focState, -1, HIGHLIGHT_FOCUS_FIRST, HIGHLIGHT_FOCUS_LAST));
                }
                else if (key == btnDOWN)
                {
                    focState = static_cast<FocusMenuItem>(adjustWrap(focState, 1, HIGHLIGHT_FOCUS_FIRST, HIGHLIGHT_FOCUS_LAST));
                }
                break;

            case FOCUS_ADJUSTMENT:
                {
                    // UP and DOWN are handled above
                    if (key == btnSELECT)
                    {
                        focState = HIGHLIGHT_FOCUS_ADJUSTMENT;
                    }
                }
                break;

            case FOCUS_RATE_ADJUSTMENT:
                {
                    if (key == btnSELECT)
                    {
                        focState = HIGHLIGHT_FOCUS_RATE;
                    }
                    else if (key == btnRIGHT)
                    {
                        rateIndex = adjustClamp(rateIndex, 1, 0, 3);
                        mount.focusSetSpeedByRate(rateIndex + 1);
                    }
                    else if (key == btnLEFT)
                    {
                        rateIndex = adjustClamp(rateIndex, -1, 0, 3);
                        mount.focusSetSpeedByRate(rateIndex + 1);
                    }
                }
                break;
        }
    }
    return waitForRelease;
}

void printFocusSubmenu()
{
    const bool inMoveEdit = (focState == FOCUS_ADJUSTMENT);
    const bool inRateEdit = (focState == FOCUS_RATE_ADJUSTMENT);
    const char motionChar = !mount.isRunningFocus() ? '-' : (mount.getFocusSpeed() < 0 ? '~' : '^');

    char lineBuf[18];

    lcdMenu.setCursor(0, 1);
    snprintf(lineBuf, sizeof(lineBuf), "%cMove [%c]%s", (focState == HIGHLIGHT_FOCUS_ADJUSTMENT || inMoveEdit) ? '>' : ' ', motionChar,
             inMoveEdit ? "*" : "");
    lcdMenu.printMenu(String(lineBuf));

    lcdMenu.setCursor(0, 2);
    snprintf(lineBuf, sizeof(lineBuf), "%cRate:%u%s", (focState == HIGHLIGHT_FOCUS_RATE || inRateEdit) ? '>' : ' ',
             static_cast<unsigned>(rateIndex + 1), inRateEdit ? "*" : "");
    lcdMenu.printMenu(String(lineBuf));

    lcdMenu.setCursor(0, 3);
    lcdMenu.printMenu(" U/DN move");

    lcdMenu.setCursor(0, 4);
    snprintf(lineBuf, sizeof(lineBuf), "%c%s", (focState == HIGHLIGHT_FOCUS_EXIT) ? '>' : ' ', TR_EXIT);
    lcdMenu.printMenu(String(lineBuf));
}

#endif
