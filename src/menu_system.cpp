#include "menu_system.h"
#include "settings.h"
#include "level_monitor.h"
#include "display_manager.h"
#include <FastLED.h>
#include "buzzer.h"
#include "mic_detector.h" 

extern CRGB leds[];

MenuSystem menu;

enum TopMenuItem {
    TOP_LEVEL,
    TOP_TIMER,
    TOP_DISPLAY,
    TOP_MICROPHONE,  
    TOP_EXIT,
    TOP_ITEM_COUNT
};

enum LevelSubItem {
    LEVEL_CALIBRATE,
    LEVEL_TOLERANCE,
    LEVEL_DISPLAY_MODE,
    LEVEL_BACK,
    LEVEL_ITEM_COUNT
};

enum TimerSubItem {
    TIMER_PAR_TIME,
    TIMER_YELLOW_WARNING,
    TIMER_RED_WARNING,
    TIMER_BACK,
    TIMER_ITEM_COUNT
};

enum DisplaySubItem {
    DISPLAY_BRIGHTNESS,
    DISPLAY_LED_BRIGHTNESS,
    DISPLAY_BUZZER_VOLUME, 
    DISPLAY_BACK,
    DISPLAY_ITEM_COUNT
};

enum MicSubItem {
    MIC_MONITOR,
    MIC_THRESHOLD,
    MIC_BACK,
    MIC_ITEM_COUNT
};

MenuSystem::MenuSystem() {
    currentMenu = MAIN_DISPLAY;
    selectedTopItem = 0;
    selectedMicItem = 0;
    selectedLevelItem = 0;
    selectedTimerItem = 0;
    selectedDisplayItem = 0;
    adjustingFloatValue = nullptr;
    adjustingIntValue = nullptr;
    tft = nullptr;
    encoder = nullptr;
}

void MenuSystem::begin(LGFX* tftPtr, RotaryEncoder* encoderPtr) {
    tft = tftPtr;
    encoder = encoderPtr;
}

void MenuSystem::handleButton() {
    if (currentMenu == MAIN_DISPLAY) {
        // Enter menu
        currentMenu = MENU_TOP_LEVEL;
        selectedTopItem = 0;
        encoder->setPosition(0);
        drawTopMenu();
        Serial.println("Entered menu");
    } else if (currentMenu == MENU_TOP_LEVEL) {
        executeTopMenuItem(selectedTopItem);
    } else if (currentMenu == MENU_LEVEL_SUBMENU) {
        executeLevelMenuItem(selectedLevelItem);
    } else if (currentMenu == MENU_TIMER_SUBMENU) {
        executeTimerMenuItem(selectedTimerItem);
    } else if (currentMenu == MENU_DISPLAY_SUBMENU) {
        executeDisplayMenuItem(selectedDisplayItem);
        } else if (currentMenu == MENU_MIC_SUBMENU) {
        executeMicMenuItem(selectedMicItem);
    } else if (currentMenu == MIC_DIAGNOSTIC_MODE) {
        // Exit diagnostic mode
        currentMenu = MENU_MIC_SUBMENU;
        drawMicSubmenu();
    } else if (currentMenu == ADJUSTING_VALUE) {

        // Save and return
        if (adjustingFloatValue != nullptr) {
            if (adjustingFloatValue == &settings.tolerance) {
                currentMenu = MENU_LEVEL_SUBMENU;
                drawLevelSubmenu();
            } else if (adjustingFloatValue == &settings.micThreshold) {  // ADD THIS
                micDetector.setThreshold(settings.micThreshold);
                currentMenu = MENU_MIC_SUBMENU;
                drawMicSubmenu();
            }
            adjustingFloatValue = nullptr;
        } else if (adjustingIntValue != nullptr) {
            if (adjustingIntValue == &settings.displayBrightness) {
                display.setBrightness(settings.displayBrightness);
                currentMenu = MENU_DISPLAY_SUBMENU;
                drawDisplaySubmenu();
            } else if (adjustingIntValue == &settings.ledBrightness) {
                FastLED.setBrightness(settings.ledBrightness);
                currentMenu = MENU_DISPLAY_SUBMENU;
                drawDisplaySubmenu();
            } else if (adjustingIntValue == &settings.buzzerVolume) {  // <-- Add this
                currentMenu = MENU_DISPLAY_SUBMENU;
                drawDisplaySubmenu();
            } else {
                currentMenu = MENU_TIMER_SUBMENU;
                drawTimerSubmenu();
            }
            adjustingIntValue = nullptr;
        }
        settings.save();
    }
}

