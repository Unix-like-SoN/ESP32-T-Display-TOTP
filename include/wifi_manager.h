#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include "display_manager.h"

class WifiManager {
public:
    // Передаем DisplayManager для вывода статуса
    WifiManager(DisplayManager& display);
    // Основная функция подключения. Возвращает true, если удалось подключиться.
    bool connect(); 
    // Запускает портал настройки
    void startConfigPortal();
    String getIP();

private:
    bool loadCredentials(String& ssid, String& password);
    void saveCredentials(const String& ssid, const String& password);
    
    DisplayManager& _display;
    String _ipAddress;
};

#endif // WIFI_MANAGER_H
