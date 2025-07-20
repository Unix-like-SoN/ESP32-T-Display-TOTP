#include "display_manager.h"
#include "config.h"

// Helper for the animation loop
void schedule_next_update(DisplayManager* dm, AnimationManager* am);

void animation_callback(float val, bool finished, DisplayManager* dm, AnimationManager* am) {
    dm->updateHeader();
    if (finished) {
        schedule_next_update(dm, am);
    }
}

void schedule_next_update(DisplayManager* dm, AnimationManager* am) {
    am->startAnimation(20, 0.0f, 1.0f, [dm, am](float val, bool finished) {
        animation_callback(val, finished, dm, am);
    });
}


DisplayManager::DisplayManager() : tft(TFT_eSPI()), animationManager(), headerSprite(&tft), totpContainerSprite(&tft), totpSprite(&tft) {
    _currentThemeColors = &DARK_THEME_COLORS;
    _totpState = TotpState::IDLE;
    _lastDrawnTotpString = "";
    _lastScrambleFrameTime = 0;
    _totpContainerNeedsRedraw = true;
}

void DisplayManager::setTheme(Theme theme) {
    Serial.println("DisplayManager::setTheme() called with theme: " + String((theme == Theme::LIGHT) ? "LIGHT" : "DARK"));
    switch (theme) {
        case Theme::DARK:
            _currentThemeColors = &DARK_THEME_COLORS;
            break;
        case Theme::LIGHT:
            _currentThemeColors = &LIGHT_THEME_COLORS;
            break;
    }
    Serial.println("Applying new theme colors to display...");
    tft.fillScreen(_currentThemeColors->background_dark);
    updateHeader(); 
    lastDisplayedCode = ""; 
    lastTimeRemaining = -1;
    _lastDrawnTotpString = ""; 
    _totpState = TotpState::IDLE;
    _totpContainerNeedsRedraw = true; // Force redraw of container with new theme
    Serial.println("Theme applied. Screen should update on next loop.");
}

void DisplayManager::update() {
    animationManager.update();

    if (_totpState == TotpState::IDLE) {
        return;
    }

    unsigned long currentTime = millis();
    const unsigned long frameInterval = 30; 

    if (currentTime - _lastScrambleFrameTime < frameInterval) {
        return;
    }
    _lastScrambleFrameTime = currentTime;

    if (_totpContainerNeedsRedraw) {
        drawTotpContainer();
    }

    unsigned long elapsedTime = currentTime - _totpAnimStartTime;
    const unsigned long scrambleDuration = 300;
    const unsigned long revealDuration = 150;
    const unsigned long totalDuration = scrambleDuration + revealDuration;

    if (elapsedTime >= totalDuration) {
        _totpState = TotpState::IDLE;
        drawTotpText(_newCode);
        _currentCode = _newCode;
        return;
    }

    String textToDraw = "";
    const char charset[] = "abcdefghijklmnopqrstuvwxyz0123456789";

    if (elapsedTime < scrambleDuration) {
        for (int i = 0; i < 6; i++) {
            textToDraw += charset[random(sizeof(charset) - 1)];
        }
    } else {
        int charsToReveal = (elapsedTime - scrambleDuration) / 25;
        textToDraw = _newCode.substring(0, charsToReveal);
        for (int i = charsToReveal; i < 6; i++) {
            textToDraw += charset[random(sizeof(charset) - 1)];
        }
    }
    
    drawTotpText(textToDraw);
}


void DisplayManager::init() {
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);

    tft.init();
    tft.setRotation(1);
    tft.fillScreen(_currentThemeColors->background_dark); 
    tft.setTextDatum(MC_DATUM);

    headerSprite.createSprite(tft.width(), 35);
    headerSprite.setTextDatum(MC_DATUM);

    // Создание спрайтов для TOTP
    int padding = 10;
    tft.setTextSize(4);
    int codeAreaWidth = tft.textWidth("888888") + padding * 2;
    int codeAreaHeight = 40 + 10;
    
    // Спрайт контейнера (с тенью)
    totpContainerSprite.createSprite(codeAreaWidth + 2, codeAreaHeight + 2);
    totpContainerSprite.setTextDatum(MC_DATUM);

    // Спрайт для текста (помещается внутри рамки)
    totpSprite.createSprite(codeAreaWidth - 2, codeAreaHeight - 2);
    totpSprite.setTextDatum(MC_DATUM);


    _totpState = TotpState::IDLE;
    _lastDrawnTotpString = "";
    _totpContainerNeedsRedraw = true;

    schedule_next_update(this, &animationManager);
}

