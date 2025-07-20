#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <TFT_eSPI.h>
#include "animation_manager.h"
#include "ui_themes.h" // Include new theme definitions

class DisplayManager {
public:
    enum class HeaderState { INTRO, STATIC, CHARGING };

    DisplayManager();
    void init();
    void update(); 
    void updateHeader();
    
    void drawLayout(const String& serviceName, int batteryPercentage, bool isCharging); 
    void updateBatteryStatus(int percentage, bool isCharging);
    void updateTOTPCode(const String& code, int timeRemaining);
    void turnOff();
    void turnOn();
    bool isCharging() const { return _isCharging; }

    void setTheme(Theme theme); // New method to set the theme

    // Deprecated, but kept for compatibility with other code
    void showMessage(const String& text, int x, int y, bool isError = false, int size = 1);
    void showMessage(const String& text, int x, int y, bool isError, int size, bool inverted);
    void fillRect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color);
    TFT_eSPI* getTft();

private:
    // New state machine for TOTP display
    enum class TotpState { IDLE, SCRAMBLING, REVEALING };

    void drawBatteryOnSprite(int percentage, bool isCharging, int chargingValue = 0);
    void drawTotpContainer();
    void drawTotpText(const String& textToDraw);

    TFT_eSPI tft;
    AnimationManager animationManager;
    TFT_eSprite headerSprite;
    TFT_eSprite totpContainerSprite;
    TFT_eSprite totpSprite;
    const ThemeColors* _currentThemeColors; // Pointer to the active theme colors

    // State Machine Variables
    HeaderState _headerState = HeaderState::STATIC;
    String _currentServiceName;
    int _currentBatteryPercentage = 0;
    bool _isCharging = false;

    // Animation-specific variables
    unsigned long _introAnimStartTime = 0;
    unsigned long _chargingAnimStartTime = 0;

    // Variables for flicker-free TOTP updates
    String lastDisplayedCode;
    int lastTimeRemaining;

    // --- New variables for the premium TOTP animation ---
    TotpState _totpState = TotpState::IDLE;
    unsigned long _totpAnimStartTime = 0;
    String _newCode;
    String _currentCode;
    String _lastDrawnTotpString;
    unsigned long _lastScrambleFrameTime = 0;
    bool _totpContainerNeedsRedraw = true;
    bool _isKeySwitched = false;
};

#endif // DISPLAY_MANAGER_H