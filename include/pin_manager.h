#ifndef PIN_MANAGER_H
#define PIN_MANAGER_H

#include "display_manager.h"

#define PIN_FILE "/pincode.json"
#define DEFAULT_PIN_LENGTH 6
#define MAX_PIN_LENGTH 10

class PinManager {
public:
    PinManager(DisplayManager& display);
    void begin();
    bool isPinEnabled();
    bool isPinSet();
    void requestPin();
    
    // Methods for web server interaction
    void setPin(const String& newPin);
    void setEnabled(bool enabled);
    int getPinLength();
    void setPinLength(int newLength);
    void saveConfig();
    void loadPinConfig();

private:
    DisplayManager& displayManager;
    int currentPinLength = DEFAULT_PIN_LENGTH;
    bool enabled = false;
    String pinHash = "";

    void savePinConfig();
    bool checkPin(const String& pin);
    void drawPinScreen();
    void updatePinScreen(int currentPosition, int currentDigit, const String& enteredPin);
};

#endif // PIN_MANAGER_H
