#pragma once

#if DISPLAY_TYPE > 0
    #include "Utility.hpp"
    #include "EPROMStore.hpp"
    #include "translations.hpp"

enum cfgItem_t
{
    CfgItemJogSens = 0,
    CfgItemLanguage = 1,
    CfgItemLedMode = 2,
    CfgItemGyro = 3,
    CfgItemAutoPa = 4,
    CfgItemAutoHome = 5,
    CfgItemGPS = 6,
    CfgItemStartSong = 7,
    CfgItemExit = 8,
};

enum cfgLedMode_t : uint8_t
{
    CfgLedModeAstro = 0,
    CfgLedModeGold = 1,
    CfgLedModeDay = 2,
};

static cfgItem_t cfgItem  = CfgItemJogSens;
static bool cfgEditing    = false;
static byte cfgScrollTop  = 0;

// Feature toggles - will be loaded from EEPROM after it's initialized
static bool cfgGyroEnabled = false;
static bool cfgAutoPaEnabled = false;
static bool cfgAutoHomeEnabled = false;
static bool cfgGpsEnabled = false;
static bool cfgStartSongEnabled = true;
static uint8_t cfgLedMode = CfgLedModeDay;

static uint8_t toMini12864Mode(uint8_t cfgMode)
{
    switch (cfgMode)
    {
        case CfgLedModeAstro:
            return (uint8_t) LcdMenu::MINI12864_MODE_NIGHT_ASTRO;
        case CfgLedModeGold:
            return (uint8_t) LcdMenu::MINI12864_MODE_COMFORT;
        default:
            return (uint8_t) LcdMenu::MINI12864_MODE_DAY;
    }
}

static uint8_t fromMini12864Mode(uint8_t miniMode)
{
    if (miniMode == (uint8_t) LcdMenu::MINI12864_MODE_NIGHT_ASTRO)
        return CfgLedModeAstro;
    if (miniMode == (uint8_t) LcdMenu::MINI12864_MODE_COMFORT)
        return CfgLedModeGold;
    if (miniMode == (uint8_t) LcdMenu::MINI12864_MODE_DAY)
        return CfgLedModeDay;
    return CfgLedModeDay;
}

// Initialize config settings from EEPROM
static void initializeConfigSettings()
{
    cfgGyroEnabled = EEPROMStore::getGyroEnabled();
    cfgAutoPaEnabled = EEPROMStore::getAutoPaEnabled();
    cfgAutoHomeEnabled = EEPROMStore::getAutoHomeEnabled();
    cfgGpsEnabled = EEPROMStore::getGpsEnabled();
    cfgStartSongEnabled = EEPROMStore::getBootSongEnabled();
    cfgLedMode = fromMini12864Mode(lcdMenu.getMini12864BacklightMode());
}

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

static void resetConfigUiStateOnExit()
{
    cfgEditing = false;
    lcdMenu.setMini12864EditBlinkActive(false);

    // Clear submenu rows so stale trailing characters (like '*') do not bleed
    // into the next page that may print shorter strings.
    lcdMenu.setCursor(0, 1);
    lcdMenu.printMenu("");
    lcdMenu.setCursor(0, 2);
    lcdMenu.printMenu("");
    lcdMenu.setCursor(0, 3);
    lcdMenu.printMenu("");
    lcdMenu.setCursor(0, 4);
    lcdMenu.printMenu("");
}

