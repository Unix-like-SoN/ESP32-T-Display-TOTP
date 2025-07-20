#ifndef SPLASH_MANAGER_H
#define SPLASH_MANAGER_H

#include "display_manager.h"

#define SPLASH_IMAGE_PATH "/splash.raw"
#define SPLASH_IMAGE_WIDTH 240
#define SPLASH_IMAGE_HEIGHT 135

class SplashScreenManager {
public:
    SplashScreenManager(DisplayManager& displayManager);
    void displaySplashScreen();
    bool deleteSplashImage();

private:
    DisplayManager& _displayManager;
};

#endif // SPLASH_MANAGER_H
