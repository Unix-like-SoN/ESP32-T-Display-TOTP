#ifndef CONFIG_H
#define CONFIG_H

// WiFi настройки
#define WIFI_SSID "Triolan_131"
#define WIFI_PASSWORD "f1ash970"

// Веб-сервер
#define WEB_SERVER_PORT 80
#define ADMIN_USERNAME "admin"
#define ADMIN_PASSWORD "your_secure_password"

// Дисплей - правильные пины для T-Display
#define TFT_WIDTH 135 
#define TFT_HEIGHT 240
#define TFT_MOSI 19
#define TFT_SCLK 18
#define TFT_CS   5
#define TFT_DC   16
#define TFT_RST  23
#define TFT_BL   4
#define SPI_FREQUENCY 27000000

// Кнопки для T-Display
#define BUTTON_1 35
#define BUTTON_2 0

// TOTP настройки
#define CONFIG_TOTP_STEP_SIZE 30
#define CONFIG_TOTP_DIGITS 6

// Файловая система
#define KEYS_FILE "/keys.json"
#define CONFIG_FILE "/config.json"
#define SPLASH_IMAGE_PATH "/splash.raw"
#define THEME_CONFIG_KEY "theme" // New: Key for theme setting in config.json

#endif

