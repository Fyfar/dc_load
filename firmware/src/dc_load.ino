#include <Adafruit_ADS1X15.h>
#include <Adafruit_MCP4725.h>
#include <Arduino.h>
#include <DataTypes.h>
#include <EncButton.h>
#include <I2CKeyPad8x8.h>
#include <LcdModule.h>
#include <LoadMode.h>
#include <NTC.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <Wire.h>
#include <config.h>
#include "secrets.h"

#include <vector>

enum class MenuItem {
    BATTERY_TYPE = 0,
    FOUR_WIRE,
    CUTOFF_VOLTAGE
};

constexpr float KELVIN_DIVIDER = 7.10234;
constexpr float VOLTAGE_DIVIDER = 10.0;
constexpr uint16_t SECONDS_IN_HOUR = 3600;
constexpr float HOURS_PER_INTEGRATION_PERIOD = (float)INTEGRATION_PERIOD / SECONDS_IN_HOUR;

constexpr uint8_t cursorPositionPerLoadMode[4] = {14, 14, 18, 15};
constexpr uint8_t cursorPositionMenuScreen[3] = {
    9,  // battery type position
    8,  // 4-wire position
    9   // cutoff voltage position
};
constexpr LoadModeValue loadModeByIndex[4] = {LoadModeValue::CC, LoadModeValue::CV, LoadModeValue::CR, LoadModeValue::CP};

constexpr adsGain_t adcGain[5] = {GAIN_ONE, GAIN_TWO, GAIN_FOUR, GAIN_EIGHT, GAIN_SIXTEEN};

const char keypadLayout[4][4] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}};

EncButtonT<S1_PIN, S2_PIN, BTN_PIN> encoder;
Adafruit_MCP4725 dac;
Adafruit_ADS1115 adc;

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
I2CKeyPad8x8 keypad(KEYPAD_I2C_ADDRESS);

LoadMode loadMode;
ElectronicLoadData data;
LcdModule lcdModule;
NTC ntc(NTC_R25, NTC_BETA, NTC_SERIES_R, NTC_ADC_PIN);

static uint16_t adcTimer = 0;
static uint16_t loadAdjustTimer = 0;
static uint16_t mqttPublishTimer = 0;
static uint16_t keypadTimer = 0;
static uint16_t temperatureTimer = 0;
static uint16_t batteryCapacityTimer = 0;

static uint16_t desiredVoltage = 0;

static uint8_t currentLoadModeIndex = 0;
static uint8_t currentBatteryType = 0;

std::vector<uint8_t> enteredNumbers;
std::vector<uint8_t> enteredFractionalNumbers;

static char mqttMsg[16];
static uint8_t gain = 0;

constexpr uint8_t batteryTypeLength() {
    return sizeof(batteryTypes) / sizeof(BatteryType);
}

void IRAM_ATTR encoderIsr() {
    encoder.tickISR();
}

void IRAM_ATTR btnIsr() {
    encoder.pressISR();
}

void encoderTurnEvent() {
    Serial.println("Encoder turn event");
    changeSelectedValue(encoder.dir());
    lcdModule.updateLcd(data, true);
}

void changeSelectedValue(int8_t direction) {
    if (data.menuEnabled) {
        MenuItem menuItem = static_cast<MenuItem>(data.menuItemIndex);
        switch (menuItem) {
            case MenuItem::BATTERY_TYPE:
                changeBatteryType(direction);
                break;
            case MenuItem::FOUR_WIRE:
                changeMeasureType(direction);
                break;
            case MenuItem::CUTOFF_VOLTAGE:
                changeCutoffVoltage(direction);
                break;
        };
    } else {
        changeDesiredCurrent(direction);
    }
}

void changeDesiredCurrent(int8_t direction) {
    changeValueByEncoder(direction, data.desiredLoad, getMaxDesiredLoad());
}

void changeBatteryType(int8_t direction) {
    currentBatteryType += direction;
    if (currentBatteryType < 0) {
        currentBatteryType = batteryTypeLength() - 1;
    } else if (currentBatteryType >= batteryTypeLength()) {
        currentBatteryType = 0;
    }
    data.cutoffVoltage = batteryTypes[currentBatteryType].cutoffVoltage;
    data.batteryType = batteryTypes[currentBatteryType];
}

void changeMeasureType(int8_t direction) {
    data.kelvinConnection = !data.kelvinConnection;
}

void changeCutoffVoltage(int8_t direction) {
    data.batteryType = batteryTypes[0];
    currentBatteryType = 0;
    changeValueByEncoder(direction, data.cutoffVoltage, VOLTAGE_LIMIT);
    Serial.printf("cutoff voltage: %.3fV\n", data.cutoffVoltage);
}

