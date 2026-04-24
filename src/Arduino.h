#pragma once

#if defined(ARDUINO) && defined(__has_include_next) && __has_include_next("Arduino.h")
    #include_next "Arduino.h"
#else
    #include <stdint.h>
    #include <stddef.h>

typedef uint8_t byte;

#ifndef PROGMEM
    #define PROGMEM
#endif

class String
{
  public:
    String() {}
    String(const char *) {}
    String(const String &) {}
    String &operator=(const char *)
    {
        return *this;
    }
    String &operator+=(const char *)
    {
        return *this;
    }
    size_t length() const
    {
        return 0u;
    }
    void toUpperCase() {}
};

inline unsigned long millis()
{
    return 0UL;
}
#endif