void MenuSystem::handleRotation(int delta) {
    if (currentMenu == MENU_TOP_LEVEL) {
        selectedTopItem += delta;
        if (selectedTopItem < 0) selectedTopItem = TOP_ITEM_COUNT - 1;
        if (selectedTopItem >= TOP_ITEM_COUNT) selectedTopItem = 0;
        drawTopMenu();
    } else if (currentMenu == MENU_LEVEL_SUBMENU) {
        selectedLevelItem += delta;
        if (selectedLevelItem < 0) selectedLevelItem = LEVEL_ITEM_COUNT - 1;
        if (selectedLevelItem >= LEVEL_ITEM_COUNT) selectedLevelItem = 0;
        drawLevelSubmenu();
    } else if (currentMenu == MENU_TIMER_SUBMENU) {
        selectedTimerItem += delta;
        if (selectedTimerItem < 0) selectedTimerItem = TIMER_ITEM_COUNT - 1;
        if (selectedTimerItem >= TIMER_ITEM_COUNT) selectedTimerItem = 0;
        drawTimerSubmenu();
    } else if (currentMenu == MENU_DISPLAY_SUBMENU) {
        selectedDisplayItem += delta;
        if (selectedDisplayItem < 0) selectedDisplayItem = DISPLAY_ITEM_COUNT - 1;
        if (selectedDisplayItem >= DISPLAY_ITEM_COUNT) selectedDisplayItem = 0;
        drawDisplaySubmenu();
        } else if (currentMenu == MENU_MIC_SUBMENU) {
        selectedMicItem += delta;
        if (selectedMicItem < 0) selectedMicItem = MIC_ITEM_COUNT - 1;
        if (selectedMicItem >= MIC_ITEM_COUNT) selectedMicItem = 0;
        drawMicSubmenu();
    } else if (currentMenu == MIC_DIAGNOSTIC_MODE) {
        // In diagnostic mode, adjust threshold with encoder
        int newPos = encoder->getPosition();
        settings.micThreshold = newPos * 50;
        settings.micThreshold = constrain(settings.micThreshold, 100.0, 10000.0);
        // Don't redraw here, main loop handles it
    } else if (currentMenu == ADJUSTING_VALUE) {
        int newPos = encoder->getPosition();
        
        if (adjustingFloatValue != nullptr) {
            if (adjustingFloatValue == &settings.tolerance) {
                *adjustingFloatValue = newPos * 0.1;
                *adjustingFloatValue = constrain(*adjustingFloatValue, 0.1, 5.0);
                drawValueAdjustment("TOLERANCE", *adjustingFloatValue, "deg");
            }
        } else if (adjustingIntValue != nullptr) {
            if (adjustingIntValue == &settings.displayBrightness) {
                *adjustingIntValue = newPos * 10;
                *adjustingIntValue = constrain(*adjustingIntValue, 10, 255);
                display.setBrightness(*adjustingIntValue);
                drawValueAdjustment("BRIGHTNESS", *adjustingIntValue, "");
            } else if (adjustingIntValue == &settings.ledBrightness) {
                *adjustingIntValue = newPos * 5;
                *adjustingIntValue = constrain(*adjustingIntValue, 5, 255);
                FastLED.setBrightness(*adjustingIntValue);
                leds[0] = CRGB::White;
                FastLED.show();
                drawValueAdjustment("LED BRIGHTNESS", *adjustingIntValue, "");
            } else if (adjustingIntValue == &settings.buzzerVolume) {  // <-- Add this
                *adjustingIntValue = newPos * 5;
                *adjustingIntValue = constrain(*adjustingIntValue, 0, 100);
                // Test beep when adjusting
                if (newPos % 4 == 0) {  // Beep every 20% change
                    buzzer.beepStart();
                }
                drawValueAdjustment("BUZZER VOL", *adjustingIntValue, "%");    
            } else if (adjustingIntValue == &settings.parTimeSeconds) {
                *adjustingIntValue = newPos;
                *adjustingIntValue = constrain(*adjustingIntValue, 5, 600);
                drawValueAdjustment("PAR TIME", *adjustingIntValue, "sec");
            } else if (adjustingIntValue == &settings.yellowWarningSeconds) {
                *adjustingIntValue = newPos;
                *adjustingIntValue = constrain(*adjustingIntValue, 5, settings.parTimeSeconds - 5);
                drawValueAdjustment("YELLOW WARN", *adjustingIntValue, "sec");
            } else if (adjustingIntValue == &settings.redWarningSeconds) {
                *adjustingIntValue = newPos;
                *adjustingIntValue = constrain(*adjustingIntValue, 1, settings.yellowWarningSeconds - 5);
                drawValueAdjustment("RED WARNING", *adjustingIntValue, "sec");
            }
        }
    }
}

