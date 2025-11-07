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
    
private:
    LGFX* tft;
    RotaryEncoder* encoder;
    
    MenuState currentMenu;
    int selectedTopItem;
    int selectedLevelItem;
    int selectedTimerItem;
    int selectedDisplayItem;
    
    // Value adjustment
    float* adjustingFloatValue;
    int* adjustingIntValue;
    
    // Menu drawing
    void drawTopMenu();
    void drawLevelSubmenu();
    void drawTimerSubmenu();
    void drawDisplaySubmenu();
    void drawValueAdjustment(const char* label, float value, const char* unit);
    void drawValueAdjustment(const char* label, int value, const char* unit);
    
    // Menu execution
    void executeTopMenuItem(int item);
    void executeLevelMenuItem(int item);
    void executeTimerMenuItem(int item);
    void executeDisplayMenuItem(int item);
};

extern MenuSystem menu;

#endif