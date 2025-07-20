#ifndef TOTP_GENERATOR_H
#define TOTP_GENERATOR_H

#include <Arduino.h>

class TOTPGenerator {
public:
    // Генерация TOTP кода из секрета в формате Base32
    String generateTOTP(const String& base32Secret);

    // Получение оставшегося времени до следующего кода
    int getTimeRemaining();

private:
    // Вспомогательные функции
    void hmacSha1(const uint8_t* key, size_t keyLen, const uint8_t* data, size_t dataLen, uint8_t* output);
    uint32_t dynamicTruncation(uint8_t* hash);
    size_t base32Decode(const String& base32, uint8_t* output);
};

#endif