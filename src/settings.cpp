#include "settings.h"
#include <Arduino.h>

Settings settings;  // Global instance

Settings::Settings() {
    // Default values
    tolerance = 0.5;
    // Hysteresis calculated in level_monitor as 10% of tolerance (0.05 with default)
    displayBrightness = 255;
    ledBrightness = 50;
    parTimeSeconds = 60;
    yellowWarningSeconds = 30;
    redWarningSeconds = 10;
    buzzerVolume = 50;
    micThreshold = 1500.0;  // Default threshold for beep detection

    gravity.x = 0;
    gravity.y = 0;
    gravity.z = 0;
    gravity.magnitude = 1.0;
    gravity.isCalibrated = false;
}

void Settings::load() {
    preferences.begin("stage_timer", false);
    
    tolerance = preferences.getFloat("tolerance", 0.5);
    // Hysteresis removed - now calculated in level_monitor
    displayBrightness = preferences.getInt("disp_bright", 255);
    ledBrightness = preferences.getInt("led_bright", 50);
    
    parTimeSeconds = preferences.getInt("par_time", 60);
    yellowWarningSeconds = preferences.getInt("yellow_warn", 30);
    redWarningSeconds = preferences.getInt("red_warn", 10);
  
    buzzerVolume = preferences.getInt("buzzer_vol", 50);
    micThreshold = preferences.getFloat("mic_thresh", 1500.0);
    
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
    Serial.printf("Tolerance: %.2f° (Hysteresis: %.2f° auto)\n", 
                  tolerance, tolerance * 0.1f);
}

void Settings::save() {
    preferences.begin("stage_timer", false);
    
    preferences.putFloat("tolerance", tolerance);
    // Hysteresis removed - now calculated in level_monitor
    preferences.putInt("disp_bright", displayBrightness);
    preferences.putInt("led_bright", ledBrightness);
    
    preferences.putInt("par_time", parTimeSeconds);
    preferences.putInt("yellow_warn", yellowWarningSeconds);
    preferences.putInt("red_warn", redWarningSeconds);
    
    preferences.putInt("buzzer_vol", buzzerVolume);
    preferences.putFloat("mic_thresh", micThreshold);

    preferences.end();
    
    Serial.println("Settings saved to flash");
    Serial.printf("Tolerance: %.2f° (Hysteresis: %.2f° auto)\n", 
                  tolerance, tolerance * 0.1f);
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