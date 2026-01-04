#ifndef SETTINGS_H
#define SETTINGS_H

#include <Preferences.h>

// Level display modes
enum LevelDisplayMode {
    LEVEL_DISPLAY_DEGREES,
    LEVEL_DISPLAY_ARROW
};

class Settings {
public:
    // Level settings
    float tolerance;
    // Note: Hysteresis is calculated in level_monitor as 10% of tolerance
    LevelDisplayMode levelDisplayMode;  // NEW: Display mode (degrees or arrow)
    
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