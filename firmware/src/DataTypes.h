#pragma once

#include <Arduino.h>

enum LoadModeValue {
    CC = 0,
    CV,
    CR,
    CP
};

struct BatteryType {
    const char *name;
    float cutoffVoltage;
};

constexpr BatteryType batteryTypes[] = {
    {"Custom", 0.0f},
    {"LiPo", 3.0f},
    {"LiIon", 3.0f},
    {"LiFePo4", 2.5f},
    {"NiMH", 0.9f}};

struct ElectronicLoadData {
    float loadVoltage;
    float loadCurrent;
    float desiredLoad;
    float batteryCapacity = 0.0f;    // in Ah
    float batteryCapacityWh = 0.0f;  // in Wh
    uint8_t temperature;
    float cutoffVoltage;
    BatteryType batteryType = batteryTypes[0];
    bool outputEnabled = false;
    bool kelvinConnection = true;
    bool menuEnabled = false;
    int8_t cursorPosition = 0;
    uint8_t menuItemIndex = 0;
    bool integerMode = true;
    LoadModeValue loadMode = LoadModeValue::CC;
};