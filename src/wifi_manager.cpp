#include <WiFi.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include "wifi_manager.h"

WifiManager::WifiManager(DisplayManager& display) : _display(display) {}

bool WifiManager::loadCredentials(String& ssid, String& password) {
    if (!LittleFS.exists("/wifi_config.json")) {
        return false;
    }
    File configFile = LittleFS.open("/wifi_config.json", "r");
    if (!configFile) return false;

    JsonDocument doc;
    if (deserializeJson(doc, configFile)) {
        configFile.close();
        return false;
    }
    configFile.close();

    ssid = doc["ssid"].as<String>();
    password = doc["password"].as<String>();
    return ssid.length() > 0;
}

bool WifiManager::connect() {
    String ssid, password;
    
    _display.showMessage("Loading WiFi config...", 10, 50);
    delay(500);

    if (!loadCredentials(ssid, password)) {
        _display.showMessage("No WiFi config found.", 10, 50, true);
        delay(1500);
        return false;
    }

    _display.showMessage("Config loaded.", 10, 50);
    _display.showMessage("Connecting to:", 10, 70);
    _display.showMessage(ssid, 10, 90, false, 2);

    // --- Улучшенная логика подключения ---
    WiFi.mode(WIFI_STA); // Явно устанавливаем режим клиента
    WiFi.disconnect(true); // Очищаем предыдущую сессию
    delay(100);

    WiFi.begin(ssid.c_str(), password.c_str());

    int attempts = 0;
    // Увеличиваем время ожидания до 20 секунд (40 * 500 мс)
    while (WiFi.status() != WL_CONNECTED && attempts < 40) {
        delay(500);
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        _ipAddress = WiFi.localIP().toString();
        return true; // Успех!
    }
    
    // Если не удалось, показываем ошибку и возвращаем false
    _display.init();
    _display.showMessage("Connection Failed!", 10, 50, true);
    _display.showMessage("Check credentials.", 10, 70);
    delay(2500);
    WiFi.disconnect();
    return false; // Неудача
}

void WifiManager::startConfigPortal() {
    const char* ap_ssid = "ESP32-TOTP-Setup";
    _display.init();
    _display.showMessage("WiFi Setup Mode", 10, 10, false, 2);
    _display.showMessage("1. Connect to WiFi:", 10, 40);
    _display.showMessage(ap_ssid, 15, 60, false, 2);
    _display.showMessage("2. Go to 192.168.4.1", 10, 90);

    WiFi.softAP(ap_ssid);
}

String WifiManager::getIP() {
    return _ipAddress;
}