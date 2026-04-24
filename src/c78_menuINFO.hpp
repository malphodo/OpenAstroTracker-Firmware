#pragma once

#if DISPLAY_TYPE > 0

    #include "Utility.hpp"
    #include "translations.hpp"

    #if SUPPORT_INFO_DISPLAY == 1

static byte infoIndex               = 0;
static byte infoScrollTop           = 0;
static unsigned long lastInfoUpdate = 0;

static void playExitBeepInfo()
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

static byte getInfoLineCount()
{
    byte count = 21;
#if USE_GYRO_LEVEL == 0
    count--;
#endif
    return count;
}

static void formatInfoLine(byte index, char *out, size_t outLen)
{
    char scratchBuffer[24];
    scratchBuffer[0] = '\0';

    byte normalized = index;
#if USE_GYRO_LEVEL == 0
    if (normalized >= 9)
    {
        normalized++;
    }
#endif

    switch (normalized)
    {
        case 0:
            snprintf(out, outLen, "RA St:%ld", mount.getCurrentStepperPosition(WEST));
            break;
        case 1:
            snprintf(out, outLen, "RA Tg:%s", mount.RAString(LCD_STRING | TARGET_STRING).c_str());
            break;
        case 2:
            snprintf(out, outLen, "RA Cu:%s", mount.RAString(LCD_STRING | CURRENT_STRING).c_str());
            break;
        case 3:
            snprintf(out, outLen, "DE St:%ld", mount.getCurrentStepperPosition(NORTH));
            break;
        case 4:
            snprintf(out, outLen, "DE Tg:%s", mount.DECString(LCD_STRING | TARGET_STRING).c_str());
            break;
        case 5:
            snprintf(out, outLen, "DE Cu:%s", mount.DECString(LCD_STRING | CURRENT_STRING).c_str());
            break;
        case 6:
            snprintf(out, outLen, "TRK St:%ld", mount.getCurrentStepperPosition(TRACKING));
            break;
        case 7:
            snprintf(scratchBuffer, sizeof(scratchBuffer), "TRK:");
            dtostrf(mount.getSpeed(TRACKING), 8, 6, &scratchBuffer[4]);
            snprintf(out, outLen, "%s", scratchBuffer);
            break;
        case 8:
        {
            float lat          = fabsf(mount.latitude().getTotalHours());
            float lng          = fabsf(mount.longitude().getTotalHours());
            const char dirLat  = (mount.latitude().getTotalHours() < 0) ? 'S' : 'N';
            const char dirLong = (mount.longitude().getTotalHours() < 0) ? 'W' : 'E';
            snprintf(out, outLen, "Loc %s%c %s%c", String(lat, 1).c_str(), dirLat, String(lng, 1).c_str(), dirLong);
            break;
        }
#if USE_GYRO_LEVEL == 1
        case 9:
        {
            int celsius = static_cast<int>(roundf(Gyro::getCurrentTemperature()));
            snprintf(out, outLen, "Temp:%d@C", celsius);
            break;
        }
#endif
        case 10:
        {
            float lowerLimit, upperLimit;
            mount.getDecLimitPositions(lowerLimit, upperLimit);
            snprintf(out, outLen, "DEC Lo:%s", String(lowerLimit, 1).c_str());
            break;
        }
        case 11:
        {
            float lowerLimit, upperLimit;
            mount.getDecLimitPositions(lowerLimit, upperLimit);
            snprintf(out, outLen, "DEC Up:%s", String(upperLimit, 1).c_str());
            break;
        }
        case 12:
            snprintf(out, outLen, "Mem:%d", freeMemory());
            break;
        case 13:
        {
            long now      = millis();
            long msPerDay = 60L * 60 * 24 * 1000;
            int days      = (int) (now / msPerDay);
            now -= days * msPerDay;
            DayTime elapsed(1.0 * now / (1000.0 * 3600.0));
            snprintf(out, outLen, "Up:%dd %02d:%02d", days, elapsed.getHours(), elapsed.getMinutes());
            break;
        }
        case 14:
        {
            LocalDate date = mount.getLocalDate();
            snprintf(out, outLen, "Date:%04d-%02d-%02d", date.year, date.month, date.day);
            break;
        }
        case 15:
        {
            DayTime drvUtc = mount.getUtcTime();
            snprintf(out, outLen, "UTC:%02d:%02d:%02d", drvUtc.getHours(), drvUtc.getMinutes(), drvUtc.getSeconds());
            break;
        }
        case 16:
        {
            DayTime local = mount.getLocalTime();
            snprintf(out, outLen, "Time:%02d:%02d:%02d", local.getHours(), local.getMinutes(), local.getSeconds());
            break;
        }
        case 17:
            snprintf(out, outLen, "TZ:%d", mount.getLocalUtcOffset());
            break;
        case 18:
        {
            DayTime lst = mount.calculateLst();
            snprintf(out, outLen, "LST:%02d:%02d:%02d", lst.getHours(), lst.getMinutes(), lst.getSeconds());
            break;
        }
        case 19:
        {
            DayTime ha = mount.calculateHa();
            snprintf(out, outLen, "HA:%02d:%02d:%02d", ha.getHours(), ha.getMinutes(), ha.getSeconds());
            break;
        }
        default:
            snprintf(out, outLen, "%s", TR_EXIT);
            break;
    }
}

