#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <LovyanGFX.hpp>
#include "pin_config.h"

// Color definitions
#define COLOR_RED    0xF800
#define COLOR_GREEN  0x07E0
#define COLOR_BLUE   0x001F
#define COLOR_CYAN   0x07FF
#define COLOR_YELLOW 0xFFE0
#define COLOR_WHITE  0xFFFF
#define COLOR_ORANGE 0xFD20

// LovyanGFX for ST7789 display
class LGFX : public lgfx::LGFX_Device
{
    lgfx::Panel_ST7789 _panel_instance;
    lgfx::Bus_SPI _bus_instance;
    lgfx::Light_PWM _light_instance;

public:
    LGFX(void);
};

class DisplayManager {
public:
    DisplayManager();
    
    void begin();
    void setBrightness(int brightness);
    
    // Level display (top 1/3)
    void drawLevelIndicator(float angle, uint16_t color, const char* status);
    void updateLevelAngle(float angle, uint16_t color);
    
    // Timer display (bottom 2/3)
    void drawTimerDisplay(int remainingSeconds, float percentage, uint16_t timerColor, const char* stateText);
    
    // Full screen displays
    void drawShooterReady();
    
    // Microphone diagnostic display
    void drawMicDiagnostics(float magnitude, float threshold, float noiseFloor, 
                           float peakMag, float avgMag, int detections);
    
    
    // Battery indicator (top-right corner)
    void drawBatteryIndicator(int percent, bool charging, uint16_t color);
    // Helper functions
    void drawProgressBar(int x, int y, int width, int height, float percentage, uint16_t color);
    
    LGFX* getTFT() { return &tft; }
    
private:
    LGFX tft;
    bool firstDraw;
    int lastDrawnSeconds;
    float lastDisplayedAngle;
    uint16_t lastStatusColor;
};

extern DisplayManager display;

#endif