#ifndef MENU_SYSTEM_H
#define MENU_SYSTEM_H

#include "display_manager.h"
#include <LovyanGFX.hpp>
#include <RotaryEncoder.h>

enum MenuState {
    MAIN_DISPLAY,
    MENU_TOP_LEVEL,
    MENU_LEVEL_SUBMENU,
    MENU_DISPLAY_SUBMENU,
    MENU_TIMER_SUBMENU,
    MENU_MIC_SUBMENU,           // NEW: Microphone submenu
    MIC_DIAGNOSTIC_MODE,        // NEW: Real-time mic monitor
    ADJUSTING_VALUE
};

class MenuSystem {
public:
    MenuSystem();
    
    void begin(LGFX* tftPtr, RotaryEncoder* encoderPtr);
    void handleButton();
    void handleRotation(int delta);
    
    MenuState getState() const { return currentMenu; }
    bool isInMenu() const { return currentMenu != MAIN_DISPLAY; }
    bool isInMicDiagnostic() const { return currentMenu == MIC_DIAGNOSTIC_MODE; }
    
private:
    LGFX* tft;
    RotaryEncoder* encoder;
    
    MenuState currentMenu;
    int selectedTopItem;
    int selectedLevelItem;
    int selectedTimerItem;
    int selectedDisplayItem;
    int selectedMicItem;        // NEW: Track microphone menu selection
    
    // Value adjustment
    float* adjustingFloatValue;
    int* adjustingIntValue;
    
    // Menu drawing
    void drawTopMenu();
    void drawLevelSubmenu();
    void drawTimerSubmenu();
    void drawDisplaySubmenu();
    void drawMicSubmenu();      // NEW: Draw microphone menu
    void drawValueAdjustment(const char* label, float value, const char* unit);
    void drawValueAdjustment(const char* label, int value, const char* unit);
    
    // Menu execution
    void executeTopMenuItem(int item);
    void executeLevelMenuItem(int item);
    void executeTimerMenuItem(int item);
    void executeDisplayMenuItem(int item);
    void executeMicMenuItem(int item);  // NEW: Execute mic menu items
};

extern MenuSystem menu;

#endif