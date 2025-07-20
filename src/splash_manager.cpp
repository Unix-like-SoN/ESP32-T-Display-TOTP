#include "splash_manager.h"
#include "LittleFS.h"
#include <FS.h>

SplashScreenManager::SplashScreenManager(DisplayManager& displayManager) : _displayManager(displayManager) {}

void SplashScreenManager::displaySplashScreen() {
    if (LittleFS.exists(SPLASH_IMAGE_PATH)) {
        fs::File splashFile = LittleFS.open(SPLASH_IMAGE_PATH, "r");
        if (splashFile) {
            size_t fileSize = splashFile.size();
            // Allocate buffer for the image. RGB565 is 2 bytes per pixel.
            uint16_t* imageBuffer = (uint16_t*) malloc(fileSize);
            
            if (imageBuffer) {
                splashFile.read((uint8_t*)imageBuffer, fileSize);
                _displayManager.getTft()->pushImage(0, 0, SPLASH_IMAGE_WIDTH, SPLASH_IMAGE_HEIGHT, imageBuffer);
                free(imageBuffer); // Free the buffer after use
            }
            
            splashFile.close();
            delay(2000); // Show splash for 2 seconds
        }
    }
}

bool SplashScreenManager::deleteSplashImage() {
    if (LittleFS.exists(SPLASH_IMAGE_PATH)) {
        return LittleFS.remove(SPLASH_IMAGE_PATH);
    }
    return true; // Return true if file doesn't exist anyway
}