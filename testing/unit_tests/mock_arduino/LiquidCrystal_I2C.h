#ifndef MOCK_LIQUIDCRYSTAL_I2C_H
#define MOCK_LIQUIDCRYSTAL_I2C_H

#include "Arduino.h"

class LiquidCrystal_I2C {
public:
    LiquidCrystal_I2C(uint8_t addr, uint8_t cols, uint8_t rows) {
        clear();
    }
    void init() {
        clear();
    }
    void backlight() {}
    void noBacklight() {}
    void clear() {
        for (int r = 0; r < 4; r++) {
            buffer[r] = std::string(20, ' ');
        }
        cursor_row = 0;
        cursor_col = 0;
    }
    void setCursor(uint8_t col, uint8_t row) {
        cursor_col = col;
        cursor_row = row;
    }
    void print(const char* str) {
        if (!str) return;
        for (size_t i = 0; str[i] != '\0' && (cursor_col + i) < 20; i++) {
            if (cursor_row < 4) {
                buffer[cursor_row][cursor_col + i] = str[i];
            }
        }
        cursor_col += strlen(str);
        if (cursor_col > 20) cursor_col = 20;
    }
    void print(char c) {
        if (cursor_row < 4 && cursor_col < 20) {
            buffer[cursor_row][cursor_col++] = c;
        }
    }
    void print(const String& s) {
        print(s.c_str());
    }
    void print(int val) {
        print(std::to_string(val).c_str());
    }

    std::string getRow(int r) const {
        return (r >= 0 && r < 4) ? buffer[r] : "";
    }

    std::string buffer[4];
    uint8_t cursor_col = 0;
    uint8_t cursor_row = 0;
};

#endif // MOCK_LIQUIDCRYSTAL_I2C_H
