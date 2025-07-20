#include "web_server.h"
#include <ArduinoJson.h>
#include "config.h"
#include <FS.h>
#include "LittleFS.h"
#include "WiFi.h"
#include "totp_generator.h"
#include "crypto_manager.h"
#include "web_pages/page_login.h"
#include "web_pages/page_index.h"
#include "web_pages/page_wifi_setup.h"
#include "web_pages/page_pin_settings.h"

AsyncWebServer server(WEB_SERVER_PORT);
KeyManager* pKeyManager;
SplashScreenManager* pSplashManager;
DisplayManager* pDisplayManager;
PinManager* pPinManager;
ConfigManager* pConfigManager; // New: Global pointer to ConfigManager
TOTPGenerator webTotpGenerator;

WebServerManager::WebServerManager(KeyManager& keyManager, SplashScreenManager& splashManager, DisplayManager& displayManager, PinManager& pinManager, ConfigManager& configManager) {
    pKeyManager = &keyManager;
    pSplashManager = &splashManager;
    pDisplayManager = &displayManager;
    pPinManager = &pinManager;
    pConfigManager = &configManager; // Initialize new pointer
    session_created_time = 0;
}

String WebServerManager::getAdminPasswordHash() {
    if (LittleFS.exists("/auth.json")) {
        fs::File authFile = LittleFS.open("/auth.json", "r");
        if (authFile) {
            JsonDocument doc;
            deserializeJson(doc, authFile);
            authFile.close();
            if (!doc["password_hash"].isNull()) {
                return doc["password_hash"].as<String>();
            }
        }
    }
    return CryptoManager::hashPassword(ADMIN_PASSWORD);
}

// --- УЛУЧШЕННАЯ ЛОГИКА АУТЕНТИФИКАЦИИ НА ОСНОВЕ СЕССИИ ---
bool WebServerManager::isAuthenticated(AsyncWebServerRequest *request) {
    // Проверка тайм-аута сессии
    if (session_id.length() > 0 && (millis() - session_created_time > SESSION_TIMEOUT)) {
        session_id = "";
        session_created_time = 0;
    }
    
    // Проверка Cookie
    if (request->hasHeader("Cookie")) {
        String cookie = request->getHeader("Cookie")->value();
        int sessionPos = cookie.indexOf("session=");
        if (sessionPos != -1) {
            int sessionStart = sessionPos + 8;
            int sessionEnd = cookie.indexOf(";", sessionStart);
            if (sessionEnd == -1) sessionEnd = cookie.length();
            
            String sessionFromCookie = cookie.substring(sessionStart, sessionEnd);
            if (sessionFromCookie.length() > 0 && sessionFromCookie.equals(session_id)) {
                return true;
            }
        }
    }
    
    // Проверка Authorization header (для загрузки файлов)
    if (request->hasHeader("Authorization")) {
        String auth = request->getHeader("Authorization")->value();
        if (auth.startsWith("Bearer ")) {
            String token = auth.substring(7);
            if (token.length() > 0 && token.equals(session_id)) {
                return true;
            }
        }
    }
    
    return false;
}

