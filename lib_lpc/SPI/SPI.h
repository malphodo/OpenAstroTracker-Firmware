#pragma once

#include <stdint.h>

#ifndef MSBFIRST
    #define MSBFIRST 1
#endif

#ifndef SPI_MODE3
    #define SPI_MODE3 3
#endif
#ifndef SPI_MODE0
    #define SPI_MODE0 0
#endif
#ifndef SPI_MODE1
    #define SPI_MODE1 1
#endif
#ifndef SPI_MODE2
    #define SPI_MODE2 2
#endif
#ifndef SPI_CLOCK_DIV2
    #define SPI_CLOCK_DIV2 2
#endif
#ifndef SPI_CLOCK_DIV4
    #define SPI_CLOCK_DIV4 4
#endif
#ifndef SPI_CLOCK_DIV8
    #define SPI_CLOCK_DIV8 8
#endif

class SPISettings
{
  public:
    SPISettings(uint32_t, uint8_t, uint8_t)
    {
    }
};

class SPIClass
{
  public:
    void begin()
    {
    }
    void end()
    {
    }
    void setClockDivider(uint8_t)
    {
    }
    void setDataMode(uint8_t)
    {
    }
    void setBitOrder(uint8_t)
    {
    }
    void beginTransaction(const SPISettings &)
    {
    }
    void endTransaction()
    {
    }
    uint8_t transfer(uint8_t data)
    {
        return data;
    }
};

extern SPIClass SPI;
