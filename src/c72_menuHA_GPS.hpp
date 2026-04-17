#pragma once

#include "../Configuration.hpp"
#include "EPROMStore.hpp"
#if USE_GPS_I2C_ADAFRUIT == 1
PUSH_NO_WARNINGS
    #include <Wire.h>
POP_NO_WARNINGS
#endif

#if USE_GPS == 1

static unsigned long gpsDiagBytesRead      = 0;
static unsigned long gpsDiagSentences      = 0;
static unsigned long gpsDiagLastSentenceMs = 0;
static int gpsDiagLastByte                 = -1;
static constexpr int GPS_DIAG_HEX_WINDOW   = 24;
static uint8_t gpsDiagHexWindow[GPS_DIAG_HEX_WINDOW];
static int gpsDiagHexWriteIndex = 0;
static int gpsDiagHexCount      = 0;

    #if DEBUG_LEVEL & DEBUG_GPS
char gpsBuf[256];
int gpsBufPos = 0;
    #endif

long lastGPSUpdate = 0;
bool gpsAqcuisitionComplete(int &indicator)
{
#if USE_GPS_I2C_ADAFRUIT == 1
    for (int chunk = 0; chunk < 4; chunk++)
    {
        const uint8_t toRead = 32;
        const uint8_t got    = Wire.requestFrom(static_cast<uint8_t>(GPS_I2C_ADDRESS), toRead);
        if (got == 0)
        {
            break;
        }

        while (Wire.available())
        {
            const int gpsChar = Wire.read();
            gpsDiagBytesRead++;
            gpsDiagLastByte = gpsChar;
            gpsDiagHexWindow[gpsDiagHexWriteIndex] = static_cast<uint8_t>(gpsChar & 0xFF);
            gpsDiagHexWriteIndex                    = (gpsDiagHexWriteIndex + 1) % GPS_DIAG_HEX_WINDOW;
            if (gpsDiagHexCount < GPS_DIAG_HEX_WINDOW)
            {
                gpsDiagHexCount++;
            }

            if (gpsChar == 36)
            {
                if (millis() - lastGPSUpdate > 500)
                {
                    indicator     = adjustWrap(indicator, 1, 0, 3);
                    lastGPSUpdate = millis();
                }
            }

            if (gps.encode(gpsChar))
            {
                gpsDiagSentences++;
                gpsDiagLastSentenceMs = millis();
                LOG(DEBUG_GPS,
                    "[GPS]: Encoded. %l sats, Location is%svalid, age is %lms",
                    gps.satellites.value(),
                    (gps.location.isValid() ? " " : " NOT "),
                    gps.location.age());
                if ((gps.location.lng() != 0) && (gps.location.age() < 30000UL))
                {
                    LOG(DEBUG_INFO, "[GPS]: Sync'd GPS location. Age is %d secs", gps.location.age() / 1000);
                    LOG(DEBUG_INFO, "[GPS]: Location: %f  %f", gps.location.lat(), gps.location.lng());
                    LOG(DEBUG_INFO, "[GPS]: UTC time is %dh%dm%ds", gps.time.hour(), gps.time.minute(), gps.time.second());
                    lcdMenu.printMenu("GPS sync'd....");

                    DayTime utcNow = DayTime(gps.time.hour(), gps.time.minute(), gps.time.second());
                    utcNow.addHours(mount.getLocalUtcOffset());
                    mount.setLocalStartTime(utcNow);
                    mount.setLocalStartDate(gps.date.year(), gps.date.month(), gps.date.day());
                    mount.setLatitude(gps.location.lat());
                    mount.setLongitude(gps.location.lng());
                    mount.delay(500);
                    return true;
                }
            }
        }
    }
    return false;
#else
    while (GPS_SERIAL_PORT.available())
    {
        int gpsChar = GPS_SERIAL_PORT.read();
        gpsDiagBytesRead++;
        gpsDiagLastByte = gpsChar;
        gpsDiagHexWindow[gpsDiagHexWriteIndex] = static_cast<uint8_t>(gpsChar & 0xFF);
        gpsDiagHexWriteIndex                    = (gpsDiagHexWriteIndex + 1) % GPS_DIAG_HEX_WINDOW;
        if (gpsDiagHexCount < GPS_DIAG_HEX_WINDOW)
        {
            gpsDiagHexCount++;
        }

    #if DEBUG_LEVEL & DEBUG_GPS
        if ((gpsBufPos < 254) && (gpsChar > 31))
        {
            gpsBuf[gpsBufPos++] = gpsChar;
        }
    #endif
        if (gpsChar == 36)
        {
            // $ (ASCII 36) marks start of message, so we switch indicator every message
            if (millis() - lastGPSUpdate > 500)
            {
                indicator     = adjustWrap(indicator, 1, 0, 3);
                lastGPSUpdate = millis();
            }
        }
        if (gps.encode(gpsChar))
        {
            gpsDiagSentences++;
            gpsDiagLastSentenceMs = millis();
    #if DEBUG_LEVEL & DEBUG_GPS
            gpsBuf[gpsBufPos++] = 0;
            LOG(DEBUG_GPS, "[GPS]: Sentence: [%s]", gpsBuf);
            gpsBufPos = 0;
    #endif

            LOG(DEBUG_GPS,
                "[GPS]: Encoded. %l sats, Location is%svalid, age is %lms",
                gps.satellites.value(),
                (gps.location.isValid() ? " " : " NOT "),
                gps.location.age());
            // Make sure we got a fix in the last 30 seconds
            if ((gps.location.lng() != 0) && (gps.location.age() < 30000UL))
            {
                LOG(DEBUG_INFO, "[GPS]: Sync'd GPS location. Age is %d secs", gps.location.age() / 1000);
                LOG(DEBUG_INFO, "[GPS]: Location: %f  %f", gps.location.lat(), gps.location.lng());
                LOG(DEBUG_INFO, "[GPS]: UTC time is %dh%dm%ds", gps.time.hour(), gps.time.minute(), gps.time.second());
                lcdMenu.printMenu("GPS sync'd....");

                DayTime utcNow = DayTime(gps.time.hour(), gps.time.minute(), gps.time.second());
                utcNow.addHours(mount.getLocalUtcOffset());
                mount.setLocalStartTime(utcNow);
                mount.setLocalStartDate(gps.date.year(), gps.date.month(), gps.date.day());
                mount.setLatitude(gps.location.lat());
                mount.setLongitude(gps.location.lng());

                mount.delay(500);

                return true;
            }
        }
    }
    return false;
#endif
}