void WebServerManager::start() {
    // --- ЭНДПОИНТЫ, НЕ ТРЕБУЮЩИЕ АУТЕНТИФИКАЦИИ ---

    // Страница входа
    server.on("/login", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send_P(200, "text/html", login_html);
    });

    // Обработка данных формы входа
    server.on("/login", HTTP_POST, [this](AsyncWebServerRequest *request){
        if (request->hasParam("username", true) && request->hasParam("password", true)) {
            String username = request->getParam("username", true)->value();
            String password = request->getParam("password", true)->value();
            
            if (username.equals(ADMIN_USERNAME) && CryptoManager::verifyPassword(password, getAdminPasswordHash())) {
                // Генерируем новую сессию
                session_id = String(random(0, 0x7FFFFFFF), HEX) + String(random(0, 0x7FFFFFFF), HEX);
                session_created_time = millis();
                
                AsyncWebServerResponse *response = request->beginResponse(302, "text/plain", "Found");
                response->addHeader("Location", "/");
                response->addHeader("Set-Cookie", "session=" + session_id + "; Path=/; HttpOnly");
                request->send(response);
                return;
            }
        }
        // Неверные данные: перенаправляем обратно на страницу входа с ошибкой
        AsyncWebServerResponse *response = request->beginResponse(302, "text/plain", "Found");
        response->addHeader("Location", "/login?error=1");
        request->send(response);
    });

    // --- ЭНДПОИНТЫ, ТРЕБУЮЩИЕ АУТЕНТИФИКАЦИИ ---

    // Главная страница
    server.on("/", HTTP_GET, [this](AsyncWebServerRequest *request){
        if (!isAuthenticated(request)) {
            request->redirect("/login");
            return;
        }
        request->send_P(200, "text/html", index_html);
    });
    
    // Выход из системы
    server.on("/logout", HTTP_GET, [this](AsyncWebServerRequest *request){
        session_id = ""; // Сбрасываем сессию
        session_created_time = 0;
        request->redirect("/login");
    });

    // Все API эндпоинты теперь проверяют сессию
    server.on("/api/change_password", HTTP_POST, [this](AsyncWebServerRequest *request) {
        if (!isAuthenticated(request)) return request->send(401);
        if (request->hasParam("password", true)) {
            String newPassword = request->getParam("password", true)->value();
            String newHash = CryptoManager::hashPassword(newPassword);
            fs::File authFile = LittleFS.open("/auth.json", "w");
            JsonDocument doc;
            doc["password_hash"] = newHash;
            serializeJson(doc, authFile);
            authFile.close();
            request->send(200, "text/plain", "Password changed successfully!");
        } else {
            request->send(400, "text/plain", "Password parameter missing.");
        }
    });

    // --- ИСПРАВЛЕННЫЙ ОБРАБОТЧИК ЗАГРУЗКИ SPLASH ---
    server.on("/api/upload_splash", HTTP_POST,
        [this](AsyncWebServerRequest *request){
            if (!isAuthenticated(request)) {
                return request->send(401, "text/plain", "Unauthorized");
            }
            request->send(200, "text/plain", "Splash image uploaded successfully!");
        },
        [this](AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data, size_t len, bool is_final){
            // Проверяем аутентификацию на каждом чанке
            if (!isAuthenticated(request)) {
                Serial.println("Upload chunk rejected: not authenticated");
                return;
            }
            
            static fs::File splashFile;
            static bool uploadError = false;
            
            if(index == 0){
                uploadError = false;
                Serial.println("Starting splash upload: " + filename);
                splashFile = LittleFS.open(SPLASH_IMAGE_PATH, "w");
                if(!splashFile){ 
                    Serial.println("Failed to open splash file for writing");
                    uploadError = true;
                    return; 
                }
            }
            
            if(len > 0 && splashFile && !uploadError){ 
                size_t written = splashFile.write(data, len);
                if (written != len) {
                    Serial.println("Write error during upload");
                    uploadError = true;
                }
            }
            
            if(is_final){ 
                if(splashFile) {
                    splashFile.close();
                    if (!uploadError) {
                        Serial.println("Splash upload completed successfully");
                    } else {
                        Serial.println("Splash upload completed with errors");
                        LittleFS.remove(SPLASH_IMAGE_PATH); // Удаляем поврежденный файл
                    }
                }
            }
        }
    );

    server.on("/api/delete_splash", HTTP_POST, [this](AsyncWebServerRequest *request){
        if (!isAuthenticated(request)) return request->send(401);
        if (pSplashManager->deleteSplashImage()) {
            request->send(200, "text/plain", "Splash image deleted.");
        } else {
            request->send(500, "text/plain", "Failed to delete splash image.");
        }
    });

    server.on("/api/keys", HTTP_GET, [this](AsyncWebServerRequest *request){
        if (!isAuthenticated(request)) return request->send(401);
        auto keys = pKeyManager->getAllKeys();
        JsonDocument doc;
        JsonArray array = doc.to<JsonArray>();
        for (const auto& key : keys) {
            JsonObject obj = array.add<JsonObject>();
            obj["name"] = key.name;
            obj["code"] = webTotpGenerator.generateTOTP(key.secret);
            obj["timeLeft"] = webTotpGenerator.getTimeRemaining();
        }
        String output;
        serializeJson(doc, output);
        request->send(200, "application/json", output);
    });

    server.on("/api/add", HTTP_POST, [this](AsyncWebServerRequest *request){
        if (!isAuthenticated(request)) return request->send(401);
        if (request->hasParam("name", true) && request->hasParam("secret", true)) {
            pKeyManager->addKey(request->getParam("name", true)->value(), request->getParam("secret", true)->value());
            request->send(200);
        } else { request->send(400); }
    });

    server.on("/api/remove", HTTP_POST, [this](AsyncWebServerRequest *request){
        if (!isAuthenticated(request)) return request->send(401);
        if (request->hasParam("index", true)) {
            pKeyManager->removeKey(request->getParam("index", true)->value().toInt());
            request->send(200);
        } else { request->send(400); }
    });

    server.on("/api/export", HTTP_GET, [this](AsyncWebServerRequest *request){
        if (!isAuthenticated(request)) return request->send(401);
        auto keys = pKeyManager->getAllKeys();
        JsonDocument doc;
        JsonArray array = doc.to<JsonArray>();
        for (const auto& key : keys) {
            JsonObject obj = array.add<JsonObject>();
            obj["name"] = key.name;
            obj["secret"] = key.secret;
        }
        String output;
        serializeJson(doc, output);
        AsyncWebServerResponse *response = request->beginResponse(200, "application/json", output);
        response->addHeader("Content-Disposition", "attachment; filename=\"keys_backup.json\"");
        request->send(response);
    });

    server.on("/api/import", HTTP_POST,
        [this](AsyncWebServerRequest *request){
            if (!isAuthenticated(request)) return request->send(401);
            request->send(200);
        },
        [this](AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data, size_t len, bool is_final){
            if (!isAuthenticated(request)) return;
            static String content;
            if(index == 0) content = "";
            for(size_t i=0; i<len; i++) content += (char)data[i];
            if(is_final) {
                if(!pKeyManager->replaceAllKeys(content)) {
                    Serial.println("Import failed!");
                }
            }
        }
    );

    server.on("/pin", HTTP_GET, [this](AsyncWebServerRequest *request){
        if (!isAuthenticated(request)) {
            request->redirect("/login");
            return;
        }
        request->send_P(200, "text/html", pin_settings_html);
    });

    server.on("/api/pincode_settings", HTTP_GET, [this](AsyncWebServerRequest *request){
        if (!isAuthenticated(request)) return request->send(401);
        pPinManager->loadPinConfig();
        JsonDocument doc;
        doc["enabled"] = pPinManager->isPinEnabled();
        doc["length"] = pPinManager->getPinLength();
        String output;
        serializeJson(doc, output);
        request->send(200, "application/json", output);
    });

    server.on("/api/pincode_settings", HTTP_POST, [this](AsyncWebServerRequest *request){
        if (!isAuthenticated(request)) return request->send(401);
        
        bool enabled = request->hasParam("enabled", true) && (request->getParam("enabled", true)->value() == "true");
        pPinManager->setEnabled(enabled);

        if (request->hasParam("length", true)) {
            pPinManager->setPinLength(request->getParam("length", true)->value().toInt());
        }

        if (enabled) {
            if (request->hasParam("pin", true) && request->hasParam("pin_confirm", true)) {
                String pin = request->getParam("pin", true)->value();
                String pin_confirm = request->getParam("pin_confirm", true)->value();

                if (pin.length() > 0) {
                    if (pin.length() < 4 || pin.length() > MAX_PIN_LENGTH) {
                        return request->send(400, "text/plain", "PIN must be between 4 and 10 digits.");
                    }
                    if (pin != pin_confirm) {
                        return request->send(400, "text/plain", "PINs do not match.");
                    }
                    pPinManager->setPin(pin);
                }
            }
            if (!pPinManager->isPinSet()) {
                pPinManager->setEnabled(false);
                pPinManager->saveConfig();
                return request->send(400, "text/plain", "Cannot enable PIN protection without setting a PIN first.");
            }
        }

        pPinManager->saveConfig();
        request->send(200, "text/plain", "Settings saved successfully!");
    });

    // API to get current theme
    server.on("/api/theme", HTTP_GET, [this](AsyncWebServerRequest *request){
        if (!isAuthenticated(request)) return request->send(401);
        Theme currentTheme = pConfigManager->loadTheme();
        String themeName = (currentTheme == Theme::LIGHT) ? "light" : "dark"; // Use lowercase for consistency with JS
        JsonDocument doc;
        doc["theme"] = themeName;
        String output;
        serializeJson(doc, output);
        request->send(200, "application/json", output);
    });

    // API to set theme
    server.on("/api/theme", HTTP_POST, [this](AsyncWebServerRequest *request){
        if (!isAuthenticated(request)) return request->send(401);
        Serial.println("Received POST /api/theme request.");
        if (request->hasParam("theme", true)) {
            String themeStr = request->getParam("theme", true)->value();
            Serial.println("Requested theme: " + themeStr);
            Theme newTheme = Theme::DARK;
            if (themeStr == "light") { // Use lowercase "light"
                newTheme = Theme::LIGHT;
            }
            Serial.println("Calling pConfigManager->saveTheme()...");
            pConfigManager->saveTheme(newTheme);
            Serial.println("pConfigManager->saveTheme() returned. Calling pDisplayManager->setTheme()...");
            pDisplayManager->setTheme(newTheme);
            Serial.println("pDisplayManager->setTheme() returned. Sending response.");
            request->send(200, "text/plain", "Theme updated successfully!");
        } else {
            Serial.println("Theme parameter missing in POST /api/theme.");
            request->send(400, "text/plain", "Theme parameter missing.");
        }
    });

    server.on("/api/reboot", HTTP_POST, [this](AsyncWebServerRequest *request){
        if (!isAuthenticated(request)) return request->send(401);
        request->send(200, "text/plain", "Rebooting...");
        delay(1000);
        ESP.restart();
    });

    server.begin();
}

void WebServerManager::startConfigServer() {
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", wifi_setup_html);
        response->addHeader("Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");
        response->addHeader("Pragma", "no-cache");
        request->send(response);
    });
    server.on("/scan", HTTP_GET, [](AsyncWebServerRequest *request){
        int n = WiFi.scanNetworks();
        JsonDocument doc;
        JsonArray array = doc.to<JsonArray>();
        for (int i = 0; i < n; ++i) {
            JsonObject net = array.add<JsonObject>();
            net["ssid"] = WiFi.SSID(i);
            net["rssi"] = WiFi.RSSI(i);
        }
        String output;
        serializeJson(doc, output);
        request->send(200, "application/json", output);
    });
    server.on("/save", HTTP_POST, [](AsyncWebServerRequest *request){
        String ssid = request->arg("ssid");
        String password = request->arg("password");
        JsonDocument doc;
        doc["ssid"] = ssid;
        doc["password"] = password;
        fs::File configFile = LittleFS.open("/wifi_config.json", "w");
        serializeJson(doc, configFile);
        configFile.close();
        request->send(200, "text/plain", "Credentials saved. Rebooting...");
        delay(1000);
        ESP.restart();
    });
    server.begin();
}

void WebServerManager::stop() {
    server.end();
}