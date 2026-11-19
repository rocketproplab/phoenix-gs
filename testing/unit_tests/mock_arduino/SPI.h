#ifndef MOCK_SPI_H
#define MOCK_SPI_H

#include "Arduino.h"

class SPIClass {
public:
    void begin() {}
    void end() {}
    uint8_t transfer(uint8_t b) { return 0; }
};

extern SPIClass SPI;

#endif // MOCK_SPI_H
