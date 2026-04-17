#pragma once

/**
 * These are only necessary for the Arduino IDE build, these are actually
 * defined in `pre_script_custom_defines.py` which is run by platformio.
 */
#if !defined(PUSH_NO_WARNINGS)
    #define PUSH_NO_WARNINGS ;
#endif
#if !defined(POP_NO_WARNINGS)
    #define POP_NO_WARNINGS ;
#endif

PUSH_NO_WARNINGS
#include <Arduino.h>
#include <WString.h>
POP_NO_WARNINGS

#if !defined(yield)
inline void yield()
{
}
#endif

extern bool inSerialControl;  // True when the serial port is in control
extern bool inNorthernHemisphere;
extern uint8_t rotaryMolSens;  // Encoder / jog sensitivity (see Config → JogSens), 1..8