void DisplayManager::drawLayout(const String& serviceName, int batteryPercentage, bool isCharging) {
    tft.fillScreen(_currentThemeColors->background_dark); 
    
    _currentServiceName = serviceName;
    _currentBatteryPercentage = batteryPercentage;
    _isCharging = isCharging;
    _headerState = HeaderState::INTRO;
    _introAnimStartTime = millis();

    lastDisplayedCode = ""; 
    lastTimeRemaining = -1;
    _lastDrawnTotpString = "";
    _totpState = TotpState::IDLE;
    _totpContainerNeedsRedraw = true;
    _isKeySwitched = true; // Флаг, что мы только что переключили ключ
}

void DisplayManager::updateBatteryStatus(int percentage, bool isCharging) {
    _currentBatteryPercentage = percentage;
    _isCharging = isCharging;

    if (_headerState != HeaderState::INTRO) {
        if (_isCharging) {
            if (_headerState != HeaderState::CHARGING) {
                _headerState = HeaderState::CHARGING;
                _chargingAnimStartTime = millis();
            }
        } else {
            _headerState = HeaderState::STATIC;
        }
    }
}

void DisplayManager::updateHeader() {
    headerSprite.fillSprite(_currentThemeColors->background_dark);

    float titleY = 20;
    
    if (_headerState == HeaderState::INTRO) {
        unsigned long elapsedTime = millis() - _introAnimStartTime;
        if (elapsedTime >= 350) {
            titleY = 20;
            _headerState = _isCharging ? HeaderState::CHARGING : HeaderState::STATIC;
            if (_isCharging) _chargingAnimStartTime = millis();
        } else {
            float progress = (float)elapsedTime / 350.0f;
            titleY = -20.0f + (40.0f * progress);
        }
    }

    headerSprite.setTextColor(_currentThemeColors->text_primary, _currentThemeColors->background_dark);
    headerSprite.setTextSize(2);
    headerSprite.drawString(_currentServiceName, headerSprite.width() / 2, (int)titleY);

    if (_headerState == HeaderState::CHARGING) {
        unsigned long chargeElapsedTime = millis() - _chargingAnimStartTime;
        int chargeProgress = (chargeElapsedTime % 1500) * 100 / 1500;
        drawBatteryOnSprite(_currentBatteryPercentage, true, chargeProgress);
    } else {
        drawBatteryOnSprite(_currentBatteryPercentage, false);
    }

    headerSprite.pushSprite(0, 0);
}

void DisplayManager::drawBatteryOnSprite(int percentage, bool isCharging, int chargingValue) {
    int x = headerSprite.width() - 28;
    int y = 5;
    int width = 22;
    int height = 11;
    int shadowOffset = 1;
    int cornerRadius = 3;

    headerSprite.drawRoundRect(x + shadowOffset, y + shadowOffset, width, height, cornerRadius, _currentThemeColors->shadow_color);
    headerSprite.fillRect(x + width + shadowOffset, y + 3 + shadowOffset, 2, 5, _currentThemeColors->shadow_color);

    headerSprite.drawRoundRect(x, y, width, height, cornerRadius, _currentThemeColors->text_secondary);
    headerSprite.fillRect(x + width, y + 3, 2, 5, _currentThemeColors->text_secondary);

    uint16_t barColor;
    int barWidth;

    if (percentage > 50) barColor = _currentThemeColors->accent_primary;
    else if (percentage > 20) barColor = _currentThemeColors->accent_secondary;
    else barColor = _currentThemeColors->error_color;
    barWidth = map(percentage, 0, 100, 0, width - 4);

    if (barWidth > 0) {
        headerSprite.fillRect(x + 2, y + 2, barWidth, height - 4, barColor);
    }
}

void DisplayManager::drawTotpContainer() {
    int cornerRadius = 8;
    int containerW = totpContainerSprite.width() - 2;
    int containerH = totpContainerSprite.height() - 2;

    // 1. Очищаем спрайт контейнера
    totpContainerSprite.fillSprite(_currentThemeColors->background_dark);

    // 2. Рисуем тень со смещением
    totpContainerSprite.fillRoundRect(2, 2, containerW, containerH, cornerRadius, _currentThemeColors->shadow_color);
    
    // 3. Рисуем основной фон контейнера
    totpContainerSprite.fillRoundRect(0, 0, containerW, containerH, cornerRadius, _currentThemeColors->background_light);
    
    // 4. Рисуем обводку поверх фона
    totpContainerSprite.drawRoundRect(0, 0, containerW, containerH, cornerRadius, _currentThemeColors->text_secondary);
    
    _totpContainerNeedsRedraw = false;
    _lastDrawnTotpString = ""; 
}

