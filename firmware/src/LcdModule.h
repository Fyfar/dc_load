#pragma once

// --------------- LCD --------------------------
#define LCD_ROWS 4
#define LCD_COLUMNS 20
#define LCD_UPDATE_PERIOD 500

#include <DataTypes.h>
#include <LoadMode.h>
#include <WiFi.h>
#include <Wire.h>
#include <hd44780.h>
#include <hd44780ioClass/hd44780_I2Cexp.h>

inline const char *const boolToString(bool b) {
    return b ? "ON" : "OFF";
}

static char buffer[21];

class LcdModule {
   public:
    int status;  // Initialize the LCD

    LcdModule() = default;

    void begin();

    void printInitScreen();

    void clear();

    void updateLcd(const ElectronicLoadData &data, bool force = false);

   protected:
    uint16_t lcdTimer = 0;

    // Print voltage and current values
    void printMainDisplay(const ElectronicLoadData &data);

    // Print the menu
    void printMenu(const ElectronicLoadData &data);

    const char *desiredLoadText(const ElectronicLoadData &data);

    void alignTextBothSides(const char *leftPart, const char *rightPart, char *result = buffer);
};