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
    lastUpdateTime = 0;
    
    acc.x = 0;
    acc.y = 0;
    acc.z = 0;
    
    gyro.x = 0;
    gyro.y = 0;
    gyro.z = 0;
}

void LevelMonitor::begin(SensorQMI8658* qmiPtr, CRGB* ledsPtr) {
    qmi = qmiPtr;
    leds = ledsPtr;
    lastUpdateTime = millis();
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
    // Accelerometer-based tilt (subject to linear acceleration)
    float xCal = acc.x - settings.gravity.x;
    float angle = atan2(xCal, -acc.y) * 180.0 / PI;
    return angle;
}

float LevelMonitor::calculateGyroAngle(float dt) {
    // Integrate gyroscope Z-axis (rotation around vertical axis)
    // QMI8658 gyro units are degrees/second, so direct integration
    float gyroAngleChange = gyro.z * dt;
    return gyroAngleChange;
}

void LevelMonitor::update() {
    if (!qmi) return;
    
    // Calculate time delta
    unsigned long currentTime = millis();
    float dt = (currentTime - lastUpdateTime) / 1000.0;  // Convert to seconds
    lastUpdateTime = currentTime;
    
    // Prevent huge dt on first call or long delays
    if (dt > 0.1 || dt <= 0) {
        dt = 0.01;  // Default to 10ms
    }
    
    // Read both sensors
    if (qmi->getDataReady()) {
        qmi->getAccelerometer(acc.x, acc.y, acc.z);
        qmi->getGyroscope(gyro.x, gyro.y, gyro.z);
    }
    
    // Calculate angles from both sources
    float accelAngle = calculateTiltAngle();
    float gyroAngleChange = calculateGyroAngle(dt);
    
    // SENSOR FUSION: Complementary filter
    // 98% gyro (short-term accuracy, no vibration noise)
    // 2% accel (long-term reference, prevents gyro drift)
    filteredAngle = GYRO_WEIGHT * (filteredAngle + gyroAngleChange) + 
                    ACCEL_WEIGHT * accelAngle;
    
    // Store for debugging
    rawAngle = accelAngle;
    
    // Calculate hysteresis as 10% of tolerance
    // This belongs here in level_monitor, not in settings
    float hysteresis = settings.tolerance * 0.1f;
    
    // Determine state with hysteresis
    LevelState newState;
    
    if (currentState == LEVEL_CENTER) {
        if (filteredAngle > (settings.tolerance + hysteresis)) {
            newState = LEVEL_CW;
        } else if (filteredAngle < -(settings.tolerance + hysteresis)) {
            newState = LEVEL_CCW;
        } else {
            newState = LEVEL_CENTER;
        }
    } else if (currentState == LEVEL_CW) {
        if (filteredAngle < (settings.tolerance - hysteresis)) {
            newState = LEVEL_CENTER;
        } else if (filteredAngle < -(settings.tolerance + hysteresis)) {
            newState = LEVEL_CCW;
        } else {
            newState = LEVEL_CW;
        }
    } else { // LEVEL_CCW
        if (filteredAngle > -(settings.tolerance - hysteresis)) {
            newState = LEVEL_CENTER;
        } else if (filteredAngle > (settings.tolerance + hysteresis)) {
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