void DisplayManager::drawTotpText(const String& textToDraw) {
    if (textToDraw == _lastDrawnTotpString && !_totpContainerNeedsRedraw) {
        return; 
    }

    // 1. Рисуем анимированный текст в свой спрайт
    totpSprite.fillSprite(_currentThemeColors->background_light);
    totpSprite.setTextColor(_currentThemeColors->text_primary, _currentThemeColors->background_light);
    totpSprite.setTextSize(4);
    totpSprite.drawString(textToDraw, totpSprite.width() / 2, totpSprite.height() / 2);

    // 2. Накладываем спрайт с текстом внутрь рамки контейнера со смещением в 1px
    totpSprite.pushToSprite(&totpContainerSprite, 1, 1);

    // 3. Выводим финальный спрайт контейнера на экран
    int centerX = tft.width() / 2;
    int codeY = tft.height() / 2;
    int containerX = centerX - totpContainerSprite.width() / 2;
    int containerY = codeY - totpContainerSprite.height() / 2;
    totpContainerSprite.pushSprite(containerX, containerY);

    _lastDrawnTotpString = textToDraw;
}


void DisplayManager::updateTOTPCode(const String& code, int timeRemaining) {
    if (_totpContainerNeedsRedraw) {
        drawTotpContainer();
    }

    // Если ключ был только что переключен, просто обновляем код без анимации
    if (_isKeySwitched) {
        _currentCode = code;
        _newCode = code;
        _totpState = TotpState::IDLE;
        _isKeySwitched = false; // Сбрасываем флаг
    }

    // Логика запуска анимации, если код изменился и это не переключение ключа
    if (code != _currentCode && _totpState == TotpState::IDLE) {
        if (_currentCode.length() > 0) {
            _newCode = code;
            _totpState = TotpState::SCRAMBLING;
            _totpAnimStartTime = millis();
            _lastScrambleFrameTime = millis();
        } else {
            _currentCode = code; // Первый код устанавливается без анимации
        }
    }
    
    // Если не анимируемся, просто рисуем текущий код
    if (_totpState == TotpState::IDLE) {
        drawTotpText(_currentCode);
    }

    // Обновление прогресс-бара времени
    if (timeRemaining != lastTimeRemaining) {
        int barY = tft.height() - 30;
        int barHeight = 10;
        int barWidth = (tft.width() - 64) * 0.8;
        int barX = (tft.width() - barWidth) / 2;
        int shadowOffset = 2;
        int barCornerRadius = 5;

        // Очищаем область прогресс-бара
        tft.fillRect(barX - shadowOffset, barY - shadowOffset, barWidth + 40 + shadowOffset, barHeight + shadowOffset * 2, _currentThemeColors->background_dark);
        
        // Рисуем рамку и фон
        tft.fillRoundRect(barX + shadowOffset, barY + shadowOffset, barWidth, barHeight, barCornerRadius, _currentThemeColors->shadow_color);
        tft.drawRoundRect(barX, barY, barWidth, barHeight, barCornerRadius, _currentThemeColors->text_secondary);
        tft.fillRoundRect(barX, barY, barWidth, barHeight, barCornerRadius, _currentThemeColors->background_light);

        // Рисуем заполнение
        int fillWidth = map(timeRemaining, CONFIG_TOTP_STEP_SIZE, 0, barWidth, 0);
        tft.fillRoundRect(barX, barY, fillWidth, barHeight, barCornerRadius, _currentThemeColors->accent_primary);

        // Рисуем текст времени
        tft.setTextColor(_currentThemeColors->text_secondary, _currentThemeColors->background_dark);
        tft.setTextSize(2);
        tft.drawString(String(timeRemaining) + "s", barX + barWidth + 20, barY + barHeight / 2);
        
        lastTimeRemaining = timeRemaining;
    }
}

void DisplayManager::showMessage(const String& text, int x, int y, bool isError, int size) {
    tft.setTextDatum(TL_DATUM);
    tft.setCursor(x, y);
    tft.setTextSize(size);
    tft.setTextColor(isError ? _currentThemeColors->error_color : _currentThemeColors->text_primary, _currentThemeColors->background_dark);
    tft.println(text);
    tft.setTextDatum(MC_DATUM);
}

void DisplayManager::showMessage(const String& text, int x, int y, bool isError, int size, bool inverted) {
    tft.setTextDatum(TL_DATUM);
    tft.setCursor(x, y);
    tft.setTextSize(size);
    if (inverted) {
        tft.setTextColor(_currentThemeColors->background_dark, _currentThemeColors->text_primary);
    } else {
        tft.setTextColor(isError ? _currentThemeColors->error_color : _currentThemeColors->text_primary, _currentThemeColors->background_dark);
    }
    tft.println(text);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(_currentThemeColors->text_primary, _currentThemeColors->background_dark);
}

void DisplayManager::turnOff() { digitalWrite(TFT_BL, LOW); }
void DisplayManager::turnOn() { digitalWrite(TFT_BL, HIGH); }
TFT_eSPI* DisplayManager::getTft() { return &tft; }
void DisplayManager::fillRect(int32_t x, int32_t t, int32_t w, int32_t h, uint32_t color) { tft.fillRect(x, t, w, h, color); }