void changeValueByEncoder(int8_t direction, float &value, float limit) {
    if (data.integerMode) {
        value += 1 * direction;
    } else {
        uint16_t multiplier = encoder.fast() ? 10 : 1;

        if (data.loadMode == LoadModeValue::CP) {
            multiplier *= 10;
        }

        value += 0.001 * multiplier * direction;
    }
    value = constrain(value, 0, limit);
}

void changeLoadValueEnterMode() {
    if (data.loadMode == LoadModeValue::CR) {
        data.integerMode = true;
        Serial.println("CR mode does not support integer/fractional mode switching");
        return;
    }

    data.integerMode = !data.integerMode;
    if (data.integerMode) {
        data.cursorPosition -= 2;
    } else {
        data.cursorPosition += 2;
    }
}

void handleClickEventOnMenu() {
    data.menuItemIndex++;
    if (data.menuItemIndex > 2) data.menuItemIndex = 0;
    updateCursorPosition();
}

void encoderClickEvent() {
    Serial.println("Encoder click event");
    if (data.menuEnabled) {
        handleClickEventOnMenu();
    } else {
        changeLoadValueEnterMode();
    }
    lcdModule.updateLcd(data, true);
}

void encoderHoldEvent() {
    Serial.println("Encoder hold event");
    data.menuEnabled = !data.menuEnabled;
    lcdModule.clear();
    clearEnteredValue();
    updateCursorPosition();
}

void encoderCallback() {
    switch (encoder.getAction()) {
        case EBAction::Turn:
            encoderTurnEvent();
            break;
        case EBAction::Click:
            encoderClickEvent();
            break;
        case EBAction::Hold:
            encoderHoldEvent();
            break;
    }
}

void adjustDesiredCurrentByLoad() {
    uint16_t ms = millis();
    if (data.outputEnabled && (uint16_t)(ms - loadAdjustTimer) >= LOAD_ADJUST_PERIOD) {
        loadAdjustTimer = ms;
        desiredVoltage = loadMode.setLoadMode(data);
        setDacOutput();
    }
}

void disableOutput() {
    data.outputEnabled = false;
    lcdModule.updateLcd(data, true);
    setDacOutput();
}

void toggleOutput() {
    if (data.outputEnabled) {
        disableOutput();
    } else {
        data.outputEnabled = true;
        desiredVoltage = loadMode.setLoadMode(data);
        lcdModule.updateLcd(data, true);
        setDacOutput();
    }
    clearEnteredValue();
}

void clearEnteredValue() {
    enteredNumbers.clear();
    enteredFractionalNumbers.clear();
    data.integerMode = true;
}

void setLoadMode(LoadModeValue loadModeValue) {
    clearEnteredValue();
    data.desiredLoad = 0;
    data.outputEnabled = false;
    data.loadMode = loadModeValue;
    currentLoadModeIndex = static_cast<uint8_t>(data.loadMode);
    updateCursorPosition();
}

void keypadTick() {
    uint16_t ms = millis();
    if ((uint16_t)(ms - keypadTimer) >= KEYPAD_TICK_PERIOD) {
        keypadTimer = ms;

        static bool btnPressed = false;
        static uint8_t lastKey = I2C_KEYPAD8x8_NOKEY;
        uint8_t key = keypad.getKey();

        if (key == I2C_KEYPAD8x8_THRESHOLD || key == I2C_KEYPAD8x8_FAIL) return;

        if (key == I2C_KEYPAD8x8_NOKEY) {
            btnPressed = false;
            return;
        }

        if (btnPressed == true) {
            Serial.println("Keypad button ignored, due to hold");
            return;
        }

        char pressedChar = keypadLayout[key / 8][key % 4];
        Serial.println(pressedChar);

        switch (pressedChar) {
            case '0' ... '9': {
                uint8_t number = pressedChar - '0';
                if (data.integerMode) {
                    if (enteredNumbers.size() < 4) {
                        if (enteredNumbers.size() == 0 && number == 0) {
                            return;  // Ignore leading zero
                        }

                        enteredNumbers.push_back(number);
                    }
                } else {
                    if (enteredFractionalNumbers.size() < 3) {
                        if (enteredFractionalNumbers.size() == 0 && number == 0) {
                            return;  // Ignore leading zero
                        }

                        enteredFractionalNumbers.push_back(number);
                    }
                }

                if (!data.menuEnabled) {
                    data.desiredLoad = min(buildNumber(), getMaxDesiredLoad());
                    Serial.println(data.desiredLoad);
                } else if (data.menuItemIndex == 2) {
                    data.cutoffVoltage = min(buildNumber(), VOLTAGE_LIMIT);
                    Serial.println(data.cutoffVoltage);
                }

                break;
            }
            case 'A': {
                setLoadMode(LoadModeValue::CC);
                break;
            }
            case 'B': {
                setLoadMode(LoadModeValue::CP);
                break;
            }
            case 'C': {
                setLoadMode(LoadModeValue::CR);
                break;
            }
            case 'D': {
                setLoadMode(LoadModeValue::CV);
                break;
            }
            case '*': {
                changeLoadValueEnterMode();
                break;
            }
            case '#': {
                toggleOutput();
                break;
            }
        }
        btnPressed = true;
        lastKey = key;
    }
}

