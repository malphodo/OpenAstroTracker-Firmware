#pragma once

#if DISPLAY_TYPE > 0
bool showTargetDEC = true;

    #if USES_ROTARY_ENCODER == 1
// Rotary encoder model for DEC menu:
//   Fields cycle: 0=Deg, 1=M, 2=S, 3=Show toggle, 4=GoTo, 5=Exit
//   Selecting an editable field enters EDIT mode (rotation changes value, click commits).
static const int DEC_FIELD_EXIT = 5;
static bool decEditing          = false;
    #endif

bool processDECKeys()
{
    lcdButton_t key;
    bool waitForRelease = false;

    #if USES_ROTARY_ENCODER == 1
    if (lcdButtons.keyChanged(&key))
    {
        waitForRelease = true;
        if (!decEditing)
        {
            if (key == btnLEFT)
            {
                DECselect = adjustWrap(DECselect, -1, 0, DEC_FIELD_EXIT);
            }
            else if (key == btnRIGHT)
            {
                DECselect = adjustWrap(DECselect, 1, 0, DEC_FIELD_EXIT);
            }
            else if (key == btnSELECT)
            {
                if (DECselect == DEC_FIELD_EXIT)
                {
                    requestBackToTop = true;
                    DECselect        = 0;
                }
                else if (DECselect == 3)
                {
                    decEditing = true;
                }
                else if (DECselect == 4)
                {
                    if (mount.isSlewingRAorDEC())
                    {
                        mount.stopSlewing(ALL_DIRECTIONS);
                    }
                    else
                    {
                        mount.startSlewingToTarget();
                    }
                }
                else
                {
                    decEditing = true;
                }
            }
        }
        else
        {
            if (key == btnLEFT || key == btnRIGHT)
            {
                int delta = (key == btnRIGHT) ? 1 : -1;
                if (DECselect == 0)
                {
                    mount.targetDEC().addDegrees(delta);
                }
                else if (DECselect == 1)
                {
                    mount.targetDEC().addMinutes(delta);
                }
                else if (DECselect == 2)
                {
                    mount.targetDEC().addSeconds(delta);
                }
                else if (DECselect == 3)
                {
                    showTargetDEC = !showTargetDEC;
                }
                if (DECselect != 3)
                {
                    showTargetDEC = true;
                }
            }
            else if (key == btnSELECT)
            {
                decEditing = false;
            }
        }
    }
    #else
    if (lcdButtons.currentState() == btnUP)
    {
        if (DECselect == 0)
        {
            mount.targetDEC().addDegrees(1);
            showTargetDEC = true;
        }
        if (DECselect == 1)
        {
            mount.targetDEC().addMinutes(1);
            showTargetDEC = true;
        }
        if (DECselect == 2)
        {
            mount.targetDEC().addSeconds(1);
            showTargetDEC = true;
        }
        if (DECselect == 3)
        {
            showTargetDEC  = !showTargetDEC;
            waitForRelease = true;
        }
        mount.delay(200);
    }
    else if (lcdButtons.currentState() == btnDOWN)
    {
        if (DECselect == 0)
        {
            mount.targetDEC().addDegrees(-1);
            showTargetDEC = true;
        }
        if (DECselect == 1)
        {
            mount.targetDEC().addMinutes(-1);
            showTargetDEC = true;
        }
        if (DECselect == 2)
        {
            mount.targetDEC().addSeconds(-1);
            showTargetDEC = true;
        }
        if (DECselect == 3)
        {
            showTargetDEC  = !showTargetDEC;
            waitForRelease = true;
        }
        mount.delay(200);
    }
    else if (lcdButtons.keyChanged(&key))
    {
        waitForRelease = true;
        switch (key)
        {
            case btnLEFT:
                DECselect = adjustWrap(DECselect, 1, 0, 3);
                break;

            case btnSELECT:
                if (mount.isSlewingRAorDEC())
                {
                    mount.stopSlewing(ALL_DIRECTIONS);
                }
                else
                {
                    mount.startSlewingToTarget();
                }
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

void printDECSubmenu()
{
    #if USES_ROTARY_ENCODER == 1
    if (DECselect == 4)
    {
        lcdMenu.printMenu(">GoTo target");
        return;
    }
    if (DECselect == DEC_FIELD_EXIT)
    {
        lcdMenu.printMenu(">Exit");
        return;
    }
    #endif
    if (mount.isSlewingIdle())
    {
        String dec = mount.DECString(LCDMENU_STRING | (showTargetDEC ? TARGET_STRING : CURRENT_STRING), DECselect).substring(0, 13);
    #if USES_ROTARY_ENCODER == 1
        dec += decEditing ? "*" : " ";
    #else
        dec += (DECselect == 3) ? ">" : " ";
    #endif
        dec += showTargetDEC ? "Ta" : "Cu";
        lcdMenu.printMenu(dec);
    }
}

#endif