void MenuSystem::drawTopMenu() {
    tft->fillScreen(TFT_BLACK);
    tft->setTextSize(2);
    tft->setTextColor(TFT_WHITE);
    tft->setCursor(25, 15);
    tft->println("SETTINGS");
    
    const char* menuItems[] = {"Level", "Timer", "Display", "Microphone", "Exit"};
    int startY = 60;
    int boxHeight = 45;
    int spacing = 8;
    
    for (int i = 0; i < TOP_ITEM_COUNT; i++) {
        int y = startY + (i * (boxHeight + spacing));
        
        if (i == selectedTopItem) {
            tft->fillRect(5, y, 160, boxHeight, TFT_BLUE);
            tft->setTextColor(TFT_WHITE);
        } else {
            tft->drawRect(5, y, 160, boxHeight, TFT_DARKGREY);
            tft->setTextColor(TFT_LIGHTGREY);
        }
        
        tft->setTextSize(2);
        tft->setCursor(45, y + 14);
        tft->println(menuItems[i]);
    }
    
    tft->setTextSize(1);
    tft->setTextColor(TFT_DARKGREY);
    tft->setCursor(15, 295);
    tft->println("Turn: Select");
    tft->setCursor(15, 310);
    tft->println("Press: Confirm");
}

void MenuSystem::drawLevelSubmenu() {
    tft->fillScreen(TFT_BLACK);
    tft->setTextSize(2);
    tft->setTextColor(TFT_WHITE);
    tft->setCursor(5, 10);
    tft->println("< LEVEL");
    
    const char* menuItems[] = {"Calibrate", "Tolerance", "Display", "Back"};
    int startY = 50;
    int boxHeight = 50;
    int spacing = 8;
    
    for (int i = 0; i < LEVEL_ITEM_COUNT; i++) {
        int y = startY + (i * (boxHeight + spacing));
        
        if (i == selectedLevelItem) {
            tft->fillRect(5, y, 160, boxHeight, TFT_BLUE);
            tft->setTextColor(TFT_WHITE);
        } else {
            tft->drawRect(5, y, 160, boxHeight, TFT_DARKGREY);
            tft->setTextColor(TFT_LIGHTGREY);
        }
        
        tft->setTextSize(2);
        tft->setCursor(10, y + 5);
        tft->println(menuItems[i]);
        
        if (i == LEVEL_TOLERANCE) {
            tft->setTextSize(2);
            tft->setCursor(10, y + 27);
            tft->print(settings.tolerance, 1);
            tft->print(" deg");
        } else if (i == LEVEL_DISPLAY_MODE) {
            tft->setTextSize(2);
            tft->setCursor(10, y + 27);
            if (settings.levelDisplayMode == LEVEL_DISPLAY_DEGREES) {
                tft->print("Degrees");
            } else {
                tft->print("Arrow");
            }
        }
    }
    
    tft->setTextSize(1);
    tft->setTextColor(TFT_DARKGREY);
    tft->setCursor(15, 295);
    tft->println("Turn: Select");
    tft->setCursor(15, 310);
    tft->println("Press: Confirm");
}