bool processConfigKeys()
{
    lcdButton_t key;
    bool waitForRelease = false;

    // Guard: reset state when entering CFG for the first time (or re-entering).
    static bool cfgNeedsReset = true;
    static unsigned long lastCfgEventMs = 0;
    static const unsigned long CFG_EVENT_HOLDOFF_MS = 150UL;
    if (cfgNeedsReset)
    {
        cfgItem        = CfgItemJogSens;
        cfgEditing     = false;
        cfgScrollTop   = 0;
        lastCfgEventMs = millis();   // block events for 150 ms after entry
        cfgNeedsReset  = false;
    }

    if (lcdButtons.keyChanged(&key))
    {
        waitForRelease = true;

#if USES_ROTARY_ENCODER == 1
        const unsigned long cfgNow = millis();
        if (!cfgEditing)
        {
            if (key == btnLEFT || key == btnRIGHT)
            {
                if (cfgNow - lastCfgEventMs >= CFG_EVENT_HOLDOFF_MS)
                {
                    if (key == btnLEFT)
                    {
                        cfgItem = static_cast<cfgItem_t>(adjustWrap(static_cast<int>(cfgItem), -1, CfgItemJogSens, CfgItemExit));
                    }
                    else
                    {
                        cfgItem = static_cast<cfgItem_t>(adjustWrap(static_cast<int>(cfgItem), 1, CfgItemJogSens, CfgItemExit));
                    }
                    lastCfgEventMs = cfgNow;
                }
            }
            else if (key == btnSELECT)
            {
                if (cfgItem == CfgItemExit)
                {
                    playExitBeep();
                    resetConfigUiStateOnExit();
                    // Wait for the SELECT button to be physically released before
                    // handing control back to the top-level loop.
                    while (lcdButtons.currentState() == btnSELECT)
                    {
                        mount.loop();
                    }
                    // Drain the SELECT→NONE keyChanged edge so the top-level handler
                    // sees btnNONE (no event) on its first poll, not btnSELECT.
                    {
                        lcdButton_t drainKey;
                        lcdButtons.keyChanged(&drainKey);
                    }
                    // Set topLevelMenuNav directly here, synchronously, so that the
                    // display section in THIS same frame already renders the title
                    // without '*'.  The requestBackToTop handler will fire later in
                    // the same frame and arm the SELECT hold-off guards.
                    topLevelMenuNav = true;
                    requestBackToTop = true;
                    cfgNeedsReset = true;   // reset state for next entry
                }
                else
                {
                    cfgEditing = true;
                    // NOTE: setMini12864EditBlinkActive(true) intentionally omitted.
                    // On this board, MINI12864_V3_RGB_PIN == LCD12864_CS_PIN (P1_21).
                    // Writing WS2812 data to the shared pin corrupts the LCD SPI bus.
                    // The asterisk suffix in printConfigSubmenu() signals edit mode instead.
                }
                lastCfgEventMs = cfgNow;
            }
        }
        else
        {
            if (cfgItem == CfgItemJogSens)
            {
                if (key == btnLEFT)
                {
                    rotaryMolSens = static_cast<uint8_t>(adjustClamp(static_cast<int>(rotaryMolSens), -1, JOGSENS_MIN, JOGSENS_MAX));
                }
                else if (key == btnRIGHT)
                {
                    rotaryMolSens = static_cast<uint8_t>(adjustClamp(static_cast<int>(rotaryMolSens), 1, JOGSENS_MIN, JOGSENS_MAX));
                }
                else if (key == btnSELECT)
                {
                    cfgEditing = false;
                    lcdMenu.setMini12864EditBlinkActive(false);
                    EEPROMStore::storeJogSens(rotaryMolSens);
                }
            }
            else if (cfgItem == CfgItemLanguage)
            {
                if (key == btnLEFT)
                {
                    currentLanguage = static_cast<Language>(adjustWrap(static_cast<int>(currentLanguage), -1, 0, LANG_COUNT - 1));
                }
                else if (key == btnRIGHT)
                {
                    currentLanguage = static_cast<Language>(adjustWrap(static_cast<int>(currentLanguage), 1, 0, LANG_COUNT - 1));
                }
                else if (key == btnSELECT)
                {
                    cfgEditing = false;
                    lcdMenu.setMini12864EditBlinkActive(false);
                    EEPROMStore::storeLanguage(currentLanguage);
                }
            }
            else if (cfgItem == CfgItemLedMode)
            {
                if (key == btnLEFT)
                {
                    cfgLedMode = static_cast<uint8_t>(adjustWrap((int) cfgLedMode, -1, CfgLedModeAstro, CfgLedModeDay));
                    lcdMenu.applyMini12864BacklightMode(toMini12864Mode(cfgLedMode));
                }
                else if (key == btnRIGHT)
                {
                    cfgLedMode = static_cast<uint8_t>(adjustWrap((int) cfgLedMode, 1, CfgLedModeAstro, CfgLedModeDay));
                    lcdMenu.applyMini12864BacklightMode(toMini12864Mode(cfgLedMode));
                }
                else if (key == btnSELECT)
                {
                    cfgEditing = false;
                    lcdMenu.setMini12864EditBlinkActive(false);
                    lcdMenu.applyMini12864BacklightMode(toMini12864Mode(cfgLedMode));
                }
            }
            else if (cfgItem == CfgItemGyro)
            {
                if (key == btnLEFT || key == btnRIGHT)
                {
                    cfgGyroEnabled = !cfgGyroEnabled;
                }
                else if (key == btnSELECT)
                {
                    cfgEditing = false;
                    lcdMenu.setMini12864EditBlinkActive(false);
                    EEPROMStore::storeGyroEnabled(cfgGyroEnabled);
                }
            }
            else if (cfgItem == CfgItemAutoPa)
            {
                if (key == btnLEFT || key == btnRIGHT)
                {
                    cfgAutoPaEnabled = !cfgAutoPaEnabled;
                }
                else if (key == btnSELECT)
                {
                    cfgEditing = false;
                    lcdMenu.setMini12864EditBlinkActive(false);
                    EEPROMStore::storeAutoPaEnabled(cfgAutoPaEnabled);
                }
            }
            else if (cfgItem == CfgItemAutoHome)
            {
                if (key == btnLEFT || key == btnRIGHT)
                {
                    cfgAutoHomeEnabled = !cfgAutoHomeEnabled;
                }
                else if (key == btnSELECT)
                {
                    cfgEditing = false;
                    lcdMenu.setMini12864EditBlinkActive(false);
                    EEPROMStore::storeAutoHomeEnabled(cfgAutoHomeEnabled);
                }
            }
            else if (cfgItem == CfgItemGPS)
            {
                if (key == btnLEFT || key == btnRIGHT)
                {
                    cfgGpsEnabled = !cfgGpsEnabled;
                }
                else if (key == btnSELECT)
                {
                    cfgEditing = false;
                    lcdMenu.setMini12864EditBlinkActive(false);
                    EEPROMStore::storeGpsEnabled(cfgGpsEnabled);
                }
            }
            else if (cfgItem == CfgItemStartSong)
            {
                if (key == btnLEFT || key == btnRIGHT)
                {
                    cfgStartSongEnabled = !cfgStartSongEnabled;
                }
                else if (key == btnSELECT)
                {
                    cfgEditing = false;
                    lcdMenu.setMini12864EditBlinkActive(false);
                    EEPROMStore::storeBootSongEnabled(cfgStartSongEnabled);
                }
            }
        }
#else
        if (!cfgEditing)
        {
            if (key == btnUP)
            {
                cfgItem = static_cast<cfgItem_t>(adjustWrap(static_cast<int>(cfgItem), -1, CfgItemJogSens, CfgItemExit));
            }
            else if (key == btnDOWN)
            {
                cfgItem = static_cast<cfgItem_t>(adjustWrap(static_cast<int>(cfgItem), 1, CfgItemJogSens, CfgItemExit));
            }
            else if (key == btnSELECT)
            {
                if (cfgItem == CfgItemExit)
                {
                    playExitBeep();
                    resetConfigUiStateOnExit();
                    cfgNeedsReset = true;
                    lcdMenu.setNextActive();
                    waitForRelease = false;
                }
                else
                {
                    cfgEditing = true;
                    // NOTE: setMini12864EditBlinkActive(true) intentionally omitted.
                    // See USES_ROTARY_ENCODER branch above for explanation.
                }
            }
        }
        else
        {
            if (cfgItem == CfgItemJogSens)
            {
                if (key == btnLEFT)
                {
                    rotaryMolSens = static_cast<uint8_t>(adjustClamp(static_cast<int>(rotaryMolSens), -1, JOGSENS_MIN, JOGSENS_MAX));
                }
                else if (key == btnRIGHT)
                {
                    rotaryMolSens = static_cast<uint8_t>(adjustClamp(static_cast<int>(rotaryMolSens), 1, JOGSENS_MIN, JOGSENS_MAX));
                }
                else if (key == btnSELECT)
                {
                    cfgEditing = false;
                    lcdMenu.setMini12864EditBlinkActive(false);
                    EEPROMStore::storeJogSens(rotaryMolSens);
                }
            }
            else if (cfgItem == CfgItemLanguage)
            {
                if (key == btnLEFT)
                {
                    currentLanguage = static_cast<Language>(adjustWrap(static_cast<int>(currentLanguage), -1, 0, LANG_COUNT - 1));
                }
                else if (key == btnRIGHT)
                {
                    currentLanguage = static_cast<Language>(adjustWrap(static_cast<int>(currentLanguage), 1, 0, LANG_COUNT - 1));
                }
                else if (key == btnSELECT)
                {
                    cfgEditing = false;
                    lcdMenu.setMini12864EditBlinkActive(false);
                    EEPROMStore::storeLanguage(currentLanguage);
                }
            }
            else if (cfgItem == CfgItemLedMode)
            {
                if (key == btnLEFT)
                {
                    cfgLedMode = static_cast<uint8_t>(adjustWrap((int) cfgLedMode, -1, CfgLedModeAstro, CfgLedModeDay));
                    lcdMenu.applyMini12864BacklightMode(toMini12864Mode(cfgLedMode));
                }
                else if (key == btnRIGHT)
                {
                    cfgLedMode = static_cast<uint8_t>(adjustWrap((int) cfgLedMode, 1, CfgLedModeAstro, CfgLedModeDay));
                    lcdMenu.applyMini12864BacklightMode(toMini12864Mode(cfgLedMode));
                }
                else if (key == btnSELECT)
                {
                    cfgEditing = false;
                    lcdMenu.setMini12864EditBlinkActive(false);
                    lcdMenu.applyMini12864BacklightMode(toMini12864Mode(cfgLedMode));
                }
            }
            else if (cfgItem == CfgItemStartSong)
            {
                if (key == btnLEFT || key == btnRIGHT)
                {
                    cfgStartSongEnabled = !cfgStartSongEnabled;
                }
                else if (key == btnSELECT)
                {
                    cfgEditing = false;
                    lcdMenu.setMini12864EditBlinkActive(false);
                    EEPROMStore::storeBootSongEnabled(cfgStartSongEnabled);
                }
            }
        }
#endif
    }

    return waitForRelease;
}

