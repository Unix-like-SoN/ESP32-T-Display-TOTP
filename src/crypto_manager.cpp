#include "crypto_manager.h"
#include "mbedtls/sha256.h"
#include "mbedtls/base64.h"

String CryptoManager::hashPassword(const String& password) {
    uint8_t hashResult[32];
    mbedtls_sha256_context ctx;

    mbedtls_sha256_init(&ctx);
    mbedtls_sha256_starts(&ctx, 0); // 0 for SHA-256
    mbedtls_sha256_update(&ctx, (const unsigned char*)password.c_str(), password.length());
    mbedtls_sha256_finish(&ctx, hashResult);
    mbedtls_sha256_free(&ctx);

    String hashStr = "";
    for (int i = 0; i < 32; i++) {
        char hex[3];
        sprintf(hex, "%02x", hashResult[i]);
        hashStr += hex;
    }
    return hashStr;
}

bool CryptoManager::verifyPassword(const String& password, const String& hash) {
    String hashedPassword = hashPassword(password);
    return hashedPassword.equals(hash);
}

String CryptoManager::base64Decode(const String& encoded) {
    if (encoded.length() == 0) {
        return "";
    }
    
    size_t output_len;
    // Первым вызовом получаем необходимый размер буфера
    mbedtls_base64_decode(NULL, 0, &output_len, (const unsigned char*)encoded.c_str(), encoded.length());

    unsigned char* decoded_buf = (unsigned char*)malloc(output_len);
    if (!decoded_buf) {
        return "";
    }

    // Вторым вызовом декодируем
    int ret = mbedtls_base64_decode(decoded_buf, output_len, &output_len, (const unsigned char*)encoded.c_str(), encoded.length());
    if (ret != 0) {
        free(decoded_buf);
        return "";
    }

    String decoded_str = String((char*)decoded_buf, output_len);
    free(decoded_buf);
    return decoded_str;
}
