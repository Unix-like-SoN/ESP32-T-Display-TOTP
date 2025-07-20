#include <Arduino.h>
#include "config.h"
#include "wifi_manager.h"
#include "display_manager.h"
#include "key_manager.h"
#include "web_server.h"
#include "totp_generator.h"
#include "LittleFS.h"
#include "esp_sleep.h"
#include "splash_manager.h"
#include "pin_manager.h"
#include "battery_manager.h"
#include "config_manager.h" // New: Include ConfigManager

#ifndef LED_BUILTIN
#define LED_BUILTIN 2 // Стандартный пин для ESP32, если не определен
#endif

// Глобальные объекты менеджеров
DisplayManager displayManager;
KeyManager keyManager;
SplashScreenManager splashManager(displayManager);
PinManager pinManager(displayManager);
BatteryManager batteryManager(34, 14); // Используем пин 34 для АЦП и 14 для питания
WifiManager wifiManager(displayManager); 
ConfigManager configManager; // New: Global ConfigManager object
WebServerManager webServerManager(keyManager, splashManager, displayManager, pinManager, configManager);
TOTPGenerator totpGenerator;

// Глобальные переменные состояния
static int currentKeyIndex = 0;
static int previousKeyIndex = -1;
unsigned long lastButtonPressTime = 0; 
const int debounceDelay = 300; 
const int factoryResetHoldTime = 5000;
const int powerOffHoldTime = 5000;
unsigned long lastActivityTime = 0;
const int screenTimeout = 30000;
bool isScreenOn = true;
bool isWebServerRunning = false;

// Для обновления батареи
unsigned long lastBatteryCheckTime = 0;
const int batteryCheckInterval = 1000; // <-- Уменьшено до 1 секунды для быстрой реакции
static int lastBatteryPercentage = -1; // <-- ВОССТАНОВЛЕНО

// Для обновления TOTP
unsigned long lastTotpUpdateTime = 0;
const int totpUpdateInterval = 250; // Обновляем каждые 250 мс


void handleFactoryResetOnBoot() {
    displayManager.init();
    displayManager.showMessage("Hold both buttons", 10, 20, false, 2);
    displayManager.showMessage("for factory reset.", 10, 40, false, 2);
    
    unsigned long startTime = millis();
    
    while(digitalRead(BUTTON_1) == LOW && digitalRead(BUTTON_2) == LOW) {
        unsigned long holdTime = millis() - startTime;
        
        if (holdTime > factoryResetHoldTime) {
            displayManager.init();
            displayManager.showMessage("FACTORY RESET!", 10, 30, true, 2);
            
            LittleFS.remove(KEYS_FILE);
            LittleFS.remove("/wifi_config.json");
            LittleFS.remove(SPLASH_IMAGE_PATH);
            LittleFS.remove("/auth.json");
            LittleFS.remove(PIN_FILE);
            LittleFS.remove(CONFIG_FILE); // New: Remove config file on factory reset
            displayManager.showMessage("Done. Rebooting...", 10, 60);
            
            delay(2500);
            ESP.restart();
        }
        
        int progress = (holdTime * 100) / factoryResetHoldTime;
        displayManager.showMessage("Resetting: " + String(progress) + "%", 10, 100);
        delay(100);
    }
    displayManager.init();
}

void setup() {
    Serial.begin(115200);
    pinMode(BUTTON_1, INPUT_PULLUP);
    pinMode(BUTTON_2, INPUT_PULLUP);

    // 1. Инициализация файловой системы и менеджеров
    batteryManager.begin();
    if (!LittleFS.begin(true)) {
        DisplayManager tempDisplay;
        tempDisplay.init();
        tempDisplay.showMessage("LittleFS Failed", 10, 30, true);
        while(1);
    }

    // Load theme before displayManager.init() to ensure correct colors from start
    Theme savedTheme = configManager.loadTheme();
    displayManager.setTheme(savedTheme);

    displayManager.init();
    keyManager.begin();
    pinManager.begin();
    
    // 2. Проверка на сброс к заводским настройкам
    if (digitalRead(BUTTON_1) == LOW && digitalRead(BUTTON_2) == LOW) {
        handleFactoryResetOnBoot();
    }

    // 3. Показ сплэш-скрина
    splashManager.displaySplashScreen();
    
    // 4. Запрос ПИН-кода
    pinManager.requestPin();
    
    // 5. Подключение к WiFi
    displayManager.init(); // Очищаем экран после "PIN OK"
    displayManager.showMessage("Initializing...", 10, 10);
    if (!wifiManager.connect()) {
        wifiManager.startConfigPortal();
        webServerManager.startConfigServer();
        isWebServerRunning = true; // Сервер запущен в режиме конфигурации
        while(1) { delay(100); }
    }

    displayManager.init();
    displayManager.showMessage("WiFi Connected!", 10, 50);
    displayManager.showMessage(wifiManager.getIP(), 10, 70);
    delay(1500); // Даем пользователю время увидеть IP

    // 6. Синхронизация времени с несколькими попытками
    configTime(0, 0, "pool.ntp.org");
    struct tm timeinfo;
    bool timeSynced = false;
    for (int i = 0; i < 3; i++) {
        displayManager.init();
        displayManager.showMessage("Time Sync... (" + String(i + 1) + "/3)", 10, 30, false, 2);
        if (getLocalTime(&timeinfo, 5000)) { // Таймаут 5 секунд на попытку
            timeSynced = true;
            displayManager.init();
            displayManager.showMessage("Time Synced!", 10, 50, false, 2);
            delay(1000);
            break;
        }
    }

    if (!timeSynced) {
        displayManager.init();
        displayManager.showMessage("ERROR:", 10, 20, true, 2);
        displayManager.showMessage("Time sync failed!", 10, 40, false, 2);
        delay(3000);
        ESP.restart();
    }
    
    // 7. Запуск основного веб-сервера
    webServerManager.start();
    isWebServerRunning = true; // Устанавливаем флаг, что сервер запущен
    lastActivityTime = millis();
}

