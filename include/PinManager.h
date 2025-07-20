#ifndef PIN_MANAGER_H
#define PIN_MANAGER_H

#include "display_manager.h"

#define PIN_CONFIG_FILE "/pin_config.json"
#define MAX_PIN_LENGTH 10

class PinManager {
public:
    PinManager(DisplayManager& display);
    void begin(); // To load PIN config
    bool isPinSet();
    void runPinCheck(); // The main blocking loop for checking PIN on startup
    
    // Methods for web server
    bool setPin(const String& newPin);
    void disablePin();

private:
    void drawPinScreen();
    bool verifyPin(const String& enteredPin);
    void loadPinConfig();
    void savePinConfig();
    String hashPin(const String& pin);

    DisplayManager& _display;
    bool _pinIsSet = false;
    String _storedPinHash = "";
    
    String _enteredPin = "";
    int _currentDigit = 0;
    int _cursorPosition = 0;
};

#endif