static void formatCfgLine(cfgItem_t item, char *out, size_t outLen)
{
    const bool selected = (item == cfgItem);
    const bool editing  = selected && cfgEditing;
    const char cur      = selected ? '>' : ' ';
    switch (item)
    {
        case CfgItemJogSens:
            snprintf(out, outLen, "%cJog:%u%s", cur, static_cast<unsigned>(rotaryMolSens), editing ? "*" : "");
            break;
        case CfgItemLanguage:
            snprintf(out, outLen, "%cLang:%s%s", cur, LANGUAGE_NAMES[currentLanguage], editing ? "*" : "");
            break;
        case CfgItemLedMode:
            snprintf(out, outLen, "%cLed:%s%s", cur,
                     (cfgLedMode == CfgLedModeAstro) ? "Astro" : ((cfgLedMode == CfgLedModeGold) ? "Gold" : "Day"),
                     editing ? "*" : "");
            break;
        case CfgItemGyro:
            snprintf(out, outLen, "%cGyro:%s%s", cur, cfgGyroEnabled ? TR_ON : TR_OFF, editing ? "*" : "");
            break;
        case CfgItemAutoPa:
            snprintf(out, outLen, "%cAutoPA:%s%s", cur, cfgAutoPaEnabled ? TR_ON : TR_OFF, editing ? "*" : "");
            break;
        case CfgItemAutoHome:
            snprintf(out, outLen, "%cAHome:%s%s", cur, cfgAutoHomeEnabled ? TR_ON : TR_OFF, editing ? "*" : "");
            break;
        case CfgItemGPS:
            snprintf(out, outLen, "%cGPS:%s%s", cur, cfgGpsEnabled ? TR_ON : TR_OFF, editing ? "*" : "");
            break;
        case CfgItemStartSong:
            snprintf(out, outLen, "%cStartSong:%s%s", cur, cfgStartSongEnabled ? TR_ON : TR_OFF, editing ? "*" : "");
            break;
        default:  // CfgItemExit
            snprintf(out, outLen, "%c%s", cur, TR_EXIT);
            break;
    }
}

void printConfigSubmenu()
{
    const byte visibleRows = 4;
    const int  maxItem     = static_cast<int>(CfgItemExit);
    const int  curIdx      = static_cast<int>(cfgItem);

    // Keep selected item inside the visible window.
    if (curIdx < static_cast<int>(cfgScrollTop))
        cfgScrollTop = static_cast<byte>(curIdx);
    else if (curIdx >= static_cast<int>(cfgScrollTop) + visibleRows)
        cfgScrollTop = static_cast<byte>(curIdx - visibleRows + 1);

    for (byte row = 0; row < visibleRows; row++)
    {
        const int itemIdx = static_cast<int>(cfgScrollTop) + row;
        lcdMenu.setCursor(0, static_cast<byte>(1 + row));
        if (itemIdx > maxItem)
        {
            lcdMenu.printMenu("");
        }
        else
        {
            char lineBuf[18];
            formatCfgLine(static_cast<cfgItem_t>(itemIdx), lineBuf, sizeof(lineBuf));
            lcdMenu.printMenu(String(lineBuf));
        }
    }
}

#endif