void MenuSystem::drawTimerSubmenu() {
    tft->fillScreen(TFT_BLACK);
    tft->setTextSize(2);
    tft->setTextColor(TFT_WHITE);
    tft->setCursor(5, 10);
    tft->println("< TIMER");
    
    const char* menuItems[] = {"Par Time", "Yellow Warn", "Red Warning", "Back"};
    int startY = 50;
    int boxHeight = 50;
    int spacing = 8;
    
    for (int i = 0; i < TIMER_ITEM_COUNT; i++) {
        int y = startY + (i * (boxHeight + spacing));
        
        if (i == selectedTimerItem) {
            tft->fillRect(5, y, 160, boxHeight, TFT_BLUE);
            tft->setTextColor(TFT_WHITE);
        } else {
            tft->drawRect(5, y, 160, boxHeight, TFT_DARKGREY);
            tft->setTextColor(TFT_LIGHTGREY);
        }
        
        tft->setTextSize(2);
        tft->setCursor(10, y + 5);
        tft->println(menuItems[i]);
        
        if (i == TIMER_PAR_TIME) {
            tft->setCursor(10, y + 27);
            tft->print(settings.parTimeSeconds);
            tft->print(" sec");
        } else if (i == TIMER_YELLOW_WARNING) {
            tft->setCursor(10, y + 27);
            tft->print(settings.yellowWarningSeconds);
            tft->print(" sec");
        } else if (i == TIMER_RED_WARNING) {
            tft->setCursor(10, y + 27);
            tft->print(settings.redWarningSeconds);
            tft->print(" sec");
        }
    }
    
    tft->setTextSize(1);
    tft->setTextColor(TFT_DARKGREY);
    tft->setCursor(15, 295);
    tft->println("Turn: Select");
    tft->setCursor(15, 310);
    tft->println("Press: Confirm");
}

void MenuSystem::drawDisplaySubmenu() {
    tft->fillScreen(TFT_BLACK);
    tft->setTextSize(2);
    tft->setTextColor(TFT_WHITE);
    tft->setCursor(5, 10);
    tft->println("< DISPLAY");
    
    const char* menuItems[] = {"Brightness", "LED Bright", "Buzzer Vol", "Back"};
    int startY = 50;
    int boxHeight = 45;
    int spacing = 6;
    
    for (int i = 0; i < DISPLAY_ITEM_COUNT; i++) {
        int y = startY + (i * (boxHeight + spacing));
        
        if (i == selectedDisplayItem) {
            tft->fillRect(5, y, 160, boxHeight, TFT_BLUE);
            tft->setTextColor(TFT_WHITE);
        } else {
            tft->drawRect(5, y, 160, boxHeight, TFT_DARKGREY);
            tft->setTextColor(TFT_LIGHTGREY);
        }
        
        tft->setTextSize(2);
        tft->setCursor(10, y + 5);
        tft->println(menuItems[i]);
        
        if (i == DISPLAY_BRIGHTNESS) {
            tft->setTextSize(2);
            tft->setCursor(10, y + 25);
            tft->print(settings.displayBrightness);
        } else if (i == DISPLAY_LED_BRIGHTNESS) {
            tft->setTextSize(2);
            tft->setCursor(10, y + 25);
            tft->print(settings.ledBrightness);
        } else if (i == DISPLAY_BUZZER_VOLUME) {
            tft->setTextSize(2);
            tft->setCursor(10, y + 25);
            tft->print(settings.buzzerVolume);
            tft->print("%");
        }
    }
    
    tft->setTextSize(1);
    tft->setTextColor(TFT_DARKGREY);
    tft->setCursor(15, 290);
    tft->println("Turn: Select");
    tft->setCursor(15, 305);
    tft->println("Press: Confirm");
}

void MenuSystem::drawValueAdjustment(const char* label, float value, const char* unit) {
    tft->fillScreen(TFT_BLACK);
    tft->setTextSize(2);
    tft->setTextColor(TFT_WHITE);
    tft->setCursor(5, 15);
    tft->print("< ");
    tft->println(label);
    
    tft->drawRect(5, 80, 160, 90, TFT_WHITE);
    tft->setTextSize(4);
    tft->setTextColor(COLOR_CYAN);
    tft->setCursor(15, 100);
    tft->print(value, 2);
    
    tft->setTextSize(2);
    tft->setCursor(15, 140);
    tft->print(unit);
    
    tft->setTextSize(1);
    tft->setTextColor(TFT_DARKGREY);
    tft->setCursor(15, 250);
    tft->println("Turn: Adjust");
    tft->setCursor(15, 265);
    tft->println("Press: Save & Exit");
}

