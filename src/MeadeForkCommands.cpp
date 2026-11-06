/**
 * @file MeadeForkCommands.cpp
 * @brief Fork-specific `:X` Meade commands (SKR 1.4 Turbo diagnostics, GPS debug,
 *        Mini12864 LCD / RGB backlight, JogSens).
 *
 * These commands are tried BEFORE the official core parser
 * (src/core/meade). Keeping them out of the core keeps future merges of the
 * official firmware simple.
 *
 * Rule: a command handled here must never shadow an official command.
 * Official `:X` leaves (v1.14.0) are:
 *   Get  :XG  R D DL DLL DLU DP S ST T B A AH AA Z C... MS M... O HR HD HS H L N
 *   Set  :XS  DLl DLu DLL.. DLU.. DP.. HR.. HD.. D.. R.. A.. L.. S.. T.. M.. X.. Y.. B..
 * so every RGB / blink setter lives under the fork-only `:XSU` family, and
 * setters sharing a letter with an official one (`:XSLE`) only match exact,
 * non-numeric payloads.
 */

#include "inc/Globals.hpp"
#include "../Configuration.hpp"
#include "Utility.hpp"
#include "LcdMenu.hpp"
#include "Mount.hpp"
#include "EPROMStore.hpp"
#include "MeadeCommandProcessor.hpp"

#include <string.h>
#include <stdio.h>

#if USE_GPS == 1
String getGPSDebugSnapshot(unsigned long pollMs = 300);     // defined in c72_menuHA_GPS.hpp
String getGPSDebugHexSnapshot(unsigned long pollMs = 300);  // defined in c72_menuHA_GPS.hpp
#endif

namespace
{

bool eq(const char *a, const char *b)
{
    return strcmp(a, b) == 0;
}

bool isDigits(const char *s, size_t count)
{
    for (size_t i = 0; i < count; i++)
    {
        if (s[i] < '0' || s[i] > '9')
        {
            return false;
        }
    }
    return true;
}

// Reads `count` decimal digits starting at s (caller checked they are digits).
int readDigits(const char *s, size_t count)
{
    int v = 0;
    for (size_t i = 0; i < count; i++)
    {
        v = v * 10 + (s[i] - '0');
    }
    return v;
}

uint8_t clampByte(int v)
{
    return static_cast<uint8_t>(constrain(v, 0, 255));
}

// Parses exactly 9 digits "rrrgggbbb" into three bytes.
bool parseRgbTriplets(const char *s, uint8_t &r, uint8_t &g, uint8_t &b)
{
    if (strlen(s) != 9 || !isDigits(s, 9))
    {
        return false;
    }
    r = clampByte(readDigits(s, 3));
    g = clampByte(readDigits(s + 3, 3));
    b = clampByte(readDigits(s + 6, 3));
    return true;
}

#if defined(RA_HOMING_SENSOR_PIN) && defined(DEC_HOMING_SENSOR_PIN)
    #define FORK_HAS_HOMING_PINS 1
    #if !defined(RA_HOMING_SENSOR_ACTIVE_STATE)
        #define RA_HOMING_SENSOR_ACTIVE_STATE LOW
    #endif
    #if !defined(DEC_HOMING_SENSOR_ACTIVE_STATE)
        #define DEC_HOMING_SENSOR_ACTIVE_STATE LOW
    #endif
#else
    #define FORK_HAS_HOMING_PINS 0
#endif

}  // namespace

/////////////////////////////
// GPS fix audible motif (called from onStartGpsAcquisition)
/////////////////////////////
void playGpsFixMotif()
{
#if defined(LCD12864_BEEPER_PIN) && (LCD12864_BEEPER_PIN != U8X8_PIN_NONE)
    // Short two-note motif so the GPS fix is clearly audible.
    pinMode(LCD12864_BEEPER_PIN, OUTPUT);
    struct GpsNote {
        uint16_t halfUs;
        uint16_t ms;
        uint8_t pauseMs;
    };
    const GpsNote gpsFixMelody[] = {
        {637, 140, 20},  // G5
        {478, 240, 0},   // C6
    };
    for (uint8_t n = 0; n < 2; n++)
    {
        const uint16_t cycles = (uint32_t) gpsFixMelody[n].ms * 500UL / gpsFixMelody[n].halfUs;
        for (uint16_t i = 0; i < cycles; i++)
        {
            digitalWrite(LCD12864_BEEPER_PIN, HIGH);
            delayMicroseconds(gpsFixMelody[n].halfUs);
            digitalWrite(LCD12864_BEEPER_PIN, LOW);
            delayMicroseconds(gpsFixMelody[n].halfUs);
        }
        if (gpsFixMelody[n].pauseMs)
            delay(gpsFixMelody[n].pauseMs);
    }
#endif
}

