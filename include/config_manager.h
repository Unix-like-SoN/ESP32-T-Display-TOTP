#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <Arduino.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include "config.h"
#include "ui_themes.h"

class ConfigManager {
public:
    ConfigManager();
    void begin();
    Theme loadTheme();
    void saveTheme(Theme theme);

private:
    // Internal state for configuration values
    Theme _currentTheme = Theme::DARK; // Default theme
};

#endif // CONFIG_MANAGER_H
