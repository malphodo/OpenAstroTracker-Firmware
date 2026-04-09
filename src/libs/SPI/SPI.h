#pragma once

#include <stdint.h>

#ifndef MSBFIRST
    #define MSBFIRST 1
#endif

#ifndef SPI_MODE3
    #define SPI_MODE3 3
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
