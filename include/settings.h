#ifndef SETTINGS_H
#define SETTINGS_H

#include <Preferences.h>

class Settings {
public:
    // Level settings
    float tolerance;
    // Note: Hysteresis auto-calculated as 10% of tolerance
    float alpha;  // IIR filter alpha
    
    // Display settings
    int displayBrightness;
    int ledBrightness;
    
    // Timer settings
    int parTimeSeconds;
    int yellowWarningSeconds;
    int redWarningSeconds;
    
    // Buzzer settings
    int buzzerVolume;
    
    // Microphone settings
    float micThreshold;

    // Calibration data
    struct {
        float x;
        float y;
        float z;
        float magnitude;
        bool isCalibrated;
    } gravity;
    
    Settings();
    void load();
    void save();
    void saveCalibration();
    
private:
    Preferences preferences;
};

// Global settings instance
extern Settings settings;

#endif