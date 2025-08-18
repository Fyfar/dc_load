#include <LcdModule.h>

hd44780_I2Cexp lcd(0x27);

void LcdModule::begin() {
    status = lcd.begin(LCD_COLUMNS, LCD_ROWS);
    lcd.noLineWrap();
    lcd.clear();

    if (status) {
        Serial.println("lcd fatal error");
    }
}

const char *LcdModule::desiredLoadText(const ElectronicLoadData &data) {
    static char desiredLoadBuffer[20];
    switch (data.loadMode) {
        case LoadModeValue::CC:
            sprintf(desiredLoadBuffer, "CC: %0.3fA", data.desiredLoad);
            break;
        case LoadModeValue::CV:
            sprintf(desiredLoadBuffer, "CV: %0.3fV", data.desiredLoad);
            break;
        case LoadModeValue::CR:
            sprintf(desiredLoadBuffer, "CR: %d%c", (uint16_t)data.desiredLoad, 244);
            break;
        case LoadModeValue::CP:
            sprintf(desiredLoadBuffer, "CP: %0.2fW", data.desiredLoad);
            break;
        default:
            break;
    }
    return desiredLoadBuffer;
}

const char *loadModeToString(LoadModeValue loadMode) {
    switch (loadMode) {
        case LoadModeValue::CC:
            return "CC";
        case LoadModeValue::CV:
            return "CV";
        case LoadModeValue::CR:
            return "CR";
        case LoadModeValue::CP:
            return "CP";
        default:
            return "CC";
    };
}

void LcdModule::alignTextBothSides(const char *leftPart, const char *rightPart, char *result) {
    sprintf(result, "%-10s%10s", leftPart, rightPart);
}

void clearDisplayBuffer(char *leftPart, char *rightPart) {
    memset(leftPart, ' ', sizeof(leftPart));
    memset(rightPart, ' ', sizeof(rightPart));
}

void LcdModule::printMainDisplay(const ElectronicLoadData &data) {
    lcd.setCursor(0, 0);
    char leftPart[20];
    char rightPart[20];
    sprintf(leftPart, "%0.3fV", abs(data.loadVoltage));
    sprintf(rightPart, "%0.3fA", data.loadCurrent);
    alignTextBothSides(leftPart, rightPart, buffer);
    lcd.print(buffer);
    clearDisplayBuffer(leftPart, rightPart);

    lcd.setCursor(0, 1);
    uint16_t capacity = (uint16_t)roundf(data.batteryCapacity);
    sprintf(leftPart, "%dmAh", capacity);
    sprintf(rightPart, "%0.2fWh", data.batteryCapacityWh);
    alignTextBothSides(leftPart, rightPart, buffer);
    lcd.print(buffer);
    clearDisplayBuffer(leftPart, rightPart);

    lcd.setCursor(0, 2);
    float power = abs(data.loadVoltage * data.loadCurrent);
    sprintf(leftPart, "%0.2fW", power);
    alignTextBothSides(leftPart, desiredLoadText(data), buffer);
    lcd.print(buffer);
    clearDisplayBuffer(leftPart, rightPart);

    lcd.setCursor(0, 3);
    sprintf(leftPart, "Output:%s", boolToString(data.outputEnabled));
    sprintf(rightPart, "%d%cC", data.temperature, (char)223);
    alignTextBothSides(leftPart, rightPart, buffer);
    lcd.print(buffer);
    clearDisplayBuffer(leftPart, rightPart);

    lcd.setCursor(data.cursorPosition, 2);
    lcd.cursor();
}

void LcdModule::printMenu(const ElectronicLoadData &data) {
    lcd.setCursor(0, 0);
    lcd.print("Battery: ");
    sprintf(buffer, "%s    ", data.batteryType.name);
    lcd.print(buffer);

    lcd.setCursor(0, 1);
    lcd.print("4-wire: ");
    sprintf(buffer, "%s ", boolToString(data.kelvinConnection));
    lcd.print(buffer);

    lcd.setCursor(0, 2);
    sprintf(buffer, "Ucutoff: %0.3fV ", data.cutoffVoltage);
    lcd.print(buffer);

    lcd.setCursor(data.cursorPosition, data.menuItemIndex);
    lcd.cursor();
}

void LcdModule::updateLcd(const ElectronicLoadData &data, bool force) {
    uint16_t ms = millis();
    if (force || (uint16_t)(ms - lcdTimer) >= LCD_UPDATE_PERIOD) {
        lcdTimer = ms;

        if (data.menuEnabled) {
            printMenu(data);
        } else {
            printMainDisplay(data);
        }
    }
}

void LcdModule::clear() {
    lcd.clear();
    lcd.setCursor(0, 0);
}

void LcdModule::printInitScreen() {
    lcd.setCursor(0, 0);
    lcd.print("DC Load");

    lcd.setCursor(0, 1);
    lcd.print(WiFi.localIP());

    lcd.setCursor(0, 2);
    lcd.print("RSSI: ");
    lcd.print(WiFi.RSSI());
}