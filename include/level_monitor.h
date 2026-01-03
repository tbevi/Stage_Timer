#ifndef LEVEL_MONITOR_H
#define LEVEL_MONITOR_H

#include <SensorQMI8658.hpp>
#include <FastLED.h>

enum LevelState {
    LEVEL_CCW,
    LEVEL_CENTER,
    LEVEL_CW
};

class LevelMonitor {
public:
    LevelMonitor();
    
    void begin(SensorQMI8658* qmiPtr, CRGB* ledsPtr);
    void update();
    void calibrate();
    
    float getRawAngle() const { return rawAngle; }
    float getFilteredAngle() const { return filteredAngle; }
    LevelState getState() const { return currentState; }
    uint16_t getStatusColor() const;
    CRGB getLEDColor() const;
    const char* getStatusText() const;
    
    bool needsRedraw() const { return stateChanged; }
    void clearRedrawFlag() { stateChanged = false; }
    
private:
    SensorQMI8658* qmi;
    CRGB* leds;
    
    float rawAngle;
    float filteredAngle;
    LevelState currentState;
    bool stateChanged;
    
    // Sensor data
    struct {
        float x, y, z;
    } acc;
    
    struct {
        float x, y, z;
    } gyro;
    
    // Timing for sensor fusion
    unsigned long lastUpdateTime;
    
    // Sensor fusion weight (98% gyro, 2% accel)
    static constexpr float GYRO_WEIGHT = 0.98;
    static constexpr float ACCEL_WEIGHT = 0.02;
    
    float calculateTiltAngle();
    float calculateGyroAngle(float dt);
};

extern LevelMonitor levelMonitor;

#endif