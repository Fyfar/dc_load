#pragma once

// --------------- ENCODER ---------------------
#define EB_NO_FOR
#define S1_PIN 1
#define S2_PIN 2
#define BTN_PIN 3
#define EB_TOUT_TIME 1000

#define KEYPAD_I2C_ADDRESS 0x21
#define KEYPAD_TICK_PERIOD 20
#define KEYPAD_DEBOUNCE_TIME (KEYPAD_TICK_PERIOD * 5)

// --------------- ADC --------------------------
#define ADC_MEASURE_PERIOD 250
#define LOAD_ADJUST_PERIOD ADC_MEASURE_PERIOD

#define MQTT_PUBLISH_PERIOD 1000

#define I2C_CLOCK 400000

#define TEMPERATURE_MEASURE_PERIOD 5000

// --------------- DAC --------------------------
#define DAC_VREF 4096
#define SHUNT_OHM 0.10156
#define CURRENT_LIMIT 4090  // mA
#define CALIBRATION_OFFSET 1.8616
#define MINIMUM_CURRENT 10  // mA
#define CALIBRATION_CLOPE 0.975533
#define DAC_I2C_ADDRESS 0x60

#define VOLTAGE_LIMIT 30.0f
#define TEMPERATURE_LIMIT 100.0f

// --------------- NTC --------------------------
#define NTC_R25 100000       // Resistance at 25°C
#define NTC_BETA 3950        // Beta coefficient of the thermistor
#define NTC_SERIES_R 100000  // Series resistor value
#define NTC_ADC_PIN 0        // ADC pin number for NTC

// --------------- Battery -----------------------
#define INTEGRATION_PERIOD 1000

// --------------- FAN ---------------------------
#define FAN_PIN 4
#define FAN_START_TEMPERATURE 70
#define FAN_STOP_TEMPERATURE 50
#define FAN_START_CURRENT 2.0f  // Amps
