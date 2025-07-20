#include "config_manager.h"

ConfigManager::ConfigManager() {
    // Constructor
}

void ConfigManager::begin() {
    // No specific begin logic needed for now, LittleFS is initialized elsewhere
}

Theme ConfigManager::loadTheme() {
    Serial.println("ConfigManager::loadTheme() called.");
    if (LittleFS.exists(CONFIG_FILE)) {
        Serial.println("Config file exists. Opening...");
        fs::File configFile = LittleFS.open(CONFIG_FILE, "r");
        if (configFile) {
            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, configFile);
            if (error == DeserializationError::Ok) {
                String themeStr = doc[THEME_CONFIG_KEY] | "dark"; // Default to "dark"
                Serial.println("Loaded theme string: " + themeStr);
                if (themeStr == "light") {
                    _currentTheme = Theme::LIGHT;
                } else {
                    _currentTheme = Theme::DARK;
                }
                Serial.println("Current theme set to: " + String((_currentTheme == Theme::LIGHT) ? "LIGHT" : "DARK"));
            } else {
                Serial.println("Failed to deserialize config file: " + String(error.c_str()));
            }
            configFile.close();
        } else {
            Serial.println("Failed to open config file for reading.");
        }
    } else {
        Serial.println("Config file does not exist. Using default theme.");
    }
    return _currentTheme;
}

void ConfigManager::saveTheme(Theme theme) {
    Serial.println("ConfigManager::saveTheme() called with theme: " + String((theme == Theme::LIGHT) ? "LIGHT" : "DARK"));
    _currentTheme = theme;
    JsonDocument doc;

    // Load existing config to preserve other settings
    if (LittleFS.exists(CONFIG_FILE)) {
        Serial.println("Config file exists. Loading existing settings...");
        fs::File configFile = LittleFS.open(CONFIG_FILE, "r");
        if (configFile) {
            deserializeJson(doc, configFile);
            configFile.close();
            Serial.println("Existing config loaded.");
        } else {
            Serial.println("Failed to open config file for reading during save. Creating new doc.");
        }
    } else {
        Serial.println("Config file does not exist. Creating new doc.");
    }

    doc[THEME_CONFIG_KEY] = (theme == Theme::LIGHT) ? "light" : "dark"; // Use lowercase

    fs::File configFile = LittleFS.open(CONFIG_FILE, "w");
    if (configFile) {
        serializeJson(doc, configFile);
        configFile.close();
        Serial.println("Theme saved successfully to config file.");
    } else {
        Serial.println("Failed to open config file for writing.");
    }
}
