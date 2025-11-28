#include "battery_manager.h"
#include "pin_config.h"
#include <esp_sleep.h>

BatteryManager battery;

BatteryManager::BatteryManager() 
    : batteryVoltage(0.0)
    , batteryPercent(100)
    , state(BATTERY_DISCHARGING)
    , lastActivityTime(0)
    , autoSleepTimeout(0)
    , adc_chars(nullptr)
    , calibrated(false)
{
}

void BatteryManager::begin() {
    Serial.println("Battery Manager: Initializing...");
    
    // Check if waking from deep sleep
    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
    switch (wakeup_reason) {
        case ESP_SLEEP_WAKEUP_EXT0:
            Serial.println("Woke from deep sleep by button press");
            break;
        case ESP_SLEEP_WAKEUP_UNDEFINED:
        default:
            Serial.println("Normal power-on or reset");
            break;
    }
    
    // Configure ADC
    pinMode(BAT_ADC, INPUT);
    analogSetAttenuation(ADC_11db);  // 0-3.3V range
    
    // Initialize ADC calibration
    adc_chars = (esp_adc_cal_characteristics_t *)calloc(1, sizeof(esp_adc_cal_characteristics_t));
    
    esp_adc_cal_value_t val_type = esp_adc_cal_characterize(
        ADC_UNIT_1,
        ADC_ATTEN_DB_11,
        ADC_WIDTH_BIT_12,
        1100,  // Default Vref
        adc_chars
    );
    
    if (val_type == ESP_ADC_CAL_VAL_EFUSE_TP) {
        Serial.println("ADC calibration: eFuse Two Point");
        calibrated = true;
    } else if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF) {
        Serial.println("ADC calibration: eFuse Vref");
        calibrated = true;
    } else {
        Serial.println("ADC calibration: Default");
        calibrated = false;
    }
    
    Serial.println("ADC configured: GPIO 4 (BAT_ADC)");
    
    // Take initial reading
    update();
    
    Serial.printf("Battery: %.2fV (%d%%)\n", batteryVoltage / 1000.0, batteryPercent);
    Serial.println("Battery Manager: OK!");
}

float BatteryManager::readVoltage() {
    // Take multiple samples and average
    uint32_t sum = 0;
    int validSamples = 0;
    
    for (int i = 0; i < ADC_SAMPLES; i++) {
        uint32_t adc_reading = analogRead(BAT_ADC);
        if (adc_reading > 0) {
            sum += adc_reading;
            validSamples++;
        }
        delay(10);
    }
    
    if (validSamples == 0) {
        Serial.println("ERROR: No valid ADC samples");
        return 0.0;
    }
    
    uint32_t adc_reading = sum / validSamples;
    
    // Convert to voltage using calibration
    float voltage = 0.0;
    
    if (calibrated && adc_chars) {
        // Use calibrated conversion (more accurate)
        uint32_t voltage_mv = esp_adc_cal_raw_to_voltage(adc_reading, adc_chars);
        voltage = voltage_mv * VOLTAGE_DIVIDER;  // Apply voltage divider
    } else {
        // Fallback to simple conversion
        voltage = ((float)adc_reading / 4095.0) * 3300.0 * VOLTAGE_DIVIDER;
    }
    
    return voltage;  // Returns millivolts
}

int BatteryManager::calculatePercentage(float voltage) {
    // LiPo discharge curve - simple linear approximation
    if (voltage >= VOLTAGE_MAX) return 100;
    if (voltage <= VOLTAGE_MIN) return 0;
    
    float percent = ((voltage - VOLTAGE_MIN) / (VOLTAGE_MAX - VOLTAGE_MIN)) * 100.0;
    return constrain((int)percent, 0, 100);
}

void BatteryManager::updateState() {
    // Simple state machine based on voltage
    static float lastVoltage = batteryVoltage;
    bool voltageRising = (batteryVoltage > lastVoltage + 50);  // Rising by 50mV
    
    if (batteryVoltage >= VOLTAGE_MAX - 50 && voltageRising) {
        state = BATTERY_FULL;
    } else if (voltageRising) {
        state = BATTERY_CHARGING;
    } else if (batteryVoltage <= VOLTAGE_CRITICAL) {
        state = BATTERY_CRITICAL;
    } else if (batteryVoltage <= VOLTAGE_LOW) {
        state = BATTERY_LOW;
    } else {
        state = BATTERY_DISCHARGING;
    }
    
    lastVoltage = batteryVoltage;
}

void BatteryManager::update() {
    // Read battery voltage
    batteryVoltage = readVoltage();
    
    // Calculate percentage
    batteryPercent = calculatePercentage(batteryVoltage);
    
    // Update state
    updateState();
    
    // Check for critical battery
    if (isCritical()) {
        Serial.println("WARNING: Battery critical! Please charge soon.");
    }
}

void BatteryManager::setAutoSleep(int timeoutSeconds) {
    autoSleepTimeout = timeoutSeconds;
    lastActivityTime = millis();
    Serial.printf("Auto-sleep: %s (%ds)\n", 
                  timeoutSeconds > 0 ? "Enabled" : "Disabled",
                  timeoutSeconds);
}

void BatteryManager::resetInactivityTimer() {
    lastActivityTime = millis();
}

bool BatteryManager::shouldSleep() const {
    if (autoSleepTimeout == 0) return false;
    
    unsigned long inactiveTime = (millis() - lastActivityTime) / 1000;
    return inactiveTime >= autoSleepTimeout;
}

void BatteryManager::enterDeepSleep() {
    Serial.println("=== PREPARING FOR DEEP SLEEP ===");
    Serial.printf("Battery: %.2fV (%d%%)\n", batteryVoltage / 1000.0, batteryPercent);
    
    // Clean up ADC calibration
    if (adc_chars) {
        Serial.println("Freeing ADC calibration...");
        free(adc_chars);
        adc_chars = nullptr;
        calibrated = false;
    }
    
    Serial.println("Configuring wake source: GPIO 18 (encoder button)");
    
    // Configure wake-up on GPIO 18 (encoder button)
    // The button pulls LOW when pressed
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_18, 0);  // Wake on LOW
    
    Serial.println("Flushing serial...");
    Serial.flush();
    delay(200);  // Give time for everything to settle
    
    Serial.println("*** ENTERING DEEP SLEEP ***");
    Serial.flush();
    delay(100);
    
    // Enter deep sleep
    esp_deep_sleep_start();
    
    // Never reaches here
}

char BatteryManager::getBatteryIcon() const {
    if (isCharging()) {
        return '↯';  // Lightning bolt for charging
    }
    
    if (batteryPercent >= 80) {
        return '█';  // Full
    } else if (batteryPercent >= 60) {
        return '▓';  // Good
    } else if (batteryPercent >= 40) {
        return '▒';  // Medium
    } else if (batteryPercent >= 20) {
        return '░';  // Low
    } else {
        return '!';  // Critical
    }
}

uint16_t BatteryManager::getBatteryColor() const {
    if (isCharging()) {
        return 0x07E0;  // Green (charging)
    }
    
    if (isCritical()) {
        return 0xF800;  // Red
    } else if (isLow()) {
        return 0xFFE0;  // Yellow
    } else {
        return 0xFFFF;  // White (good)
    }
}