#include "settings.h"
#include <Arduino.h>

Settings settings;  // Global instance

Settings::Settings() {
    // Default values
    tolerance = 0.5;
    hysteresis = 0.1;
    alpha = 0.2;
    displayBrightness = 255;
    ledBrightness = 50;
    parTimeSeconds = 60;
    yellowWarningSeconds = 30;
    redWarningSeconds = 10;
    buzzerVolume = 50;
    micThreshold = 1500.0;  // Default threshold for beep detection
    autoSleepTimeout = 300;  // 5 minutes default

    gravity.x = 0;
    gravity.y = 0;
    gravity.z = 0;
    gravity.magnitude = 1.0;
    gravity.isCalibrated = false;
}

void Settings::load() {
    preferences.begin("stage_timer", false);
    
    tolerance = preferences.getFloat("tolerance", 0.5);
    hysteresis = preferences.getFloat("hysteresis", 0.1);
    alpha = preferences.getFloat("alpha", 0.2);
    displayBrightness = preferences.getInt("disp_bright", 255);
    ledBrightness = preferences.getInt("led_bright", 50);
    
    parTimeSeconds = preferences.getInt("par_time", 60);
    yellowWarningSeconds = preferences.getInt("yellow_warn", 30);
    redWarningSeconds = preferences.getInt("red_warn", 10);
  
    buzzerVolume = preferences.getInt("buzzer_vol", 50);
    micThreshold = preferences.getFloat("mic_thresh", 1500.0);
    autoSleepTimeout = preferences.getInt("sleep_time", 300);
    
    gravity.isCalibrated = preferences.getBool("calibrated", false);
    if (gravity.isCalibrated) {
        gravity.x = preferences.getFloat("grav_x", 0);
        gravity.y = preferences.getFloat("grav_y", 0);
        gravity.z = preferences.getFloat("grav_z", 0);
        gravity.magnitude = preferences.getFloat("grav_mag", 1.0);
        Serial.println("Loaded calibration from flash");
    }
    
    preferences.end();
    
    Serial.println("Settings loaded from flash");
}

void Settings::save() {
    preferences.begin("stage_timer", false);
    
    preferences.putFloat("tolerance", tolerance);
    preferences.putFloat("hysteresis", hysteresis);
    preferences.putFloat("alpha", alpha);
    preferences.putInt("disp_bright", displayBrightness);
    preferences.putInt("led_bright", ledBrightness);
    
    preferences.putInt("par_time", parTimeSeconds);
    preferences.putInt("yellow_warn", yellowWarningSeconds);
    preferences.putInt("red_warn", redWarningSeconds);
    
    preferences.putInt("buzzer_vol", buzzerVolume);
    preferences.putFloat("mic_thresh", micThreshold);
    preferences.putInt("sleep_time", autoSleepTimeout);

    preferences.end();
    
    Serial.println("Settings saved to flash");
}

void Settings::saveCalibration() {
    preferences.begin("stage_timer", false);
    
    preferences.putBool("calibrated", gravity.isCalibrated);
    preferences.putFloat("grav_x", gravity.x);
    preferences.putFloat("grav_y", gravity.y);
    preferences.putFloat("grav_z", gravity.z);
    preferences.putFloat("grav_mag", gravity.magnitude);
    
    preferences.end();
    
    Serial.println("Calibration saved to flash");
}