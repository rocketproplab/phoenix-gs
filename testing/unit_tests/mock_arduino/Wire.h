#ifndef MOCK_WIRE_H
#define MOCK_WIRE_H

#include "Arduino.h"

class TwoWire {
public:
    void begin() {}
};

extern TwoWire Wire;

#endif // MOCK_WIRE_H
