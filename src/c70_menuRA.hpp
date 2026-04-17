#pragma once

#if DISPLAY_TYPE > 0
bool showTargetRA = true;

    #if USES_ROTARY_ENCODER == 1
// Rotary encoder model for RA menu:
//   Fields cycle: 0=H, 1=M, 2=S, 3=Show toggle, 4=GoTo, 5=Exit
//   Selecting an editable field enters EDIT mode (rotation changes value, click commits).
static const int RA_FIELD_EXIT = 5;
static bool raEditing          = false;
    #endif

bool processRAKeys()
{
    lcdButton_t key;
    bool waitForRelease = false;

    #if USES_ROTARY_ENCODER == 1
    if (lcdButtons.keyChanged(&key))
    {
        waitForRelease = true;
        if (!raEditing)
        {
            if (key == btnLEFT)
            {
                RAselect = adjustWrap(RAselect, -1, 0, RA_FIELD_EXIT);
            }
            else if (key == btnRIGHT)
            {
                RAselect = adjustWrap(RAselect, 1, 0, RA_FIELD_EXIT);
            }
            else if (key == btnSELECT)
            {
                if (RAselect == RA_FIELD_EXIT)
                {
                    requestBackToTop = true;
                    RAselect         = 0;
                }
                else if (RAselect == 3)
                {
                    raEditing = true;
                }
                else if (RAselect == 4)
                {
                    if (mount.isSlewingRAorDEC())
                    {
                        mount.stopSlewing(ALL_DIRECTIONS);
                        mount.waitUntilStopped(ALL_DIRECTIONS);
                    }
                    mount.startSlewingToTarget();
                }
                else
                {
                    raEditing = true;
                }
            }
        }
        else
        {
            if (key == btnLEFT || key == btnRIGHT)
            {
                int delta = (key == btnRIGHT) ? 1 : -1;
                if (RAselect == 0)
                {
                    mount.targetRA().addHours(delta);
                }
                else if (RAselect == 1)
                {
                    mount.targetRA().addMinutes(delta);
                }
                else if (RAselect == 2)
                {
                    mount.targetRA().addSeconds(delta);
                }
                else if (RAselect == 3)
                {
                    showTargetRA = !showTargetRA;
                }
                if (RAselect != 3)
                {
                    showTargetRA = true;
                }
            }
            else if (key == btnSELECT)
            {
                raEditing = false;
            }
        }
    }
    #else
    if (lcdButtons.currentState() == btnUP)
    {
        if (RAselect == 0)
        {
            mount.targetRA().addHours(1);
            showTargetRA = true;
        }
        if (RAselect == 1)
        {
            mount.targetRA().addMinutes(1);
            showTargetRA = true;
        }
        if (RAselect == 2)
        {
            mount.targetRA().addSeconds(1);
            showTargetRA = true;
        }
        if (RAselect == 3)
        {
            showTargetRA  = !showTargetRA;
            waitForRelease = true;
        }

        mount.delay(200);
    }
    else if (lcdButtons.currentState() == btnDOWN)
    {
        if (RAselect == 0)
        {
            mount.targetRA().addHours(-1);
            showTargetRA = true;
        }
        if (RAselect == 1)
        {
            mount.targetRA().addMinutes(-1);
            showTargetRA = true;
        }
        if (RAselect == 2)
        {
            mount.targetRA().addSeconds(-1);
            showTargetRA = true;
        }
        if (RAselect == 3)
        {
            showTargetRA  = !showTargetRA;
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
                RAselect = adjustWrap(RAselect, 1, 0, 3);
                break;

            case btnSELECT:
                if (mount.isSlewingRAorDEC())
                {
                    mount.stopSlewing(ALL_DIRECTIONS);
                    mount.waitUntilStopped(ALL_DIRECTIONS);
                }
                mount.startSlewingToTarget();
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

void printRASubmenu()
{
    #if USES_ROTARY_ENCODER == 1
    if (RAselect == 4)
    {
        lcdMenu.printMenu(">GoTo target");
        return;
    }
    if (RAselect == RA_FIELD_EXIT)
    {
        lcdMenu.printMenu(">Exit");
        return;
    }
    #endif
    if (mount.isSlewingIdle())
    {
        String ra = mount.RAString(LCDMENU_STRING | (showTargetRA ? TARGET_STRING : CURRENT_STRING), RAselect).substring(0, 12);
    #if USES_ROTARY_ENCODER == 1
        ra += raEditing ? "*" : " ";
    #else
        ra += (RAselect == 3) ? ">" : " ";
    #endif
        ra += showTargetRA ? "Ta" : "Cu";
        lcdMenu.printMenu(ra);
    }
}
#endif