void updateCursorPosition() {
    if (data.menuEnabled) {
        data.cursorPosition = cursorPositionMenuScreen[data.menuItemIndex];
    } else {
        data.cursorPosition = cursorPositionPerLoadMode[currentLoadModeIndex];
    }
}

float getMaxDesiredLoad() {
    switch (data.loadMode) {
        case LoadModeValue::CC:
            return CURRENT_LIMIT / 1000.0f;
        case LoadModeValue::CV:
            return VOLTAGE_LIMIT;
        case LoadModeValue::CP:
            return VOLTAGE_LIMIT * CURRENT_LIMIT / 1000.0;
        case LoadModeValue::CR:
            return VOLTAGE_LIMIT / MINIMUM_CURRENT * 1000.0;
        default:
            return 1.0;
    }
}

float buildNumber() {
    float result = 0.0f;

    // Собираем целую часть
    for (size_t i = 0; i < enteredNumbers.size(); i++) {
        result *= 10;
        result += enteredNumbers[i];
    }

    // Собираем дробную часть
    float fraction = 0.0f;
    float divisor = 10.0f;
    for (size_t i = 0; i < enteredFractionalNumbers.size(); i++) {
        fraction += enteredFractionalNumbers[i] / divisor;
        divisor *= 10.0f;
    }

    result += fraction;

    return result;
}

void controlFan() {
    if (data.temperature >= FAN_START_TEMPERATURE || data.loadCurrent >= FAN_START_CURRENT) {
        digitalWrite(FAN_PIN, HIGH);
    } else if (data.temperature <= FAN_STOP_TEMPERATURE && data.loadCurrent < FAN_START_CURRENT) {
        digitalWrite(FAN_PIN, LOW);
    }
}

void measureTemperature() {
    uint16_t ms = millis();
    if ((uint16_t)(ms - temperatureTimer) >= TEMPERATURE_MEASURE_PERIOD) {
        temperatureTimer = ms;

        if (data.menuEnabled) {
            return;
        }

        float temperature = ntc.getAverageTemperature();
        data.temperature = (uint8_t)temperature;

        controlFan();

        if (temperature >= TEMPERATURE_LIMIT) {
            Serial.println("Temperature limit exceeded, disabling output");
            disableOutput();
        }
    }
}

void calculateCapacity() {
    if (!data.outputEnabled || data.loadCurrent <= 0) {
        return;
    }

    uint16_t ms = millis();
    if ((uint16_t)(ms - batteryCapacityTimer) >= INTEGRATION_PERIOD) {
        batteryCapacityTimer = ms;

        if (data.cutoffVoltage > 0 && data.cutoffVoltage > data.loadVoltage) {
            Serial.println("Cutoff voltage reached, disabling output");
            disableOutput();
            return;
        }

        data.batteryCapacity += data.loadCurrent * HOURS_PER_INTEGRATION_PERIOD;
        data.batteryCapacityWh += (data.loadCurrent * data.loadVoltage) / SECONDS_IN_HOUR;
    }
}

bool isADCGainTuned(int16_t adc0, int16_t adc1) {
    if (max(adc0, adc1) >= 25000 && gain > 0) {
        gain--;
        adc.setGain(adcGain[gain]);
        Serial.println("Reducing ADC gain...");
        return false;
    } else if (max(adc0, adc1) <= 10000 && gain < 4) {
        gain++;
        adc.setGain(adcGain[gain]);
        Serial.println("Increasing ADC gain...");
        return false;
    }

    return true;
}

void measureVoltageAndCurrent() {
    uint16_t ms = millis();
    if ((uint16_t)(ms - adcTimer) >= ADC_MEASURE_PERIOD) {
        adcTimer = ms;

        int16_t adcCurrent;
        int16_t adcVoltage;
        float voltageDivider = data.kelvinConnection ? KELVIN_DIVIDER : VOLTAGE_DIVIDER;

        if (data.outputEnabled) {
            adcCurrent = adc.readADC_SingleEnded(0);
        }

        if (data.kelvinConnection) {
            adcVoltage = adc.readADC_Differential_2_3();
        } else {
            adcVoltage = adc.readADC_SingleEnded(1);
        }

        if (isADCGainTuned(adcCurrent, adcVoltage)) {
            data.loadCurrent = adc.computeVolts(adcCurrent) / SHUNT_OHM;
            data.loadVoltage = adc.computeVolts(adcVoltage) * voltageDivider;
        }
    }
}

