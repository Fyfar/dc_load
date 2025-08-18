#pragma once

#include <Arduino.h>
#include <DataTypes.h>
#include <config.h>

class LoadMode {
   public:
    // Set the load mode and return the target current in Amperes
    uint16_t setLoadMode(const ElectronicLoadData &data) {
        switch (data.loadMode) {
            case LoadModeValue::CC:
                return setConstantCurrent(data);
            case LoadModeValue::CV:
                return setConstantVoltage(data);
            case LoadModeValue::CR:
                return setConstantResistance(data);
            case LoadModeValue::CP:
                return setConstantPower(data);
            default:
                return 0;  // Invalid mode
        };
    }

   private:
    // Set constant current mode (returns target current in Amperes)
    uint16_t setConstantCurrent(const ElectronicLoadData &data);

    // Set constant voltage mode (returns required current in Amperes)
    uint16_t setConstantVoltage(const ElectronicLoadData &electronicLoadData);

    // Set constant resistance mode (returns required current in Amperes)
    uint16_t setConstantResistance(const ElectronicLoadData &electronicLoadData);

    // Set constant power mode (returns required current in Amperes)
    uint16_t setConstantPower(const ElectronicLoadData &electronicLoadData);
};