String getGPSDebugSnapshot(unsigned long pollMs = 300)
{
    const unsigned long until = millis() + pollMs;
    int indicator             = 0;

    // Drain incoming GPS bytes for a short window and update parser/counters.
    while (millis() < until)
    {
        gpsAqcuisitionComplete(indicator);
        delay(2);
    }

    char buf[120];
    snprintf(buf,
             sizeof(buf),
             "%lu,%lu,%u,%u,%lu,%d#",
             gpsDiagBytesRead,
             gpsDiagSentences,
             static_cast<unsigned>(gps.satellites.value()),
             gps.location.isValid() ? 1u : 0u,
             static_cast<unsigned long>(gps.location.age()),
             gpsDiagLastByte);
    return String(buf);
}

String getGPSDebugHexSnapshot(unsigned long pollMs = 300)
{
    const unsigned long until = millis() + pollMs;
    int indicator             = 0;
    while (millis() < until)
    {
        gpsAqcuisitionComplete(indicator);
        delay(2);
    }

    String out = String(gpsDiagHexCount) + ",";
    for (int i = 0; i < gpsDiagHexCount; i++)
    {
        const int start = (gpsDiagHexWriteIndex - gpsDiagHexCount + GPS_DIAG_HEX_WINDOW) % GPS_DIAG_HEX_WINDOW;
        const int idx   = (start + i) % GPS_DIAG_HEX_WINDOW;
        char hex[4];
        snprintf(hex, sizeof(hex), "%02X", static_cast<unsigned>(gpsDiagHexWindow[idx]));
        out += hex;
        if (i + 1 < gpsDiagHexCount)
        {
            out += ".";
        }
    }
    out += "#";
    return out;
}

    #if DISPLAY_TYPE > 0

