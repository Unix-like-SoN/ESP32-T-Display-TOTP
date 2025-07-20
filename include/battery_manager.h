#ifndef BATTERY_MANAGER_H
#define BATTERY_MANAGER_H

#include <Arduino.h>
#include "esp_adc_cal.h" // Required for ADC calibration
#include "driver/adc.h"  // Required for ADC driver functions

class BatteryManager {
public:
    BatteryManager(int adcPin, int powerPin);
    void begin();
    float getVoltage();
    int getPercentage();

private:
    int _adcPin;
    int _powerPin;
    // Калибровочные значения для LiPo батареи
    const float _maxVoltage = 3.8; // Adjusted max voltage for better discharge curve representation 
    const float _minVoltage = 3.2;

    esp_adc_cal_characteristics_t _adc_chars; // ADC calibration characteristics

    // Helper function for floating point mapping
    float fmap(float x, float in_min, float in_max, float out_min, float out_max);
};

#endif // BATTERY_MANAGER_H
