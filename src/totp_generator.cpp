#include "totp_generator.h"
#include "config.h"
#include <mbedtls/md.h>
#include <time.h>

String TOTPGenerator::generateTOTP(const String& base32Secret) {
    uint8_t key[64];
    size_t keyLen = base32Decode(base32Secret, key);

    if (keyLen == 0) {
        return "DECODE ERROR";
    }

    time_t now;
    time(&now);
    uint64_t timeStep = now / CONFIG_TOTP_STEP_SIZE;

    uint8_t timeBytes[8];
    for (int i = 7; i >= 0; i--) {
        timeBytes[i] = timeStep & 0xFF;
        timeStep >>= 8;
    }

    uint8_t hash[20];
    hmacSha1(key, keyLen, timeBytes, 8, hash);

    uint32_t code = dynamicTruncation(hash);
    
    code %= 1000000; // 6-значный код

    char codeStr[7];
    sprintf(codeStr, "%06d", code);
    return String(codeStr);
}

int TOTPGenerator::getTimeRemaining() {
    time_t now;
    time(&now);
    return CONFIG_TOTP_STEP_SIZE - (now % CONFIG_TOTP_STEP_SIZE);
}

void TOTPGenerator::hmacSha1(const uint8_t* key, size_t keyLen, const uint8_t* data, size_t dataLen, uint8_t* output) {
    mbedtls_md_context_t ctx;
    const mbedtls_md_info_t* md_info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA1);
    
    mbedtls_md_init(&ctx);
    mbedtls_md_setup(&ctx, md_info, 1); // 1 for HMAC
    mbedtls_md_hmac_starts(&ctx, key, keyLen);
    mbedtls_md_hmac_update(&ctx, data, dataLen);
    mbedtls_md_hmac_finish(&ctx, output);
    mbedtls_md_free(&ctx);
}

uint32_t TOTPGenerator::dynamicTruncation(uint8_t* hash) {
    int offset = hash[19] & 0x0F;
    return ((hash[offset] & 0x7F) << 24) |
           ((hash[offset + 1] & 0xFF) << 16) |
           ((hash[offset + 2] & 0xFF) << 8) |
           (hash[offset + 3] & 0xFF);
}

// Улучшенная реализация декодирования Base32: гибкая и корректная.
size_t TOTPGenerator::base32Decode(const String& base32, uint8_t* output) {
    const char* table = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";
    int buffer = 0;
    int bitsLeft = 0;
    size_t count = 0;

    for (char c : base32) {
        if (isspace(c)) { // Пропускаем пробелы
            continue;
        }
        
        c = toupper(c);

        if (c == '=') { // Достигли конца данных (padding)
            break;
        }

        const char* p = strchr(table, c);
        if (p == nullptr) {
            // Если символ не в таблице (например, дефис), пропускаем его
            continue;
        }

        buffer = (buffer << 5) | (p - table);
        bitsLeft += 5;

        if (bitsLeft >= 8) {
            output[count++] = (buffer >> (bitsLeft - 8)) & 0xFF;
            bitsLeft -= 8;
        }
    }
    return count;
}