void MenuSystem::drawValueAdjustment(const char* label, int value, const char* unit) {
    tft->fillScreen(TFT_BLACK);
    tft->setTextSize(2);
    tft->setTextColor(TFT_WHITE);
    tft->setCursor(5, 15);
    tft->print("< ");
    tft->println(label);
    
    tft->drawRect(5, 80, 160, 90, TFT_WHITE);
    tft->setTextSize(4);
    tft->setTextColor(COLOR_CYAN);
    tft->setCursor(40, 110);
    tft->print(value);
    
    tft->setTextSize(2);
    tft->setCursor(15, 140);
    tft->print(unit);
    
    int barWidth = map(constrain(value, 0, 255), 0, 255, 0, 150);
    tft->fillRect(10, 190, barWidth, 15, COLOR_GREEN);
    tft->drawRect(10, 190, 150, 15, TFT_WHITE);
    
    tft->setTextSize(1);
    tft->setTextColor(TFT_DARKGREY);
    tft->setCursor(15, 250);
    tft->println("Turn: Adjust");
    tft->setCursor(15, 265);
    tft->println("Press: Save & Exit");
}

void MenuSystem::executeTopMenuItem(int item) {
    switch(item) {
        case TOP_LEVEL:
            currentMenu = MENU_LEVEL_SUBMENU;
            selectedLevelItem = 0;
            encoder->setPosition(0);
            drawLevelSubmenu();
            break;
        case TOP_TIMER:
            currentMenu = MENU_TIMER_SUBMENU;
            selectedTimerItem = 0;
            encoder->setPosition(0);
            drawTimerSubmenu();
            break;
        case TOP_DISPLAY:
            currentMenu = MENU_DISPLAY_SUBMENU;
            selectedDisplayItem = 0;
            encoder->setPosition(0);
            drawDisplaySubmenu();
            break;
        case TOP_MICROPHONE:  // ADD THIS CASE
            currentMenu = MENU_MIC_SUBMENU;
            selectedMicItem = 0;
            encoder->setPosition(0);
            drawMicSubmenu();
            break;
        case TOP_EXIT:
            settings.save();
            currentMenu = MAIN_DISPLAY;
            display.setBrightness(settings.displayBrightness);
            FastLED.setBrightness(settings.ledBrightness);
            tft->fillScreen(TFT_BLACK); 
            Serial.println("Exited menu");
            break;
    }
}

void MenuSystem::executeLevelMenuItem(int item) {
    switch(item) {
        case LEVEL_CALIBRATE:
            tft->fillScreen(COLOR_CYAN);
            tft->setTextColor(TFT_BLACK);
            tft->setTextSize(1);
            tft->setCursor(10, 100);
            tft->println("CALIBRATING...");
            tft->setCursor(10, 120);
            tft->println("Hold LEVEL");
            delay(1000);
            
            levelMonitor.calibrate();
            
            tft->fillScreen(COLOR_GREEN);
            tft->setTextSize(2);
            tft->setCursor(30, 140);
            tft->println("DONE!");
            delay(1500);
            drawLevelSubmenu();
            break;
        case LEVEL_TOLERANCE:
            currentMenu = ADJUSTING_VALUE;
            adjustingFloatValue = &settings.tolerance;
            encoder->setPosition((int)(settings.tolerance * 10));
            drawValueAdjustment("TOLERANCE", settings.tolerance, "deg");
            break;
        case LEVEL_DISPLAY_MODE:
            // Toggle between degrees and arrow
            if (settings.levelDisplayMode == LEVEL_DISPLAY_DEGREES) {
                settings.levelDisplayMode = LEVEL_DISPLAY_ARROW;
            } else {
                settings.levelDisplayMode = LEVEL_DISPLAY_DEGREES;
            }
            settings.save();
            drawLevelSubmenu();
            Serial.printf("Display mode changed to: %s\n",
                            settings.levelDisplayMode == LEVEL_DISPLAY_DEGREES ? "Degrees" : "Arrow");
            break;
        case LEVEL_BACK:
            currentMenu = MENU_TOP_LEVEL;
            selectedTopItem = 0;
            encoder->setPosition(0);
            drawTopMenu();
            break;
    }
}

