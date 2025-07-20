#ifndef CRYPTO_MANAGER_H
#define CRYPTO_MANAGER_H

#include <Arduino.h>

class CryptoManager {
public:
    // Хеширует пароль с использованием SHA-256
    static String hashPassword(const String& password);

    // Проверяет, соответствует ли пароль заданному хешу
    static bool verifyPassword(const String& password, const String& hash);

    // Декодирует строку Base64
    static String base64Decode(const String& encoded);
};

#endif // CRYPTO_MANAGER_H
