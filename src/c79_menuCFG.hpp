#pragma once

#if DISPLAY_TYPE > 0
    #include "Utility.hpp"

enum cfgItem_t
{
    CfgItemMolSens = 0,
    CfgItemExit    = 1,
};

static cfgItem_t cfgItem = CfgItemMolSens;
static bool cfgEditing   = false;

static void playExitBeep()
{
#if defined(LCD12864_BEEPER_PIN) && (LCD12864_BEEPER_PIN != U8X8_PIN_NONE)
    pinMode(LCD12864_BEEPER_PIN, OUTPUT);
    for (int i = 0; i < 60; i++)
    {
        digitalWrite(LCD12864_BEEPER_PIN, HIGH);
        delayMicroseconds(260);
        digitalWrite(LCD12864_BEEPER_PIN, LOW);
        delayMicroseconds(260);
    }
#endif
}

bool processConfigKeys()
{
    lcdButton_t key;
    bool waitForRelease = false;

    if (lcdButtons.keyChanged(&key))
    {
        waitForRelease = true;

#if DISPLAY_TYPE == DISPLAY_TYPE_MINI12864_V2
        if (!cfgEditing)
        {
            if (key == btnLEFT)
            {
                cfgItem = static_cast<cfgItem_t>(adjustWrap(static_cast<int>(cfgItem), -1, CfgItemMolSens, CfgItemExit));
            }
            else if (key == btnRIGHT)
            {
                cfgItem = static_cast<cfgItem_t>(adjustWrap(static_cast<int>(cfgItem), 1, CfgItemMolSens, CfgItemExit));
            }
            else if (key == btnSELECT)
            {
                if (cfgItem == CfgItemExit)
                {
                    playExitBeep();
                    lcdMenu.setNextActive();
                    waitForRelease = false;
                }
                else
                {
                    cfgEditing = true;
                }
            }
        }
        else
        {
            if (cfgItem == CfgItemMolSens)
            {
                if (key == btnLEFT)
                {
                    rotaryMolSens = static_cast<uint8_t>(adjustClamp(static_cast<int>(rotaryMolSens), -1, 1, 8));
                }
                else if (key == btnRIGHT)
                {
                    rotaryMolSens = static_cast<uint8_t>(adjustClamp(static_cast<int>(rotaryMolSens), 1, 1, 8));
                }
                else if (key == btnSELECT)
                {
                    cfgEditing = false;
                }
            }
        }
#else
        if (!cfgEditing)
        {
            if (key == btnUP)
            {
                cfgItem = static_cast<cfgItem_t>(adjustWrap(static_cast<int>(cfgItem), -1, CfgItemMolSens, CfgItemExit));
            }
            else if (key == btnDOWN)
            {
                cfgItem = static_cast<cfgItem_t>(adjustWrap(static_cast<int>(cfgItem), 1, CfgItemMolSens, CfgItemExit));
            }
            else if (key == btnSELECT)
            {
                if (cfgItem == CfgItemExit)
                {
                    playExitBeep();
                    lcdMenu.setNextActive();
                    waitForRelease = false;
                }
                else
                {
                    cfgEditing = true;
                }
            }
        }
        else
        {
            if (cfgItem == CfgItemMolSens)
            {
                if (key == btnLEFT)
                {
                    rotaryMolSens = static_cast<uint8_t>(adjustClamp(static_cast<int>(rotaryMolSens), -1, 1, 8));
                }
                else if (key == btnRIGHT)
                {
                    rotaryMolSens = static_cast<uint8_t>(adjustClamp(static_cast<int>(rotaryMolSens), 1, 1, 8));
                }
                else if (key == btnSELECT)
                {
                    cfgEditing = false;
                }
            }
        }
#endif
    }

    return waitForRelease;
}

void printConfigSubmenu()
{
    char line[20];
    if (cfgItem == CfgItemMolSens)
    {
        snprintf(line, sizeof(line), ">MolSens:%u%s", static_cast<unsigned>(rotaryMolSens), cfgEditing ? "*" : "");
    }
    else
    {
        snprintf(line, sizeof(line), ">Exit");
    }
    lcdMenu.printMenu(String(line));
}

#endif
