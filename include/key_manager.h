#ifndef KEY_MANAGER_H
#define KEY_MANAGER_H

#include <vector>
#include <Arduino.h>

// Структура для хранения ключа
struct TOTPKey {
    String name;
    String secret;
};

class KeyManager {
public:
    KeyManager();
    bool begin(); // Загружает ключи в память при старте
    
    // Функции для управления ключами
    bool addKey(const String& name, const String& secret);
    bool removeKey(int index);
    std::vector<TOTPKey> getAllKeys();
    bool replaceAllKeys(const String& jsonContent); // Новая функция

private:
    bool loadKeys();
    bool saveKeys();

    // Шифрование/дешифрование с помощью внутреннего ключа
    void generateDeviceKey(unsigned char* key);
    bool encryptData(const uint8_t* plain, size_t plain_len, std::vector<uint8_t>& output);
    bool decryptData(const uint8_t* encrypted, size_t encrypted_len, std::vector<uint8_t>& output);

    std::vector<TOTPKey> keys; // Ключи хранятся в памяти в расшифрованном виде
};

#endif // KEY_MANAGER_H