void setDacOutput() {
    if (data.outputEnabled && desiredVoltage > 0) {
        uint16_t dacCode = round((desiredVoltage - CALIBRATION_OFFSET) / CALIBRATION_CLOPE);
        dacCode = constrain(dacCode, 0, DAC_VREF);
        dac.setVoltage(dacCode, false);
        Wire.setClock(I2C_CLOCK);
    } else {
        dac.setVoltage(0, false);
        Wire.setClock(I2C_CLOCK);
    }
}

void publishDataToMqtt() {
    uint16_t ms = millis();
    if ((uint16_t)(ms - mqttPublishTimer) >= MQTT_PUBLISH_PERIOD) {
        mqttPublishTimer = ms;

        snprintf(mqttMsg, 16, "%0.3f", data.loadVoltage);
        mqttClient.publish("dcLoad/voltage", mqttMsg);

        snprintf(mqttMsg, 16, "%0.3f", data.loadCurrent);
        mqttClient.publish("dcLoad/current", mqttMsg);

        snprintf(mqttMsg, 16, "%0d", data.temperature);
        mqttClient.publish("dcLoad/temperature", mqttMsg);

        snprintf(mqttMsg, 16, "%0.3f", data.batteryCapacity);
        mqttClient.publish("dcLoad/capacity/mah", mqttMsg);

        snprintf(mqttMsg, 16, "%0.2f", data.batteryCapacityWh);
        mqttClient.publish("dcLoad/capacity/wh", mqttMsg);
    }
}

void setupWifi() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    WiFi.setTxPower(WIFI_POWER_2dBm);
    while (WiFi.status() != WL_CONNECTED) {
        Serial.println("Connection Failed!");
        delay(1000);
    }
}

void mqttConnect() {
    while (!mqttClient.connected()) {
        Serial.print("Attempting MQTT connection...");
        // Create a random client ID
        String clientId = "ESP32_DC_Load-";
        clientId += String(random(0xffff), HEX);
        // Attempt to connect
        if (mqttClient.connect(clientId.c_str())) {
            Serial.println("connected");
        } else {
            Serial.print("failed, rc=");
            Serial.print(mqttClient.state());
            Serial.println(" try again in 5 seconds");
            // Wait 5 seconds before retrying
            delay(3000);
        }
    }
}

void setup() {
    Serial.begin(115200);

    Wire.setPins(8, 9);
    Wire.setClock(I2C_CLOCK);

    dac.begin(DAC_I2C_ADDRESS);
    dac.setVoltage(0, false);

    adc.begin();
    adc.setGain(adcGain[gain]);
    adc.setDataRate(RATE_ADS1115_16SPS);

    attachInterrupt(S1_PIN, encoderIsr, CHANGE);
    attachInterrupt(S2_PIN, encoderIsr, CHANGE);
    attachInterrupt(BTN_PIN, btnIsr, FALLING);
    encoder.setEncISR(true);
    encoder.attach(encoderCallback);

    pinMode(FAN_PIN, OUTPUT);
    digitalWrite(FAN_PIN, HIGH);  // Test fan

    setupWifi();
    mqttClient.setServer(mqttServer, 1883);
    mqttConnect();

    if (keypad.begin() == false) {
        Serial.println("\nERROR: cannot communicate to keypad.\nPlease reboot.\n");
        while (1);
    }

    keypad.setDebounceThreshold(KEYPAD_DEBOUNCE_TIME);

    Serial.println("Ready to measure");
    Serial.println(WiFi.localIP());

    lcdModule.begin();
    lcdModule.printInitScreen();

    adcTimer = (uint16_t)millis();
    loadAdjustTimer = (uint16_t)millis();
    mqttPublishTimer = (uint16_t)millis();
    keypadTimer = (uint16_t)millis();
    temperatureTimer = (uint16_t)millis();
    batteryCapacityTimer = (uint16_t)millis();

    delay(3000);
    data.cursorPosition = 14;
    lcdModule.clear();
    digitalWrite(FAN_PIN, LOW);  // Fan off
}

void loop() {
    encoder.tick();

    measureVoltageAndCurrent();
    measureTemperature();
    adjustDesiredCurrentByLoad();
    keypadTick();
    calculateCapacity();
    lcdModule.updateLcd(data);

    mqttClient.loop();
    if (!mqttClient.connected()) {
        mqttConnect();
    }
    publishDataToMqtt();
}