void handleButtons() {
    static unsigned long button1PressStartTime = 0;
    static unsigned long button2PressStartTime = 0;
    bool buttonPressed = false;

    // --- Логика для Кнопки 1 (GPIO 35) ---
    if (digitalRead(BUTTON_1) == LOW) {
        if (button1PressStartTime == 0) { // Нажата только что
            button1PressStartTime = millis();
        } else if (isWebServerRunning && (millis() - button1PressStartTime > powerOffHoldTime)) {
            // Длительное нажатие: выключить веб-сервер
            webServerManager.stop();
            isWebServerRunning = false;
            displayManager.init();
            TFT_eSPI* tft = displayManager.getTft();
            tft->setTextDatum(MC_DATUM);
            tft->drawString("Web Server OFF", tft->width() / 2, tft->height() / 2);
            delay(2000);
            button1PressStartTime = 0; // Сбрасываем, чтобы избежать повторного срабатывания
            previousKeyIndex = -1; // Принудительное обновление экрана
        }
    } else {
        if (button1PressStartTime > 0) { // Была отпущена
            // Короткое нажатие: переключить ключ
            if (millis() - button1PressStartTime < powerOffHoldTime) {
                 auto keys = keyManager.getAllKeys();
                if (!keys.empty()) {
                    currentKeyIndex = (currentKeyIndex == 0) ? keys.size() - 1 : currentKeyIndex - 1;
                    buttonPressed = true;
                }
            }
            button1PressStartTime = 0; // Сбрасываем таймер
        }
    }

    // --- Логика для Кнопки 2 (GPIO 0) ---
    if (digitalRead(BUTTON_2) == LOW) {
        if (button2PressStartTime == 0) { // Нажата только что
            button2PressStartTime = millis();
        } else if (millis() - button2PressStartTime > powerOffHoldTime) {
            // Длительное нажатие: выключить устройство
            displayManager.init();
            displayManager.showMessage("Shutting down...", 10, 30, false, 2);
            delay(1000);
            displayManager.turnOff();
            esp_deep_sleep_start();
        }
    }
    else {
        if (button2PressStartTime > 0) { // Была отпущена
            // Короткое нажатие: переключить ключ
            if (millis() - button2PressStartTime < powerOffHoldTime) {
                auto keys = keyManager.getAllKeys();
                if (!keys.empty()) {
                    currentKeyIndex = (currentKeyIndex + 1) % keys.size();
                    buttonPressed = true;
                }
            }
            button2PressStartTime = 0; // Сбрасываем таймер
        }
    }

    if (buttonPressed) {
        lastActivityTime = millis();
        if (!isScreenOn) {
            displayManager.turnOn();
            isScreenOn = true;
        }
        // Принудительное обновление экрана при нажатии
        previousKeyIndex = -1; 
    }
}

void loop() {
    displayManager.update(); // <-- ОБНОВЛЯЕМ АНИМАЦИИ
    handleButtons();

    if (isScreenOn && (millis() - lastActivityTime > screenTimeout)) {
        displayManager.turnOff();
        isScreenOn = false;
    }

    if (isScreenOn) {
        // Обновляем статус батареи по таймеру
        if (millis() - lastBatteryCheckTime > batteryCheckInterval) {
            lastBatteryCheckTime = millis();
            
            int currentBatteryPercentage = batteryManager.getPercentage();
            float voltage = batteryManager.getVoltage();
            bool isCharging = (voltage > 4.15); // Consider charging if voltage is above 4.15V 

            Serial.print("main.cpp - currentBatteryPercentage: ");
            Serial.print(currentBatteryPercentage);
            Serial.print("%, isCharging: ");
            Serial.println(isCharging ? "true" : "false");

            displayManager.updateBatteryStatus(currentBatteryPercentage, isCharging);
        }

        // Обновляем TOTP и прогресс-бар по таймеру
        if (millis() - lastTotpUpdateTime > totpUpdateInterval) {
            lastTotpUpdateTime = millis();
            auto keys = keyManager.getAllKeys();
            if (!keys.empty()) {
                if (currentKeyIndex != previousKeyIndex) {
                    // При смене ключа, просто сообщаем DisplayManager новое состояние
                    displayManager.drawLayout(keys[currentKeyIndex].name, batteryManager.getPercentage(), batteryManager.getVoltage() > 4.18);
                    previousKeyIndex = currentKeyIndex;
                }
                
                String code = totpGenerator.generateTOTP(keys[currentKeyIndex].secret);
                int timeLeft = totpGenerator.getTimeRemaining();
                displayManager.updateTOTPCode(code, timeLeft);

            } else {
                if (previousKeyIndex != -1) {
                    displayManager.init(); // Re-init to clear screen
                    previousKeyIndex = -1;
                }
                displayManager.showMessage("No keys found.", 10, 10);
                displayManager.showMessage("Add via web UI.", 10, 30);
            }
        }
    }
    
    // delay(250); // УДАЛЕНО ДЛЯ ПЛАВНОЙ АНИМАЦИИ
}