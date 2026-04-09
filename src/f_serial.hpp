#pragma once

#if TEST_VERIFY_MODE == 1
    #include "testmenu.hpp"
    #include "testmenudef.hpp"
#endif

#include "b_setup.hpp"
#if defined(BOARD) && (BOARD == BOARD_LPC1769_SKR_V14_TURBO)
    #include <CDCSerial.h>
#endif

#if defined(BOARD) && (BOARD == BOARD_LPC1769_SKR_V14_TURBO)
inline void ensureUsbSerialReady()
{
    // OATControl opens COM with DTR disabled. On LPC176x CDC this can keep
    // host_connected false and silently drop writes. Force-enable TX path.
    if (!UsbSerial.host_connected)
    {
        UsbSerial.host_connected = true;
    }
}
#endif

#if SUPPORT_SERIAL_CONTROL == 1
    #include "MeadeCommandProcessor.hpp"

void processSerialData();

////////////////////////////////////////////////
// The main loop when under serial control
void serialLoop()
{
    mount.loop();
    mount.displayStepperPositionThrottled();

    #if defined(ESP32) || (defined(BOARD) && (BOARD == BOARD_LPC1769_SKR_V14_TURBO))
    processSerialData();
    #endif

    #if (WIFI_ENABLED == 1)
    wifiControl.loop();
    #endif

    #if TEST_VERIFY_MODE == 1
    mainTestMenu.tick();
    #endif
}

    //////////////////////////////////////////////////
    // Event that is triggered when the serial port receives data.
    #ifndef ESP32
void serialEvent()
{
    processSerialData();
}
    #endif

    #if TEST_VERIFY_MODE == 1

void processTestState()
{
    static char buffer[32];
    static unsigned int index = 0;
    switch (TestMenu::getMenuState())
    {
        case testMenuState_t::CLEAR:
            TestMenu::setMenuState(testMenuState_t::WAITING_ON_INPUT);
            break;

        case testMenuState_t::WAITING_ON_INPUT:
            while (Serial.available() > 0)
            {
                if (Serial.readBytes(buffer, 1) == 1)
                {
                    if ((buffer[0] >= '0') && (buffer[0] <= '9'))
                    {
                        Serial.println(buffer[0]);
                        int pressedKey = buffer[0] - '0';
                        TestMenu::getCurrentMenu()->onKeyPressed(pressedKey);
                    }
                }
            }
            break;

        case testMenuState_t::WAITING_ON_COMMAND:
            while (Serial.available() > 0)
            {
                char ch;
                if (Serial.readBytes(&ch, 1) == 1)
                {
                    if (isascii(ch))
                    {
                        buffer[index] = ch;
                        if (ch == '#')
                        {
                            buffer[index + 1] = '\0';
                            TestMenu::getCurrentMenu()->onCommandReceived(buffer);
                            TestMenu::setMenuState(testMenuState_t::WAITING_ON_INPUT);
                            TestMenu::getCurrentMenu()->display();
                            index = 0;
                        }
                        else
                        {
                            index++;
                            if (index > ARRAY_SIZE(buffer) - 1)
                            {
                                Serial.println(F("Buffer overflow, too many chars received"));
                                index = 0;
                            }
                        }
                    }
                }
            }
            break;
    }
}

void processSerialData()
{
    processTestState();
}
    #else
// ESP needs to call this in a loop :_(
void processSerialData()
{
    static char buffer[20];
    static unsigned int index = 0;
    for (;;)
    {
        bool readOk = false;
    #if defined(BOARD) && (BOARD == BOARD_LPC1769_SKR_V14_TURBO)
        // PC control path is USB CDC only on SKR (see setup: UART0 not started in release builds).
        if (UsbSerial.available() > 0)
        {
            readOk = (UsbSerial.readBytes((buffer + index), 1) == 1);
        }
    #else
        if (Serial.available() > 0)
        {
            readOk = (Serial.readBytes((buffer + index), 1) == 1);
        }
    #endif

        if (!readOk)
        {
            break;
        }

        if (buffer[index] == 0x06)
        {
            LOG(DEBUG_SERIAL, "[SERIAL]: Received: ACK request, replying P");
            #if defined(BOARD) && (BOARD == BOARD_LPC1769_SKR_V14_TURBO)
            ensureUsbSerialReady();
            UsbSerial.print('P');
            #else
            Serial.print('P');
            #endif
            index = 0;
        }
        else if (buffer[index] == '#')
        {
            // Ignoring trailing hash
            buffer[index]      = '\0';
            const String inCmd = String(buffer);
            LOG(DEBUG_SERIAL, "[SERIAL]: ReceivedCommand(%d chars): [%s]", inCmd.length(), inCmd.c_str());

            const String retVal = MeadeCommandProcessor::instance()->processCommand(inCmd);
            if (retVal != "")
            {
                LOG(DEBUG_SERIAL, "[SERIAL]: RepliedWith:  [%s]", retVal.c_str());
                #if defined(BOARD) && (BOARD == BOARD_LPC1769_SKR_V14_TURBO)
                ensureUsbSerialReady();
                UsbSerial.print(retVal.c_str());
                #else
                Serial.print(retVal.c_str());
                #endif
            }
            // Wait for next command
            index = 0;
        }
        else if (buffer[index] >= ' ')
        {
            index++;
            if (index >= sizeof(buffer))
            {
                LOG(DEBUG_SERIAL, "[SERIAL]: Command buffer overflow! Ignoring received data.");
                index = 0;
            }
        }
        mount.loop();
    }
}

    #endif
#endif
