#include <LoadMode.h>
#include <config.h>

// Constant Current Mode
uint16_t LoadMode::setConstantCurrent(const ElectronicLoadData &data) {
    uint16_t current = data.desiredLoad * 1000.f;                  // Convert to mA
    current = constrain(current, MINIMUM_CURRENT, CURRENT_LIMIT);  // Clamp to limits
    return current;
}

// Constant Voltage Mode
uint16_t LoadMode::setConstantVoltage(const ElectronicLoadData &data) {
    float error = data.loadVoltage - data.desiredLoad;
    uint16_t newCurrent = static_cast<uint16_t>((data.loadCurrent * 1000.0) + (error * 1000.0f));  // Adjusted P controller

    newCurrent = constrain(newCurrent, MINIMUM_CURRENT, CURRENT_LIMIT);  // Clamp to limits
    return newCurrent;
}

// Constant Resistance Mode
uint16_t LoadMode::setConstantResistance(const ElectronicLoadData &data) {
    if (data.desiredLoad <= 0 || data.loadVoltage <= 0) return CURRENT_LIMIT;  // Return minimum current

    uint16_t calculatedCurrent = static_cast<uint16_t>(data.loadVoltage * 1000.0f / data.desiredLoad);
    calculatedCurrent = constrain(calculatedCurrent, MINIMUM_CURRENT, CURRENT_LIMIT);  // Clamp to limits

    return calculatedCurrent;
}

// Constant Power Mode
uint16_t LoadMode::setConstantPower(const ElectronicLoadData &data) {
    if (data.loadVoltage <= 0) return CURRENT_LIMIT;  // Return minimum current

    uint16_t calculatedCurrent = static_cast<uint16_t>(data.desiredLoad * 1000.0f / data.loadVoltage);
    calculatedCurrent = constrain(calculatedCurrent, MINIMUM_CURRENT, CURRENT_LIMIT);  // Clamp to limits

    return calculatedCurrent;
}