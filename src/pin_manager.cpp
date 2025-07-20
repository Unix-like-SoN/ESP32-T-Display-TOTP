#include "pin_manager.h"
#include <FS.h>
#include "config.h"
#include "crypto_manager.h"
#include <ArduinoJson.h>
#include "LittleFS.h"

PinManager::PinManager(DisplayManager& display) : displayManager(display) {
    // Конструктор пуст
}

void PinManager::begin() {
    loadPinConfig();
}

void PinManager::loadPinConfig() {
    if (LittleFS.exists(PIN_FILE)) {
        fs::File configFile = LittleFS.open(PIN_FILE, "r");
        if (configFile) {
            JsonDocument doc;
            if (deserializeJson(doc, configFile) == DeserializationError::Ok) {
                enabled = doc["enabled"] | false;
                pinHash = doc["hash"].as<String>();
                currentPinLength = doc["length"] | DEFAULT_PIN_LENGTH;
            }
            configFile.close();
        }
    }
}

void PinManager::savePinConfig() {
    JsonDocument doc;
    doc["enabled"] = enabled;
    doc["hash"] = pinHash;
    doc["length"] = currentPinLength;

    fs::File configFile = LittleFS.open(PIN_FILE, "w");
    if (configFile) {
        serializeJson(doc, configFile);
        configFile.close();
    }
}

void PinManager::updatePinScreen(int currentPosition, int currentDigit, const String& enteredPin) {
    TFT_eSPI* tft = displayManager.getTft();
    int centerX = tft->width() / 2;

    // Обновляем только маску PIN-кода
    String pinMask = "";
    for (int i = 0; i < enteredPin.length(); i++) pinMask += "*";
    for (int i = 0; i < (currentPinLength - enteredPin.length()); i++) pinMask += ".";
    
    tft->setTextDatum(MC_DATUM);
    tft->setTextSize(3);
    // Очищаем предыдущую маску и рисуем новую
    tft->fillRect(0, 50, tft->width(), 24, TFT_BLACK); // Adjusted Y for clearing
    tft->drawString(pinMask, centerX, 60); // Adjusted Y for better vertical centering

    // Обновляем только селектор цифр
    String selector = "< " + String(currentDigit) + " >";
    tft->setTextSize(2);
    // Очищаем предыдущий селектор и рисуем новый
    tft->fillRect(0, 85, tft->width(), 16, TFT_BLACK); // Adjusted Y for clearing
    tft->drawString(selector, centerX, 95); // Adjusted Y for better vertical centering
}

void PinManager::drawPinScreen() {
    TFT_eSPI* tft = displayManager.getTft();
    tft->fillScreen(TFT_BLACK);
    tft->setTextDatum(MC_DATUM); // Устанавливаем выравнивание по центру

    int centerX = tft->width() / 2;

    // Рисуем заголовок
    tft->setTextSize(2);
    tft->setTextColor(TFT_WHITE, TFT_BLACK);
    tft->drawString("Enter PIN Code", centerX, 25); // Adjusted Y for better spacing
}

void PinManager::requestPin() {
    if (!enabled || !isPinSet()) {
        return;
    }

    String enteredPin = "";
    int currentDigit = 0;
    unsigned long lastButtonPress = 0;
    const int debounce = 250;
    
    drawPinScreen(); // Начальная отрисовка (один раз)
    updatePinScreen(enteredPin.length(), currentDigit, enteredPin); // Первоначальное отображение маски и селектора

    while (true) {
        // Кнопка 1 (пин 35) - переключение цифры
        if (digitalRead(BUTTON_1) == LOW && (millis() - lastButtonPress > debounce)) {
            lastButtonPress = millis();
            currentDigit = (currentDigit + 1) % 10;
            updatePinScreen(enteredPin.length(), currentDigit, enteredPin); // Обновляем только изменяемые части
        }

        // Кнопка 2 (пин 0) - подтверждение цифры
        if (digitalRead(BUTTON_2) == LOW && (millis() - lastButtonPress > debounce)) {
            lastButtonPress = millis();
            enteredPin += String(currentDigit);
            currentDigit = 0;
            updatePinScreen(enteredPin.length(), currentDigit, enteredPin); // Обновляем маску

            if (enteredPin.length() >= currentPinLength) {
                TFT_eSPI* tft = displayManager.getTft();
                int centerX = tft->width() / 2;
                tft->setTextDatum(MC_DATUM);

                if (checkPin(enteredPin)) {
                    tft->fillScreen(TFT_BLACK);
                    tft->setTextSize(3);
                    tft->drawString("PIN OK", centerX, 67); // Centered vertically
                    delay(1000);
                    return;
                } else {
                    tft->fillScreen(TFT_BLACK);
                    tft->setTextSize(2);
                    tft->setTextColor(TFT_RED);
                    tft->drawString("WRONG PIN", centerX, 67); // Centered vertically
                    delay(2000);
                    
                    // Сбрасываем для повторного ввода
                    enteredPin = "";
                    drawPinScreen(); // Перерисовываем экран после ошибки
                    updatePinScreen(enteredPin.length(), currentDigit, enteredPin);
                }
            }
        }
        delay(50);
    }
}

void PinManager::setPin(const String& newPin) {
    if (newPin.length() > 0) {
        pinHash = CryptoManager::hashPassword(newPin);
    }
}

void PinManager::setEnabled(bool newStatus) {
    enabled = newStatus;
}

int PinManager::getPinLength() {
    return currentPinLength;
}

void PinManager::setPinLength(int newLength) {
    if (newLength >= 4 && newLength <= MAX_PIN_LENGTH) {
        currentPinLength = newLength;
    }
}

void PinManager::saveConfig() {
    savePinConfig();
}

bool PinManager::isPinEnabled() {
    return enabled;
}

bool PinManager::isPinSet() {
    return pinHash.length() > 0;
}

bool PinManager::checkPin(const String& pin) {
    return CryptoManager::verifyPassword(pin, pinHash);
}
