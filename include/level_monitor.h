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
    
    struct {
        float x, y, z;
    } acc;
    
    float calculateTiltAngle();
};

extern LevelMonitor levelMonitor;

#endif