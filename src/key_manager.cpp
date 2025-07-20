#include "key_manager.h"
#include "config.h"
#include <LittleFS.h>
#include <ArduinoJson.h>
#include "mbedtls/aes.h"
#include "mbedtls/sha256.h"
#include <esp_system.h>

KeyManager::KeyManager() {}

bool KeyManager::begin() {
    return loadKeys();
}

bool KeyManager::addKey(const String& name, const String& secret) {
    for (const auto& key : keys) {
        if (key.name == name) return false;
    }
    keys.push_back({name, secret});
    return saveKeys();
}

bool KeyManager::removeKey(int index) {
    if (index < 0 || index >= keys.size()) return false;
    keys.erase(keys.begin() + index);
    return saveKeys();
}

std::vector<TOTPKey> KeyManager::getAllKeys() {
    return keys;
}

// --- Новая функция для импорта ---
bool KeyManager::replaceAllKeys(const String& jsonContent) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, jsonContent);
    if (error) {
        Serial.print("Import failed, invalid JSON: ");
        Serial.println(error.c_str());
        return false;
    }

    keys.clear();
    JsonArray array = doc.as<JsonArray>();
    for (JsonObject obj : array) {
        keys.push_back({
            obj["name"].as<String>(),
            obj["secret"].as<String>()
        });
    }

    // Сохраняем новый набор ключей, который будет автоматически зашифрован
    return saveKeys();
}


void KeyManager::generateDeviceKey(unsigned char* key) {
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    
    mbedtls_sha256_context ctx;
    mbedtls_sha256_init(&ctx);
    mbedtls_sha256_starts(&ctx, 0);
    mbedtls_sha256_update(&ctx, mac, 6);
    mbedtls_sha256_update(&ctx, (const unsigned char*)ADMIN_PASSWORD, strlen(ADMIN_PASSWORD));
    mbedtls_sha256_finish(&ctx, key);
    mbedtls_sha256_free(&ctx);
}

// --- Новые, надежные ф��нкции шифрования с PKCS7 padding ---

bool KeyManager::encryptData(const uint8_t* plain, size_t plain_len, std::vector<uint8_t>& output) {
    unsigned char key[32];
    generateDeviceKey(key);
    
    mbedtls_aes_context aes;
    mbedtls_aes_init(&aes);
    mbedtls_aes_setkey_enc(&aes, key, 256);

    // PKCS7 Padding
    size_t padding_len = 16 - (plain_len % 16);
    size_t padded_len = plain_len + padding_len;
    
    std::vector<uint8_t> padded_input(padded_len);
    memcpy(padded_input.data(), plain, plain_len);
    for(size_t i = 0; i < padding_len; i++) {
        padded_input[plain_len + i] = padding_len;
    }

    output.resize(padded_len);
    for (size_t i = 0; i < padded_len; i += 16) {
        mbedtls_aes_crypt_ecb(&aes, MBEDTLS_AES_ENCRYPT, padded_input.data() + i, output.data() + i);
    }
    
    mbedtls_aes_free(&aes);
    return true;
}

bool KeyManager::decryptData(const uint8_t* encrypted, size_t encrypted_len, std::vector<uint8_t>& output) {
    if (encrypted_len % 16 != 0) return false; // Зашифрованные данные должны быть кратны 16

    unsigned char key[32];
    generateDeviceKey(key);

    mbedtls_aes_context aes;
    mbedtls_aes_init(&aes);
    mbedtls_aes_setkey_dec(&aes, key, 256);

    std::vector<uint8_t> decrypted_padded(encrypted_len);
    for (size_t i = 0; i < encrypted_len; i += 16) {
        mbedtls_aes_crypt_ecb(&aes, MBEDTLS_AES_DECRYPT, encrypted + i, decrypted_padded.data() + i);
    }
    mbedtls_aes_free(&aes);

    // PKCS7 Unpadding
    uint8_t padding_len = decrypted_padded.back();
    if (padding_len > 16 || padding_len == 0) return false; // Неверное значение дополнения

    size_t plain_len = encrypted_len - padding_len;
    output.assign(decrypted_padded.begin(), decrypted_padded.begin() + plain_len);
    
    return true;
}

bool KeyManager::loadKeys() {
    if (!LittleFS.exists(KEYS_FILE)) return true;

    File file = LittleFS.open(KEYS_FILE, "r");
    if (!file) return false;

    size_t file_size = file.size();
    if (file_size == 0) {
        file.close();
        keys.clear();
        return true;
    }
    
    std::vector<uint8_t> file_buffer(file_size);
    file.read(file_buffer.data(), file_size);
    file.close();

    std::vector<uint8_t> decrypted_buffer;
    if (!decryptData(file_buffer.data(), file_size, decrypted_buffer)) {
        return false;
    }
    
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, decrypted_buffer.data(), decrypted_buffer.size());
    if (error) {
        Serial.print("JSON parsing failed: ");
        Serial.println(error.c_str());
        return false;
    }

    keys.clear();
    JsonArray array = doc.as<JsonArray>();
    for (JsonObject obj : array) {
        keys.push_back({obj["name"].as<String>(), obj["secret"].as<String>()});
    }
    return true;
}

bool KeyManager::saveKeys() {
    JsonDocument doc;
    JsonArray array = doc.to<JsonArray>();
    for (const auto& key : keys) {
        JsonObject obj = array.add<JsonObject>();
        obj["name"] = key.name;
        obj["secret"] = key.secret;
    }
    
    String json_string;
    serializeJson(doc, json_string);

    std::vector<uint8_t> encrypted_buffer;
    if (!encryptData((uint8_t*)json_string.c_str(), json_string.length(), encrypted_buffer)) {
        return false;
    }

    File file = LittleFS.open(KEYS_FILE, "w");
    if (!file) return false;
    
    file.write(encrypted_buffer.data(), encrypted_buffer.size());
    file.close();
    return true;
}
