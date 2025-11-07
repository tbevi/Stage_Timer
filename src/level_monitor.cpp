#include "level_monitor.h"
#include "settings.h"
#include <Arduino.h>

LevelMonitor levelMonitor;

// Color definitions
#define COLOR_RED    0xF800
#define COLOR_GREEN  0x07E0
#define COLOR_BLUE   0x001F

LevelMonitor::LevelMonitor() {
    rawAngle = 0;
    filteredAngle = 0;
    currentState = LEVEL_CENTER;
    stateChanged = true;
    qmi = nullptr;
    leds = nullptr;
}

void LevelMonitor::begin(SensorQMI8658* qmiPtr, CRGB* ledsPtr) {
    qmi = qmiPtr;
    leds = ledsPtr;
}

void LevelMonitor::calibrate() {
    Serial.println("\n*** CALIBRATION ***");
    Serial.println("Hold board LEVEL (horizontal)");
    
    float xSum = 0, ySum = 0, zSum = 0;
    
    for(int i = 0; i < 100; i++) {
        if (qmi->getDataReady()) {
            qmi->getAccelerometer(acc.x, acc.y, acc.z);
            xSum += acc.x;
            ySum += acc.y;
            zSum += acc.z;
        }
        delay(10);
    }
    
    settings.gravity.x = xSum / 100.0;
    settings.gravity.y = ySum / 100.0;
    settings.gravity.z = zSum / 100.0;
    settings.gravity.magnitude = sqrt(settings.gravity.x * settings.gravity.x + 
                                     settings.gravity.y * settings.gravity.y + 
                                     settings.gravity.z * settings.gravity.z);
    settings.gravity.isCalibrated = true;
    settings.saveCalibration();
    
    Serial.println("Calibration complete!");
    Serial.print("Gravity: X:"); Serial.print(settings.gravity.x, 3);
    Serial.print(" Y:"); Serial.print(settings.gravity.y, 3);
    Serial.print(" Z:"); Serial.println(settings.gravity.z, 3);
}

float LevelMonitor::calculateTiltAngle() {
    float xCal = acc.x - settings.gravity.x;
    float angle = atan2(xCal, -acc.y) * 180.0 / PI;
    return angle;
}

void LevelMonitor::update() {
    if (!qmi) return;
    
    // Read sensor
    if (qmi->getDataReady()) {
        qmi->getAccelerometer(acc.x, acc.y, acc.z);
    }
    
    // Calculate angles
    rawAngle = calculateTiltAngle();
    filteredAngle = settings.alpha * rawAngle + (1.0 - settings.alpha) * filteredAngle;
    
    // Determine state with hysteresis
    LevelState newState;
    
    if (currentState == LEVEL_CENTER) {
        if (rawAngle > (settings.tolerance + settings.hysteresis)) {
            newState = LEVEL_CW;
        } else if (rawAngle < -(settings.tolerance + settings.hysteresis)) {
            newState = LEVEL_CCW;
        } else {
            newState = LEVEL_CENTER;
        }
    } else if (currentState == LEVEL_CW) {
        if (rawAngle < (settings.tolerance - settings.hysteresis)) {
            newState = LEVEL_CENTER;
        } else if (rawAngle < -(settings.tolerance + settings.hysteresis)) {
            newState = LEVEL_CCW;
        } else {
            newState = LEVEL_CW;
        }
    } else { // LEVEL_CCW
        if (rawAngle > -(settings.tolerance - settings.hysteresis)) {
            newState = LEVEL_CENTER;
        } else if (rawAngle > (settings.tolerance + settings.hysteresis)) {
            newState = LEVEL_CW;
        } else {
            newState = LEVEL_CCW;
        }
    }
    
    if (newState != currentState) {
        currentState = newState;
        stateChanged = true;
    }
}

uint16_t LevelMonitor::getStatusColor() const {
    switch(currentState) {
        case LEVEL_CENTER: return COLOR_GREEN;
        case LEVEL_CW: return COLOR_RED;
        case LEVEL_CCW: return COLOR_BLUE;
        default: return COLOR_GREEN;
    }
}

CRGB LevelMonitor::getLEDColor() const {
    switch(currentState) {
        case LEVEL_CENTER: return CRGB::Green;
        case LEVEL_CW: return CRGB::Red;
        case LEVEL_CCW: return CRGB::Blue;
        default: return CRGB::Green;
    }
}

const char* LevelMonitor::getStatusText() const {
    switch(currentState) {
        case LEVEL_CENTER: return "LEVEL";
        case LEVEL_CW: return "CW";
        case LEVEL_CCW: return "CCW";
        default: return "LEVEL";
    }
}