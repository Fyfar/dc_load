#pragma once

#ifndef NTC_H
#define NTC_H

#include <Arduino.h>
#include <math.h>

class NTC {
   private:
    uint32_t R25;         // Resistance at 25°C (ohms)
    uint16_t beta;        // Beta coefficient of the thermistor
    uint32_t seriesR;     // Series resistor value (ohms)
    uint8_t adcPin;       // ADC pin number
    uint16_t voltageRef;  // Voltage reference value

   public:
    NTC(uint32_t r25, uint16_t betaValue, uint32_t seriesResistor, uint8_t pin, uint16_t voltageRef = 3300)
        : R25(r25), beta(betaValue), seriesR(seriesResistor), adcPin(pin), voltageRef(voltageRef) {}

    int8_t readTemperature() {
        uint32_t resistance = readResistance();
        return calculateTemperature(resistance);
    }

    // Get average temperature from multiple readings
    int8_t getAverageTemperature(uint8_t samples = 10) {
        int16_t sum = 0;
        for (uint8_t i = 0; i < samples; ++i) {
            sum += readTemperature();
        }
        return roundf(sum / (float)samples);
    }

    uint32_t readResistance() {
        uint32_t adcVoltage = analogReadMilliVolts(adcPin);
        return seriesR * ((float)voltageRef / adcVoltage - 1);
    }

    int8_t calculateTemperature(uint32_t resistance) {
        float steinhart = log((float)resistance / R25);
        steinhart /= beta;
        steinhart += 1.0 / (25.0 + 273.15);
        steinhart = 1.0 / steinhart;
        return roundf(steinhart - 273.15);  // Convert Kelvin to Celsius
    }
};

#endif  // NTC_H
