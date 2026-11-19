#ifndef MOCK_ARDUINO_H
#define MOCK_ARDUINO_H

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <iomanip>

typedef uint8_t byte;
typedef bool boolean;

#define HIGH 0x1
#define LOW  0x0

#define INPUT 0x0
#define OUTPUT 0x1
#define INPUT_PULLUP 0x2

#define BIN 2
#define OCT 8
#define DEC 10
#define HEX 16

#define SS 10

#define F(x) (x)

// Global simulation state
extern unsigned long simulated_time_ms;
extern std::map<int, int> simulated_pins;

inline unsigned long millis() {
    return simulated_time_ms;
}

inline void delay(unsigned long ms) {
    simulated_time_ms += ms;
}

inline int digitalRead(uint8_t pin) {
    if (simulated_pins.find(pin) != simulated_pins.end()) {
        return simulated_pins[pin];
    }
    return HIGH; // Default pull-up: HIGH (unpressed/down)
}

inline void digitalWrite(uint8_t pin, uint8_t val) {
    simulated_pins[pin] = val;
}

inline void pinMode(uint8_t pin, uint8_t mode) {
    // No-op for simulation
}

class String : public std::string {
public:
    String() : std::string() {}
    String(const char* s) : std::string(s ? s : "") {}
    String(const std::string& s) : std::string(s) {}
    String(int v) : std::string(std::to_string(v)) {}
    String(unsigned int v) : std::string(std::to_string(v)) {}
    String(long v) : std::string(std::to_string(v)) {}
    String(unsigned long v) : std::string(std::to_string(v)) {}
    String(uint8_t v) : std::string(std::to_string(v)) {}
    String(uint8_t v, int base) : std::string(std::to_string(v)) {}
};

inline String operator+(const String& lhs, const String& rhs) {
    return String(static_cast<const std::string&>(lhs) + static_cast<const std::string&>(rhs));
}
inline String operator+(const char* lhs, const String& rhs) {
    return String(std::string(lhs) + static_cast<const std::string&>(rhs));
}
inline String operator+(const String& lhs, const char* rhs) {
    return String(static_cast<const std::string&>(lhs) + std::string(rhs));
}

class HardwareSerial {
public:
    void begin(unsigned long baud) {}
    
    template<typename T>
    void print(T v) {}
    
    template<typename T, typename U>
    void print(T v, U u) {}

    void print(const String& s) {}
    void print(const char* s) {}
    void print(char c) {}

    template<typename T>
    void println(T v) {}
    
    void println(const char* s = "") {}
    void println(const String& s) {}
    void println(char c) {}
};

extern HardwareSerial Serial;

#endif // MOCK_ARDUINO_H
