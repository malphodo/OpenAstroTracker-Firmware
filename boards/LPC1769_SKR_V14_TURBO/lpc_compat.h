#pragma once

/**
 * Arduino helpers missing from the LPC176x core, force-included in every
 * translation unit of the skr14turbo build (firmware and libraries such as
 * TinyGPS++) via `-include` in platformio.ini.
 *
 * They used to be `-D radians\(x\)=...` build flags, which only survive the
 * Windows shell; a header works on every host OS.
 */

#ifndef TWO_PI
    #define TWO_PI 6.28318530718f
#endif

#ifndef radians
    #define radians(x) ((x) * 0.017453292519943295f)
#endif

#ifndef degrees
    #define degrees(x) ((x) * 57.29577951308232f)
#endif

#ifndef yield
    #define yield() ((void) 0)
#endif