// States that HA menu displays goes through
enum haMenuState_t
{
    SHOWING_HA_SYNC = 1,
    SHOWING_HA_SET,
    SHOWING_HA_EXIT,
    ENTER_HA_MANUALLY,
    STARTING_GPS,
};

int indicator         = 0;
haMenuState_t haState = STARTING_GPS;
bool haGpsEditing     = false;

bool processHAKeys()
{
    lcdButton_t key;
    bool waitForRelease = false;

    if (haState == STARTING_GPS)
    {
        if (gpsAqcuisitionComplete(indicator))
        {
            LOG(DEBUG_INFO, "[HA]: GPS acquired");
#if USE_GPS_I2C_ADAFRUIT == 0
            GPS_SERIAL_PORT.end();
#endif
            haState = SHOWING_HA_SYNC;
        #if SUPPORT_GUIDED_STARTUP == 1
            if (startupState == StartupWaitForHACompletion)
            {
                LOG(DEBUG_INFO, "[HA]: We were in startup, so confirm HA");
                startupState = StartupHAConfirmed;
                inStartup    = true;
            }
        #endif
        }
    }

    if (lcdButtons.keyChanged(&key))
    {
        waitForRelease = true;
        LOG(DEBUG_INFO, "[HA]: Key %d was pressed in state %d", key, haState);
        if (haState == SHOWING_HA_SYNC)
        {
            if (key == btnSELECT)
            {
                haState = STARTING_GPS;
#if USE_GPS_I2C_ADAFRUIT == 0
                GPS_SERIAL_PORT.begin(GPS_BAUD_RATE);
#endif
            }
#if USES_ROTARY_ENCODER == 1
            else if (key == btnLEFT)
            {
                haState = SHOWING_HA_EXIT;
            }
            else if (key == btnRIGHT)
            {
                haState = SHOWING_HA_SET;
            }
#else
            else if ((key == btnUP) || (key == btnDOWN))
            {
                haState = SHOWING_HA_SET;
            }
#endif
        }
        else if (haState == SHOWING_HA_SET)
        {
            if (key == btnSELECT)
            {
                haState = ENTER_HA_MANUALLY;
                haGpsEditing = false;
            }
#if USES_ROTARY_ENCODER == 1
            else if (key == btnLEFT)
            {
                haState = SHOWING_HA_SYNC;
            }
            else if (key == btnRIGHT)
            {
                haState = SHOWING_HA_EXIT;
            }
#else
            else if ((key == btnUP) || (key == btnDOWN))
            {
                haState = SHOWING_HA_SYNC;
            }
#endif
        }
        else if (haState == SHOWING_HA_EXIT)
        {
            if (key == btnSELECT)
            {
                requestBackToTop = true;
            }
            else if (key == btnLEFT)
            {
                haState = SHOWING_HA_SET;
            }
            else if (key == btnRIGHT)
            {
                haState = SHOWING_HA_SYNC;
            }
        }
        else if (haState == ENTER_HA_MANUALLY)
        {
#if USES_ROTARY_ENCODER == 1
            if (key == btnSELECT)
            {
                if (!haGpsEditing)
                {
                    haGpsEditing = true;
                }
                else
                {
                    DayTime ha(mount.HA());
                    EEPROMStore::storeHATime(mount.HA());
                    lcdMenu.printMenu("Stored.");
                    mount.delay(500);
                    haState       = SHOWING_HA_SET;
                    haGpsEditing  = false;
        #if SUPPORT_GUIDED_STARTUP == 1
                    if (startupState == StartupWaitForHACompletion)
                    {
                        startupState = StartupHAConfirmed;
                        inStartup    = true;
                    }
        #endif
                }
            }
            else if (!haGpsEditing && ((key == btnLEFT) || (key == btnRIGHT)))
            {
                int delta = (key == btnRIGHT) ? 1 : -1;
                HAselect  = adjustWrap(HAselect, delta, 0, 1);
            }
            else if (haGpsEditing && ((key == btnLEFT) || (key == btnRIGHT)))
            {
                DayTime ha(mount.HA());
                int delta = (key == btnRIGHT) ? 1 : -1;
                if (HAselect == 0)
                    ha.addHours(delta);
                if (HAselect == 1)
                    ha.addMinutes(delta);
                mount.setHA(ha);
            }
#else
            if (key == btnSELECT)
            {
                DayTime ha(mount.HA());
                EEPROMStore::storeHATime(mount.HA());
                lcdMenu.printMenu("Stored.");
                mount.delay(500);
                haState = SHOWING_HA_SET;
        #if SUPPORT_GUIDED_STARTUP == 1
                if (startupState == StartupWaitForHACompletion)
                {
                    startupState = StartupHAConfirmed;
                    inStartup    = true;
                }
        #endif
            }
            else if (key == btnUP)
            {
                DayTime ha(mount.HA());
                if (HAselect == 0)
                    ha.addHours(1);
                if (HAselect == 1)
                    ha.addMinutes(1);
                mount.setHA(ha);
            }
            else if (key == btnDOWN)
            {
                DayTime ha(mount.HA());
                if (HAselect == 0)
                    ha.addHours(-1);
                if (HAselect == 1)
                    ha.addMinutes(-1);
                mount.setHA(ha);
            }
            else if (key == btnLEFT)
            {
                HAselect = adjustWrap(HAselect, 1, 0, 1);
            }
#endif
        }

        if (key == btnRIGHT)
        {
            LOG(DEBUG_INFO, "[HA]: Right Key was pressed");
            if (haState == STARTING_GPS)
            {
                LOG(DEBUG_INFO, "[HA]: In GPS Start mode, switching to manual");
#if USE_GPS_I2C_ADAFRUIT == 0
                GPS_SERIAL_PORT.end();
#endif
                haState = SHOWING_HA_SYNC;
            }
#if USES_ROTARY_ENCODER == 0
        #if SUPPORT_GUIDED_STARTUP == 1
            else if (startupState == StartupWaitForHACompletion)
            {
                LOG(DEBUG_INFO, "[HA]: In Startup, not in GPS Start mode, leaving");
                startupState = StartupHAConfirmed;
                inStartup    = true;
            }
        #endif
            else
            {
                LOG(DEBUG_INFO, "[HA]: leaving HA");
                lcdMenu.setNextActive();
            }
#endif
        }
    }

    return waitForRelease;
}

void printHASubmenu()
{
    const char *ind = "*+* ";
    char satBuffer[20];
    if (haState == SHOWING_HA_SYNC)
    {
        sprintf(satBuffer, "%02dh %02dm >Sync", mount.HA().getHours(), mount.HA().getMinutes());
    }
    else if (haState == SHOWING_HA_SET)
    {
        sprintf(satBuffer, "%02dh %02dm >Set", mount.HA().getHours(), mount.HA().getMinutes());
    }
    else if (haState == SHOWING_HA_EXIT)
    {
        sprintf(satBuffer, "          >Exit");
    }
    else if (haState == STARTING_GPS)
    {
        sprintf(satBuffer, "  Found %u sats", static_cast<unsigned>(gps.satellites.value()));
        satBuffer[0] = ind[indicator];
    }
    else if (haState == ENTER_HA_MANUALLY)
    {
        sprintf(satBuffer, " %02dh %02dm%c", mount.HA().getHours(), mount.HA().getMinutes(), haGpsEditing ? '*' : ' ');
        satBuffer[HAselect * 4] = '>';
    }
    lcdMenu.printMenu(satBuffer);
}

    #endif
#endif