bool processStatusKeys()
{
    lcdButton_t key;
    bool waitForRelease = false;

#if USES_ROTARY_ENCODER == 1
    // Guard: reset state when entering the page for the first time (or re-entering).
    static bool infoNeedsReset = true;
    static unsigned long lastInfoEventMs = 0;
    static const unsigned long INFO_EVENT_HOLDOFF_MS = 150UL;
    if (infoNeedsReset)
    {
        infoIndex      = 0;
        infoScrollTop  = 0;
        lastInfoEventMs = millis();   // block events for 150 ms after entry
        infoNeedsReset = false;
    }
#endif

    if (lcdButtons.keyChanged(&key))
    {
        waitForRelease = true;
        lastInfoUpdate = 0;

        const byte maxIndex = static_cast<byte>(getInfoLineCount() - 1);

#if USES_ROTARY_ENCODER == 1
        const unsigned long infoNavNow = millis();
        if (key == btnLEFT || key == btnRIGHT)
        {
            if (infoNavNow - lastInfoEventMs >= INFO_EVENT_HOLDOFF_MS)
            {
                if (key == btnLEFT)
                {
                    infoIndex = adjustWrap(infoIndex, -1, 0, maxIndex);
                }
                else
                {
                    infoIndex = adjustWrap(infoIndex, 1, 0, maxIndex);
                }
                lastInfoEventMs = infoNavNow;
            }
        }
        else if (key == btnSELECT)
        {
            if (infoIndex == maxIndex && (infoNavNow - lastInfoEventMs >= INFO_EVENT_HOLDOFF_MS))
            {
                playExitBeepInfo();
                requestBackToTop = true;
                infoNeedsReset = true;   // reset state for next entry
            }
            lastInfoEventMs = infoNavNow;
        }

        const byte visibleRows = 4;
        if (infoIndex < infoScrollTop)
        {
            infoScrollTop = infoIndex;
        }
        else if (infoIndex >= static_cast<byte>(infoScrollTop + visibleRows))
        {
            infoScrollTop = static_cast<byte>(infoIndex - visibleRows + 1);
        }
#else
        if (key == btnDOWN)
        {
            infoIndex = adjustWrap(infoIndex, 1, 0, maxIndex);
        }
        else if (key == btnUP)
        {
            infoIndex = adjustWrap(infoIndex, -1, 0, maxIndex);
        }
        else if (key == btnSELECT)
        {
            if (infoIndex == maxIndex)
            {
                playExitBeepInfo();
                lcdMenu.setNextActive();
                waitForRelease = false;
            }
        }
#endif
    }

    return waitForRelease;
}

void printStatusSubmenu()
{
    if (millis() - lastInfoUpdate < DISPLAY_UPDATE_TIME)
    {
        return;
    }

#if DISPLAY_TYPE == DISPLAY_TYPE_MINI12864_V2
    const byte visibleRows = 4;
    for (byte row = 0; row < visibleRows; row++)
    {
        const byte lineIndex = static_cast<byte>(infoScrollTop + row);
        char payload[24];
        if (lineIndex < getInfoLineCount())
        {
            formatInfoLine(lineIndex, payload, sizeof(payload));
        }
        else
        {
            payload[0] = '\0';
        }

        String line = String((lineIndex == infoIndex) ? ">" : " ") + String(payload);
        if (line.length() > 16)
        {
            line = line.substring(0, 16);
        }
        lcdMenu.setCursor(0, static_cast<byte>(1 + row));
        lcdMenu.printMenu(line);
    }
#else
    char payload[24];
    formatInfoLine(infoIndex, payload, sizeof(payload));
    lcdMenu.printMenu(String(payload));
#endif

    lastInfoUpdate = millis();
}

    #endif

#endif
