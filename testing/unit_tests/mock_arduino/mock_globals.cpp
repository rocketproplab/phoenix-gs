#include "Arduino.h"
#include "Wire.h"
#include "SPI.h"

unsigned long simulated_time_ms = 0;
std::map<int, int> simulated_pins;
HardwareSerial Serial;
TwoWire Wire;
SPIClass SPI;