void MenuSystem::executeTimerMenuItem(int item) {
    switch(item) {
        case TIMER_PAR_TIME:
            currentMenu = ADJUSTING_VALUE;
            adjustingIntValue = &settings.parTimeSeconds;
            encoder->setPosition(settings.parTimeSeconds);
            drawValueAdjustment("PAR TIME", settings.parTimeSeconds, "sec");
            break;
        case TIMER_YELLOW_WARNING:
            currentMenu = ADJUSTING_VALUE;
            adjustingIntValue = &settings.yellowWarningSeconds;
            encoder->setPosition(settings.yellowWarningSeconds);
            drawValueAdjustment("YELLOW WARN", settings.yellowWarningSeconds, "sec");
            break;
        case TIMER_RED_WARNING:
            currentMenu = ADJUSTING_VALUE;
            adjustingIntValue = &settings.redWarningSeconds;
            encoder->setPosition(settings.redWarningSeconds);
            drawValueAdjustment("RED WARNING", settings.redWarningSeconds, "sec");
            break;
        case TIMER_BACK:
            currentMenu = MENU_TOP_LEVEL;
            selectedTopItem = 1;
            encoder->setPosition(1);
            drawTopMenu();
            break;
    }
}
void MenuSystem::drawMicSubmenu() {
    tft->fillScreen(TFT_BLACK);
    tft->setTextSize(2);
    tft->setTextColor(TFT_WHITE);
    tft->setCursor(5, 10);
    tft->println("< MICROPHONE");
    
    const char* menuItems[] = {"Monitor", "Threshold", "Back"};
    int startY = 70;
    int boxHeight = 60;
    int spacing = 10;
    
    for (int i = 0; i < MIC_ITEM_COUNT; i++) {
        int y = startY + (i * (boxHeight + spacing));
        
        if (i == selectedMicItem) {
            tft->fillRect(5, y, 160, boxHeight, TFT_BLUE);
            tft->setTextColor(TFT_WHITE);
        } else {
            tft->drawRect(5, y, 160, boxHeight, TFT_DARKGREY);
            tft->setTextColor(TFT_LIGHTGREY);
        }
        
        tft->setTextSize(2);
        tft->setCursor(10, y + 10);
        tft->println(menuItems[i]);
        
        if (i == MIC_THRESHOLD) {
            tft->setTextSize(2);
            tft->setCursor(10, y + 35);
            tft->printf("%.0f", settings.micThreshold);
        }
    }
    
    tft->setTextSize(1);
    tft->setTextColor(TFT_DARKGREY);
    tft->setCursor(15, 290);
    tft->println("Turn: Select");
    tft->setCursor(15, 305);
    tft->println("Press: Confirm");
}

void MenuSystem::executeMicMenuItem(int item) {
    switch(item) {
        case MIC_MONITOR:
            // Enter real-time diagnostic mode
            currentMenu = MIC_DIAGNOSTIC_MODE;
            micDetector.resetStats();
            encoder->setPosition((int)(settings.micThreshold / 50));
            Serial.println("Entered mic diagnostic mode");
            break;
        case MIC_THRESHOLD:
            currentMenu = ADJUSTING_VALUE;
            adjustingFloatValue = &settings.micThreshold;
            encoder->setPosition((int)(settings.micThreshold / 50));
            drawValueAdjustment("MIC THRESH", settings.micThreshold, "");
            break;
        case MIC_BACK:
            currentMenu = MENU_TOP_LEVEL;
            selectedTopItem = 3;  // Position on "Microphone"
            encoder->setPosition(3);
            drawTopMenu();
            break;
    }
}
void MenuSystem::executeDisplayMenuItem(int item) {
    switch(item) {
        case DISPLAY_BRIGHTNESS:
            currentMenu = ADJUSTING_VALUE;
            adjustingIntValue = &settings.displayBrightness;
            encoder->setPosition(settings.displayBrightness / 10);
            drawValueAdjustment("BRIGHTNESS", settings.displayBrightness, "");
            break;
        case DISPLAY_LED_BRIGHTNESS:
            currentMenu = ADJUSTING_VALUE;
            adjustingIntValue = &settings.ledBrightness;
            encoder->setPosition(settings.ledBrightness / 5);
            drawValueAdjustment("LED BRIGHTNESS", settings.ledBrightness, "");
            break;
        case DISPLAY_BUZZER_VOLUME:  // <-- Add this
            currentMenu = ADJUSTING_VALUE;
            adjustingIntValue = &settings.buzzerVolume;
            encoder->setPosition(settings.buzzerVolume / 5);
            drawValueAdjustment("BUZZER VOL", settings.buzzerVolume, "%");
            break;
        case DISPLAY_BACK:
            currentMenu = MENU_TOP_LEVEL;
            selectedTopItem = 2;
            encoder->setPosition(2);
            drawTopMenu();
            break;
    }
}