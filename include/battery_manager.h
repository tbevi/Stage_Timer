#ifndef BATTERY_MANAGER_H
#define BATTERY_MANAGER_H

#include <Arduino.h>
#include <driver/adc.h>
#include <esp_adc_cal.h>

enum BatteryState {
    BATTERY_CHARGING,
    BATTERY_FULL,
    BATTERY_DISCHARGING,
    BATTERY_LOW,
    BATTERY_CRITICAL
};

class BatteryManager {
public:
    BatteryManager();
    
    /**
     * Initialize battery monitoring with ADC calibration
     */
    void begin();
    
    /**
     * Update battery readings (call in main loop)
     */
    void update();
    
    /**
     * Get battery percentage (0-100%)
     */
    int getPercentage() const { return batteryPercent; }
    
    /**
     * Get battery voltage in millivolts
     */
    float getVoltage() const { return batteryVoltage; }
    
    /**
     * Get battery state
     */
    BatteryState getState() const { return state; }
    
    /**
     * Check if battery is low (< 20%)
     */
    bool isLow() const { return batteryPercent < 20; }
    
    /**
     * Check if battery is critical (< 10%)
     */
    bool isCritical() const { return batteryPercent < 10; }
    
    /**
     * Check if currently charging
     */
    bool isCharging() const { return state == BATTERY_CHARGING || state == BATTERY_FULL; }
    
    /**
     * Enable/disable deep sleep after timeout
     * @param timeoutSeconds - seconds of inactivity before sleep (0 = disabled)
     */
    void setAutoSleep(int timeoutSeconds);
    
    /**
     * Reset inactivity timer (call on user interaction)
     */
    void resetInactivityTimer();
    
    /**
     * Check if device should enter sleep mode
     */
    bool shouldSleep() const;
    
    /**
     * Enter deep sleep mode
     * Device will wake on encoder button press
     */
    void enterDeepSleep();
    
    /**
     * Get battery icon character for display
     * Returns: '█' (full), '▓' (good), '▒' (medium), '░' (low), '!' (critical)
     */
    char getBatteryIcon() const;
    
    /**
     * Get battery color for display
     */
    uint16_t getBatteryColor() const;
    
private:
    float batteryVoltage;
    int batteryPercent;
    BatteryState state;
    
    // Auto-sleep settings
    unsigned long lastActivityTime;
    int autoSleepTimeout;  // seconds, 0 = disabled
    
    // ESP ADC calibration
    esp_adc_cal_characteristics_t *adc_chars;
    bool calibrated;
    
    // ADC calibration values for LiPo battery
    static constexpr float VOLTAGE_MIN = 3200.0;  // 3.2V (empty)
    static constexpr float VOLTAGE_MAX = 4200.0;  // 4.2V (full)
    static constexpr float VOLTAGE_LOW = 3400.0;  // 3.4V (20%)
    static constexpr float VOLTAGE_CRITICAL = 3300.0;  // 3.3V (10%)
    
    // ADC configuration
    static constexpr int ADC_SAMPLES = 10;
    static constexpr float VOLTAGE_DIVIDER = 3.0;  // From example code: vol * 3
    
    /**
     * Read raw ADC value and convert to voltage using calibration
     */
    float readVoltage();
    
    /**
     * Update battery state based on voltage
     */
    void updateState();
    
    /**
     * Calculate percentage from voltage
     */
    int calculatePercentage(float voltage);
};

// Global instance
extern BatteryManager battery;

#endif // BATTERY_MANAGER_H