#ifndef WEBSERVER_MANAGER_H
#define WEBSERVER_MANAGER_H

#include <ESPAsyncWebServer.h>
#include "key_manager.h"
#include "splash_manager.h"
#include "display_manager.h"
#include "pin_manager.h"
#include "config_manager.h" // New: Include ConfigManager

class WebServerManager {
public:
    WebServerManager(KeyManager& keyManager, SplashScreenManager& splashManager, DisplayManager& displayManager, PinManager& pinManager, ConfigManager& configManager); // Added ConfigManager
    void start();
    void stop();
    void startConfigServer();

private:
    String getAdminPasswordHash();
    bool isAuthenticated(AsyncWebServerRequest *request);
    String session_id = ""; // Хранилище для ID сессии
    unsigned long session_created_time = 0; // <-- ДОБАВИТЬ ЭТУ СТРОКУ
    static const unsigned long SESSION_TIMEOUT = 3600000; // <-- ДОБАВИТЬ ЭТУ СТРОКУ (1 час в миллисекундах)
};

#endif // WEBSERVER_MANAGER_H