/////////////////////////////
// Fork command dispatcher
/////////////////////////////
bool MeadeCommandProcessor::handleForkCommand(const char *cmd, String &reply)
{
    if (cmd == nullptr)
    {
        return false;
    }

    // Normalise ":X....[#]" into a local buffer holding what follows ":X".
    if (cmd[0] == ':')
    {
        cmd++;
    }
    if (cmd[0] != 'X')
    {
        return false;
    }
    cmd++;

    char c[24];
    size_t n = 0;
    while (cmd[n] != '\0' && cmd[n] != '#' && n + 1 < sizeof(c))
    {
        c[n] = cmd[n];
        n++;
    }
    c[n] = '\0';
    if (n == 0)
    {
        return false;
    }

    LcdMenu *lcd            = _lcdMenu;
    const bool rgbSupported = (lcd != nullptr) && lcd->isMini12864RgbSupported();

    // ---------------------------------------------------------------
    // :XLT#  LCD visibility test pattern (clear -> black box -> invert sweep)
    // ---------------------------------------------------------------
    if (eq(c, "LT"))
    {
        if (lcd != nullptr)
        {
            lcd->runVisibilityTestPattern();
            reply = "LCDTEST_OK#";
        }
        else
        {
            reply = "LCDTEST_NOMENU#";
        }
        return true;
    }

    // ---------------------------------------------------------------
    // Get family :XG...
    // ---------------------------------------------------------------
    if (c[0] == 'G')
    {
        const char *leaf = c + 1;

        if (eq(leaf, "ZZ"))  // :XGZZ#  link check
        {
            reply = "ZOK#";
            return true;
        }
        if (eq(leaf, "I2"))  // :XGI2#
        {
            // Full-bus I2C scan can lock up the LPC176x Wire implementation on some bus faults.
            // Keep this command non-blocking to avoid freezing the firmware.
            reply = "I2C:SCAN_DISABLED#";
            return true;
        }
        if (eq(leaf, "HL") || eq(leaf, "HT") || eq(leaf, "HU"))  // Hall / endstop debug
        {
#if FORK_HAS_HOMING_PINS
            char buf[48];
            if (leaf[1] == 'L')  // :XGHL#  raw levels, one read per pin
            {
                snprintf(buf, sizeof(buf), "%d,%d#", digitalRead(RA_HOMING_SENSOR_PIN), digitalRead(DEC_HOMING_SENSOR_PIN));
            }
            else if (leaf[1] == 'T')  // :XGHT#  logically triggered (matches *_ACTIVE_STATE)
            {
                int ra  = (digitalRead(RA_HOMING_SENSOR_PIN) == RA_HOMING_SENSOR_ACTIVE_STATE) ? 1 : 0;
                int dec = (digitalRead(DEC_HOMING_SENSOR_PIN) == DEC_HOMING_SENSOR_ACTIVE_STATE) ? 1 : 0;
                snprintf(buf, sizeof(buf), "%d,%d#", ra, dec);
            }
            else  // :XGHU#  five quick samples per pin; identical digits = stuck line
            {
                int r[5], d[5];
                for (int i = 0; i < 5; i++)
                {
                    if (i > 0)
                    {
                        delayMicroseconds(80);
                    }
                    r[i] = digitalRead(RA_HOMING_SENSOR_PIN);
                    d[i] = digitalRead(DEC_HOMING_SENSOR_PIN);
                }
                snprintf(buf, sizeof(buf), "%d,%d,%d,%d,%d|%d,%d,%d,%d,%d#", r[0], r[1], r[2], r[3], r[4], d[0], d[1], d[2], d[3], d[4]);
            }
            reply = buf;
#else
            reply = "-1,-1#";
#endif
            return true;
        }
        if (eq(leaf, "LC") && lcd != nullptr)  // :XGLC#  backlight brightness
        {
            reply = String(lcd->getBacklightBrightness()) + "#";
            return true;
        }
        if (eq(leaf, "LE") && lcd != nullptr)  // :XGLE#  backlight enabled
        {
            reply = String(lcd->isBacklightEnabled() ? 1 : 0) + "#";
            return true;
        }
        if (eq(leaf, "LM") || eq(leaf, "UM"))  // :XGLM# / :XGUM#  current Mini12864 preset mode
        {
            reply = rgbSupported ? String((int) lcd->getMini12864BacklightMode()) + "#" : String("0#");
            return true;
        }
        if (leaf[0] == 'U')  // :XGUx#  Mini12864 RGB state
        {
            if (!rgbSupported)
            {
                reply = "0#";
                return true;
            }
            uint8_t r, g, b;
            char buf[24];
            if (eq(leaf, "UE"))
            {
                reply = String(lcd->isMini12864RgbEnabled() ? 1 : 0) + "#";
            }
            else if (eq(leaf, "UC"))
            {
                lcd->getMini12864Rgb(&r, &g, &b);
                snprintf(buf, sizeof(buf), "%u,%u,%u#", (unsigned) r, (unsigned) g, (unsigned) b);
                reply = buf;
            }
            else if (eq(leaf, "UR"))
            {
                lcd->getMini12864Rgb(&r, &g, &b);
                snprintf(
                    buf, sizeof(buf), "%u,%u,%u,%u#", lcd->isMini12864RgbEnabled() ? 1u : 0u, (unsigned) r, (unsigned) g, (unsigned) b);
                reply = buf;
            }
            else if (eq(leaf, "UF"))
            {
                reply = String((int) lcd->getMini12864ButtonBlinkMode()) + "#";
            }
            else if (eq(leaf, "UX"))
            {
                reply = String(lcd->isMini12864EditBlinkActive() ? 1 : 0) + "#";
            }
            else
            {
                reply = "0#";
            }
            return true;
        }
        if (eq(leaf, "P"))  // :XGP#  GPS: bytes,sentences,sats,valid,age_ms,last_byte#
        {
#if USE_GPS == 1
            reply = getGPSDebugSnapshot();
#else
            reply = "0,0,0,0,0,-1#";
#endif
            return true;
        }
        if (eq(leaf, "Q"))  // :XGQ#  GPS: n,HH.HH...# most recent raw bytes
        {
#if USE_GPS == 1
            reply = getGPSDebugHexSnapshot();
#else
            reply = "0,#";
#endif
            return true;
        }
        if (eq(leaf, "J"))  // :XGJ#  JogSens 1..8
        {
            reply = String((int) rotaryMolSens) + "#";
            return true;
        }
        return false;  // Official Get leaf: let the core parser answer.
    }

    // ---------------------------------------------------------------
    // Set family :XS...
    // ---------------------------------------------------------------
    if (c[0] == 'S')
    {
        const char *leaf = c + 1;

        // :XSI0# / :XSI1#  display inversion
        if (eq(leaf, "I0") || eq(leaf, "I1"))
        {
            if (lcd == nullptr)
            {
                reply = "0#";
                return true;
            }
            lcd->setDisplayInverted(leaf[1] == '1');
            reply = "1#";
            return true;
        }
        // :XSW0# / :XSW1#  LCD controller power-save off/on
        if (eq(leaf, "W0") || eq(leaf, "W1"))
        {
            if (lcd == nullptr)
            {
                reply = "0#";
                return true;
            }
            lcd->setDisplayPower(leaf[1] == '1');
            reply = "1#";
            return true;
        }
        // :XSLE0# / :XSLE1#  backlight off/on (exact match: :XSL<number> stays ALT steps/deg)
        if ((eq(leaf, "LE0") || eq(leaf, "LE1")) && lcd != nullptr)
        {
            lcd->setBacklightEnabled(leaf[2] == '1');
            reply = "1#";
            return true;
        }
        // :XSJn#  JogSens, clamped and persisted
        if (leaf[0] == 'J' && leaf[1] != '\0')
        {
            int newSens   = String(leaf + 1).toInt();
            newSens       = constrain(newSens, (int) JOGSENS_MIN, (int) JOGSENS_MAX);
            rotaryMolSens = static_cast<uint8_t>(newSens);
            EEPROMStore::storeJogSens(rotaryMolSens);
            reply = "1#";
            return true;
        }
        // :XSCnnn#  backlight brightness, clamped to the display range
        if (leaf[0] == 'C' && leaf[1] != '\0' && lcd != nullptr)
        {
            int minBrightness, maxBrightness;
            lcd->getBacklightBrightnessRange(&minBrightness, &maxBrightness);
            int newLevel = constrain(static_cast<int>(String(leaf + 1).toInt()), minBrightness, maxBrightness);
            lcd->setBacklightBrightness(newLevel);
            reply = "1#";
            return true;
        }
        // :XSU...  Mini12864 V3 RGB backlight / button LEDs (fork-only family)
        if (leaf[0] == 'U' && leaf[1] != '\0')
        {
            if (!rgbSupported)
            {
                reply = "0#";
                return true;
            }
            const char sub    = leaf[1];
            const char *value = leaf + 2;
            uint8_t r, g, b;
            bool ok = false;

            if (sub == 'E' && (eq(value, "0") || eq(value, "1")))  // :XSUE0# / :XSUE1#  enable/disable
            {
                lcd->setMini12864RgbEnabled(value[0] == '1');
                ok = true;
            }
            else if (sub == 'C')  // :XSUCr,g,b#  set colour (does not change enable)
            {
                String payload = value;
                int c1         = payload.indexOf(',');
                int c2         = payload.indexOf(',', c1 + 1);
                if ((c1 > 0) && (c2 > c1))
                {
                    lcd->setMini12864Rgb(clampByte(payload.substring(0, c1).toInt()),
                                         clampByte(payload.substring(c1 + 1, c2).toInt()),
                                         clampByte(payload.substring(c2 + 1).toInt()));
                    ok = true;
                }
            }
            else if ((sub == 'R' || sub == 'G' || sub == 'B') && value[0] != '\0')  // :XSURnnn# / G / B  one channel
            {
                lcd->getMini12864Rgb(&r, &g, &b);
                const uint8_t v = clampByte(String(value).toInt());
                if (sub == 'R')
                    r = v;
                else if (sub == 'G')
                    g = v;
                else
                    b = v;
                lcd->setMini12864Rgb(r, g, b);
                ok = true;
            }
            else if (sub == 'S' && parseRgbTriplets(value, r, g, b))  // :XSUSrrrgggbbb#  colour + enable (was :XSR)
            {
                // Store the colour BEFORE flipping the enable flag so the hardware write
                // uses the new values (otherwise stale R/G/B can blank the chain).
                lcd->setMini12864Rgb(r, g, b);
                lcd->setMini12864RgbEnabled(r != 0 || g != 0 || b != 0);
                ok = true;
            }
            else if (sub == 'L' && value[0] >= '1' && value[0] <= '9' && parseRgbTriplets(value + 1, r, g, b))
            {
                // :XSULNrrrgggbbb#  single LED N (1-based) (was :XSLN...)
                const uint8_t led = static_cast<uint8_t>(value[0] - '0');
                ok                = (led <= lcd->getMini12864LedCount()) && lcd->setMini12864RgbLed(led, r, g, b);
            }
            else if (sub == 'M' && strlen(value) == 1 && isDigits(value, 1))  // :XSUMn#  preset mode (was :XSM)
            {
                const int mode = value[0] - '0';
                ok             = (mode < (int) LcdMenu::MINI12864_MODE_COUNT) && lcd->applyMini12864BacklightMode((uint8_t) mode);
            }
            else if (sub == 'F' && strlen(value) == 1 && isDigits(value, 1))  // :XSUFn#  button blink mode (was :XSB)
            {
                ok = lcd->setMini12864ButtonBlinkMode((uint8_t) (value[0] - '0'));
            }
            else if (sub == 'X' && (eq(value, "0") || eq(value, "1")))  // :XSUX0# / :XSUX1#  force edit blink (was :XSE)
            {
                lcd->setMini12864EditBlinkActive(value[0] == '1');
                ok = true;
            }
            reply = ok ? "1#" : "0#";
            return true;
        }
        return false;  // Official Set leaf (R, D, L, B, M, ...): let the core parser handle it.
    }

    return false;
}
