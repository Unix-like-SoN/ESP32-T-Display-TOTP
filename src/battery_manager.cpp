#include "battery_manager.h"
#include "esp_log.h" // For logging

static const char *TAG = "BatteryManager"; // Tag for ESP_LOGx

BatteryManager::BatteryManager(int adcPin, int powerPin) : _adcPin(adcPin), _powerPin(powerPin) {}

void BatteryManager::begin() {
    pinMode(_powerPin, OUTPUT);
    digitalWrite(_powerPin, LOW); // Убедимся, что питание делителя выключено по умолчанию

    // Configure ADC
    adc1_config_width(ADC_WIDTH_BIT_12); // 12-bit resolution
    adc1_config_channel_atten(ADC1_CHANNEL_6, ADC_ATTEN_DB_11); // GPIO 34 is ADC1_CHANNEL_6, 11dB attenuation

    // Characterize ADC for calibration
    esp_adc_cal_value_t val_type = esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 1100, &_adc_chars);
    if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF) {
        ESP_LOGI(TAG, "eFuse Vref");
    } else if (val_type == ESP_ADC_CAL_VAL_EFUSE_TP) {
        ESP_LOGI(TAG, "eFuse Two Point");
    } else {
        ESP_LOGI(TAG, "Default Vref");
    }
}

float BatteryManager::getVoltage() {
    digitalWrite(_powerPin, HIGH); // Включаем питание делителя напряжения
    delay(10); // Небольшая задержка для стабилизации напряжения
    
    uint32_t adcRaw = adc1_get_raw(ADC1_CHANNEL_6); // Get raw ADC value
    uint32_t voltage_mv = esp_adc_cal_raw_to_voltage(adcRaw, &_adc_chars); // Convert raw to voltage in mV

    Serial.print("Raw ADC Value: ");
    Serial.println(adcRaw);
    Serial.print("Voltage at ADC Pin (mV): ");
    Serial.println(voltage_mv);
    
    digitalWrite(_powerPin, LOW); // Выключаем питание делителя для экономии

    // Assuming a 1:2 voltage divider (voltage at ADC pin is half the battery voltage)
    // Convert mV to V and multiply by divider ratio
    float voltage = (float)voltage_mv / 1000.0 * 1.826; // Adjusted based on observed ADC voltage for full battery 
    
    Serial.print("Calculated Battery Voltage: ");
    Serial.println(voltage, 3); // Print with 3 decimal places
    return voltage;
}

// Реализация map для float
float BatteryManager::fmap(float x, float in_min, float in_max, float out_min, float out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

int BatteryManager::getPercentage() {
    float voltage = getVoltage();
    // Ограничиваем напряжение в пределах калибровки
    voltage = constrain(voltage, _minVoltage, _maxVoltage);
    // Преобразуем напряжение в проценты с использованием fmap
    int percentage = fmap(voltage, _minVoltage, _maxVoltage, 0.0, 100.0);

    Serial.print("BatteryManager::getPercentage() - Voltage (constrained): ");
    Serial.print(voltage, 3);
    Serial.print("V, Percentage: ");
    Serial.println(percentage);

    // Ensure percentage is within 0-100 range
    return constrain(percentage, 0, 100);
}