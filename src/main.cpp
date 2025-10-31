#include <Arduino.h>
#include <Wire.h>
#include <LovyanGFX.hpp>
#include "pin_config.h"
#include <SensorQMI8658.hpp>
#include <FastLED.h>
#include <RotaryEncoder.h>
#include <Preferences.h>

#define USBSerial Serial

CRGB leds[NUM_LEDS];
Preferences preferences;

// LovyanGFX for ST7789 display
class LGFX : public lgfx::LGFX_Device
{
    lgfx::Panel_ST7789 _panel_instance;
    lgfx::Bus_SPI _bus_instance;
    lgfx::Light_PWM _light_instance;

public:
    LGFX(void)
    {
        {
            auto cfg = _bus_instance.config();
            
            cfg.spi_host = SPI2_HOST;
            cfg.spi_mode = 0;
            cfg.freq_write = 80000000;
            cfg.freq_read = 16000000;
            cfg.spi_3wire = false;
            cfg.use_lock = true;
            cfg.dma_channel = SPI_DMA_CH_AUTO;
            
            cfg.pin_sclk = LCD_SCLK;
            cfg.pin_mosi = LCD_MOSI;
            cfg.pin_miso = -1;
            cfg.pin_dc = LCD_DC;
            
            _bus_instance.config(cfg);
            _panel_instance.setBus(&_bus_instance);
        }

        {
            auto cfg = _panel_instance.config();
            
            cfg.pin_cs = LCD_CS;
            cfg.pin_rst = LCD_RST;
            cfg.pin_busy = -1;
            
            cfg.panel_width = 170;
            cfg.panel_height = 320;
            cfg.offset_x = 35;
            cfg.offset_y = 0;
            cfg.offset_rotation = 0;
            cfg.dummy_read_pixel = 8;
            cfg.dummy_read_bits = 1;
            cfg.readable = false;
            cfg.invert = true;
            cfg.rgb_order = false;
            cfg.dlen_16bit = false;
            cfg.bus_shared = false;
            
            _panel_instance.config(cfg);
        }

        {
            auto cfg = _light_instance.config();
            
            cfg.pin_bl = LCD_BL;
            cfg.invert = false;
            cfg.freq = 44100;
            cfg.pwm_channel = 0;
            
            _light_instance.config(cfg);
            _panel_instance.setLight(&_light_instance);
        }

        setPanel(&_panel_instance);
    }
};

LGFX tft;
SensorQMI8658 qmi;
RotaryEncoder encoder(ENCODER_CLK, ENCODER_DT, RotaryEncoder::LatchMode::FOUR3);

// IMU data structure
struct {
    float x;
    float y;
    float z;
} acc;

// Calibration: Store gravity vector when level
struct {
    float x;
    float y;
    float z;
    float magnitude;
} gravityRef;

bool calibrated = false;

// Display update variables
unsigned long lastUpdate = 0;
uint16_t lastStatusColor = 0;
float lastDisplayedAngle = 999;
bool firstDraw = true;

// Color definitions
#define COLOR_RED    0xF800
#define COLOR_GREEN  0x07E0
#define COLOR_BLUE   0x001F
#define COLOR_CYAN   0x07FF
#define COLOR_YELLOW 0xFFE0
#define COLOR_WHITE  0xFFFF
#define COLOR_ORANGE 0xFD20

// Menu states
enum MenuState {
    MAIN_DISPLAY,
    MENU_TOP_LEVEL,
    MENU_LEVEL_SUBMENU,
    MENU_DISPLAY_SUBMENU,
    MENU_TIMER_SUBMENU,
    ADJUSTING_VALUE
};

enum TopMenuItem {
    TOP_LEVEL,
    TOP_TIMER,
    TOP_DISPLAY,
    TOP_EXIT,
    TOP_ITEM_COUNT
};

enum LevelSubItem {
    LEVEL_CALIBRATE,
    LEVEL_TOLERANCE,
    LEVEL_HYSTERESIS,
    LEVEL_BACK,
    LEVEL_ITEM_COUNT
};

enum DisplaySubItem {
    DISPLAY_BRIGHTNESS,
    DISPLAY_LED_BRIGHTNESS,
    DISPLAY_BACK,
    DISPLAY_ITEM_COUNT
};

enum TimerSubItem {
    TIMER_PAR_TIME,
    TIMER_YELLOW_WARNING,
    TIMER_RED_WARNING,
    TIMER_BACK,
    TIMER_ITEM_COUNT
};

MenuState currentMenu = MAIN_DISPLAY;
int selectedTopItem = 0;
int selectedLevelItem = 0;
int selectedDisplayItem = 0;
int selectedTimerItem = 0;

// Timer states
enum TimerState {
    TIMER_IDLE,
    TIMER_READY,
    TIMER_RUNNING,
    TIMER_FINISHED
};

TimerState timerState = TIMER_IDLE;
unsigned long timerStartMillis = 0;
unsigned long buttonPressStartMillis = 0;
bool longPressDetected = false;

// Settings (loaded from/saved to flash)
float settingTolerance = 0.5;
float settingHysteresis = 0.1;
int displayBrightness = 255;
int ledBrightness = 50;
float settingAlpha = 0.2;

// Timer settings
int parTimeSeconds = 60;
int yellowWarningSeconds = 30;
int redWarningSeconds = 10;

// Current state
uint16_t currentLevelColor = COLOR_GREEN;
float displayedAngle = 0;

// Value adjustment tracking
float* adjustingFloatValue = nullptr;
int* adjustingIntValue = nullptr;
float adjustStep = 0.1;
int adjustIntStep = 10;

// Encoder interrupt
void IRAM_ATTR checkEncoder() {
    encoder.tick();
}

// Load settings from flash
void loadSettings() {
    preferences.begin("stage_timer", false);
    
    settingTolerance = preferences.getFloat("tolerance", 0.5);
    settingHysteresis = preferences.getFloat("hysteresis", 0.1);
    displayBrightness = preferences.getInt("disp_bright", 255);
    ledBrightness = preferences.getInt("led_bright", 50);
    settingAlpha = preferences.getFloat("alpha", 0.2);
    
    // Timer settings
    parTimeSeconds = preferences.getInt("par_time", 60);
    yellowWarningSeconds = preferences.getInt("yellow_warn", 30);
    redWarningSeconds = preferences.getInt("red_warn", 10);
    
    // Load calibration if it exists
    if (preferences.getBool("calibrated", false)) {
        gravityRef.x = preferences.getFloat("grav_x", 0);
        gravityRef.y = preferences.getFloat("grav_y", 0);
        gravityRef.z = preferences.getFloat("grav_z", 0);
        gravityRef.magnitude = preferences.getFloat("grav_mag", 1.0);
        calibrated = true;
        USBSerial.println("Loaded calibration from flash");
    }
    
    preferences.end();
    
    USBSerial.println("Settings loaded:");
    USBSerial.print("  Par Time: "); USBSerial.println(parTimeSeconds);
    USBSerial.print("  Yellow Warning: "); USBSerial.println(yellowWarningSeconds);
    USBSerial.print("  Red Warning: "); USBSerial.println(redWarningSeconds);
}

// Save settings to flash
void saveSettings() {
    preferences.begin("stage_timer", false);
    
    preferences.putFloat("tolerance", settingTolerance);
    preferences.putFloat("hysteresis", settingHysteresis);
    preferences.putInt("disp_bright", displayBrightness);
    preferences.putInt("led_bright", ledBrightness);
    preferences.putFloat("alpha", settingAlpha);
    
    preferences.putInt("par_time", parTimeSeconds);
    preferences.putInt("yellow_warn", yellowWarningSeconds);
    preferences.putInt("red_warn", redWarningSeconds);
    
    preferences.end();
    
    USBSerial.println("Settings saved to flash");
}

// Save calibration to flash
void saveCalibration() {
    preferences.begin("stage_timer", false);
    
    preferences.putBool("calibrated", true);
    preferences.putFloat("grav_x", gravityRef.x);
    preferences.putFloat("grav_y", gravityRef.y);
    preferences.putFloat("grav_z", gravityRef.z);
    preferences.putFloat("grav_mag", gravityRef.magnitude);
    
    preferences.end();
    
    USBSerial.println("Calibration saved to flash");
}

void performCalibration() {
    USBSerial.println("\n*** CALIBRATION ***");
    USBSerial.println("Hold board LEVEL (horizontal)");
    
    // Take average of 100 readings
    float xSum = 0;
    float ySum = 0;
    float zSum = 0;
    
    for(int i = 0; i < 100; i++) {
        if (qmi.getDataReady()) {
            qmi.getAccelerometer(acc.x, acc.y, acc.z);
            xSum += acc.x;
            ySum += acc.y;
            zSum += acc.z;
        }
        delay(10);
    }
    
    gravityRef.x = xSum / 100.0;
    gravityRef.y = ySum / 100.0;
    gravityRef.z = zSum / 100.0;
    gravityRef.magnitude = sqrt(gravityRef.x * gravityRef.x + 
                               gravityRef.y * gravityRef.y + 
                               gravityRef.z * gravityRef.z);
    
    calibrated = true;
    saveCalibration();
    
    USBSerial.println("Calibration complete!");
}

float calculateSimpleTiltAngle() {
    float xCal = acc.x - gravityRef.x;
    float angle = atan2(xCal, -acc.y) * 180.0 / PI;
    return angle;
}

void drawProgressBar(int x, int y, int width, int height, float percentage, uint16_t color) {
    // Draw border
    tft.drawRect(x, y, width, height, TFT_DARKGREY);
    
    // Fill bar based on percentage
    int fillWidth = (int)((width - 4) * percentage);
    if (fillWidth > 0) {
        tft.fillRect(x + 2, y + 2, fillWidth, height - 4, color);
    }
    
    // Clear remaining area
    int remainWidth = (width - 4) - fillWidth;
    if (remainWidth > 0) {
        tft.fillRect(x + 2 + fillWidth, y + 2, remainWidth, height - 4, TFT_BLACK);
    }
}

void drawTimerDisplay(int remainingSeconds) {
    // Timer area starts at y=107, height = 213
    int timerY = 107;
    
    // Determine color based on remaining time
    uint16_t timerColor;
    if (remainingSeconds > yellowWarningSeconds) {
        timerColor = COLOR_WHITE;
    } else if (remainingSeconds > redWarningSeconds) {
        timerColor = COLOR_YELLOW;
    } else {
        timerColor = COLOR_RED;
    }
    
    // Clear timer area
    static int lastDrawnSeconds = -1;
    if (remainingSeconds != lastDrawnSeconds) {
        tft.fillRect(0, timerY, 170, 213, TFT_BLACK);
        lastDrawnSeconds = remainingSeconds;
    }
    
    // Draw state text at top
    tft.setTextSize(1);
    tft.setTextColor(TFT_DARKGREY);
    tft.setCursor(10, timerY + 10);
    if (timerState == TIMER_IDLE) {
        tft.println("Long press to ready");
    } else if (timerState == TIMER_READY) {
        tft.setTextColor(COLOR_GREEN);
        tft.println("READY - Press to start");
    } else if (timerState == TIMER_FINISHED) {
        tft.setTextColor(COLOR_RED);
        tft.println("TIME!");
    }
    
    // Draw large time display
    tft.setTextSize(5);
    tft.setTextColor(timerColor);
    
    int minutes = remainingSeconds / 60;
    int seconds = remainingSeconds % 60;
    
    char timeStr[10];
    sprintf(timeStr, "%d:%02d", minutes, seconds);
    
    // Center the time
    int textWidth = strlen(timeStr) * 30;
    int x = (170 - textWidth) / 2;
    tft.setCursor(x, timerY + 50);
    tft.print(timeStr);
    
    // Draw progress bar
    float percentage = (float)remainingSeconds / (float)parTimeSeconds;
    drawProgressBar(10, timerY + 130, 150, 30, percentage, timerColor);
    
    // Draw par time reference
    tft.setTextSize(1);
    tft.setTextColor(TFT_DARKGREY);
    tft.setCursor(10, timerY + 175);
    tft.print("Par: ");
    tft.print(parTimeSeconds);
    tft.print("s");
}

void drawTopMenu() {
    tft.fillScreen(TFT_BLACK);
    
    // Title
    tft.setTextSize(2);
    tft.setTextColor(TFT_WHITE);
    tft.setCursor(25, 15);
    tft.println("SETTINGS");
    
    // Menu items
    const char* menuItems[] = {
        "Level",
        "Timer",
        "Display",
        "Exit"
    };
    
    int startY = 60;
    int boxHeight = 45;
    int spacing = 8;
    
    for (int i = 0; i < TOP_ITEM_COUNT; i++) {
        int y = startY + (i * (boxHeight + spacing));
        
        // Draw box
        if (i == selectedTopItem) {
            tft.fillRect(5, y, 160, boxHeight, TFT_BLUE);
            tft.setTextColor(TFT_WHITE);
        } else {
            tft.drawRect(5, y, 160, boxHeight, TFT_DARKGREY);
            tft.setTextColor(TFT_LIGHTGREY);
        }
        
        // Center text
        tft.setTextSize(2);
        tft.setCursor(45, y + 14);
        tft.println(menuItems[i]);
    }
    
    // Instructions
    tft.setTextSize(1);
    tft.setTextColor(TFT_DARKGREY);
    tft.setCursor(15, 295);
    tft.println("Turn: Select");
    tft.setCursor(15, 310);
    tft.println("Press: Confirm");
}

void drawLevelSubmenu() {
    tft.fillScreen(TFT_BLACK);
    
    // Title
    tft.setTextSize(2);
    tft.setTextColor(TFT_WHITE);
    tft.setCursor(5, 10);
    tft.println("< LEVEL");
    
    const char* menuItems[] = {
        "Calibrate",
        "Tolerance",
        "Hysteresis",
        "Back"
    };
    
    int startY = 50;
    int boxHeight = 50;
    int spacing = 8;
    
    for (int i = 0; i < LEVEL_ITEM_COUNT; i++) {
        int y = startY + (i * (boxHeight + spacing));
        
        if (i == selectedLevelItem) {
            tft.fillRect(5, y, 160, boxHeight, TFT_BLUE);
            tft.setTextColor(TFT_WHITE);
        } else {
            tft.drawRect(5, y, 160, boxHeight, TFT_DARKGREY);
            tft.setTextColor(TFT_LIGHTGREY);
        }
        
        tft.setTextSize(2);
        tft.setCursor(10, y + 5);
        tft.println(menuItems[i]);
        
        if (i == LEVEL_TOLERANCE) {
            tft.setTextSize(2);
            tft.setCursor(10, y + 27);
            tft.print(settingTolerance, 1);
            tft.print(" deg");
        } else if (i == LEVEL_HYSTERESIS) {
            tft.setTextSize(2);
            tft.setCursor(10, y + 27);
            tft.print(settingHysteresis, 2);
            tft.print(" deg");
        }
    }
    
    tft.setTextSize(1);
    tft.setTextColor(TFT_DARKGREY);
    tft.setCursor(15, 295);
    tft.println("Turn: Select");
    tft.setCursor(15, 310);
    tft.println("Press: Confirm");
}

void drawTimerSubmenu() {
    tft.fillScreen(TFT_BLACK);
    
    // Title
    tft.setTextSize(2);
    tft.setTextColor(TFT_WHITE);
    tft.setCursor(5, 10);
    tft.println("< TIMER");
    
    const char* menuItems[] = {
        "Par Time",
        "Yellow Warn",
        "Red Warning",
        "Back"
    };
    
    int startY = 50;
    int boxHeight = 50;
    int spacing = 8;
    
    for (int i = 0; i < TIMER_ITEM_COUNT; i++) {
        int y = startY + (i * (boxHeight + spacing));
        
        if (i == selectedTimerItem) {
            tft.fillRect(5, y, 160, boxHeight, TFT_BLUE);
            tft.setTextColor(TFT_WHITE);
        } else {
            tft.drawRect(5, y, 160, boxHeight, TFT_DARKGREY);
            tft.setTextColor(TFT_LIGHTGREY);
        }
        
        tft.setTextSize(2);
        tft.setCursor(10, y + 5);
        tft.println(menuItems[i]);
        
        if (i == TIMER_PAR_TIME) {
            tft.setTextSize(2);
            tft.setCursor(10, y + 27);
            tft.print(parTimeSeconds);
            tft.print(" sec");
        } else if (i == TIMER_YELLOW_WARNING) {
            tft.setTextSize(2);
            tft.setCursor(10, y + 27);
            tft.print(yellowWarningSeconds);
            tft.print(" sec");
        } else if (i == TIMER_RED_WARNING) {
            tft.setTextSize(2);
            tft.setCursor(10, y + 27);
            tft.print(redWarningSeconds);
            tft.print(" sec");
        }
    }
    
    tft.setTextSize(1);
    tft.setTextColor(TFT_DARKGREY);
    tft.setCursor(15, 295);
    tft.println("Turn: Select");
    tft.setCursor(15, 310);
    tft.println("Press: Confirm");
}

void drawDisplaySubmenu() {
    tft.fillScreen(TFT_BLACK);
    
    // Title
    tft.setTextSize(2);
    tft.setTextColor(TFT_WHITE);
    tft.setCursor(5, 10);
    tft.println("< DISPLAY");
    
    const char* menuItems[] = {
        "Brightness",
        "LED Bright",
        "Back"
    };
    
    int startY = 60;
    int boxHeight = 50;
    int spacing = 10;
    
    for (int i = 0; i < DISPLAY_ITEM_COUNT; i++) {
        int y = startY + (i * (boxHeight + spacing));
        
        if (i == selectedDisplayItem) {
            tft.fillRect(5, y, 160, boxHeight, TFT_BLUE);
            tft.setTextColor(TFT_WHITE);
        } else {
            tft.drawRect(5, y, 160, boxHeight, TFT_DARKGREY);
            tft.setTextColor(TFT_LIGHTGREY);
        }
        
        tft.setTextSize(2);
        tft.setCursor(10, y + 5);
        tft.println(menuItems[i]);
        
        if (i == DISPLAY_BRIGHTNESS) {
            tft.setTextSize(2);
            tft.setCursor(10, y + 27);
            tft.print(displayBrightness);
        } else if (i == DISPLAY_LED_BRIGHTNESS) {
            tft.setTextSize(2);
            tft.setCursor(10, y + 27);
            tft.print(ledBrightness);
        }
    }
    
    tft.setTextSize(1);
    tft.setTextColor(TFT_DARKGREY);
    tft.setCursor(15, 290);
    tft.println("Turn: Select");
    tft.setCursor(15, 305);
    tft.println("Press: Confirm");
}

void drawValueAdjustment(const char* label, float value, const char* unit) {
    tft.fillScreen(TFT_BLACK);
    
    tft.setTextSize(2);
    tft.setTextColor(TFT_WHITE);
    tft.setCursor(5, 15);
    tft.print("< ");
    tft.println(label);
    
    tft.drawRect(5, 80, 160, 90, TFT_WHITE);
    
    tft.setTextSize(4);
    tft.setTextColor(COLOR_CYAN);
    tft.setCursor(15, 100);
    tft.print(value, 2);
    
    tft.setTextSize(2);
    tft.setCursor(15, 140);
    tft.print(unit);
    
    tft.setTextSize(1);
    tft.setTextColor(TFT_DARKGREY);
    tft.setCursor(15, 250);
    tft.println("Turn: Adjust");
    tft.setCursor(15, 265);
    tft.println("Press: Save & Exit");
}

void drawValueAdjustment(const char* label, int value, const char* unit) {
    tft.fillScreen(TFT_BLACK);
    
    tft.setTextSize(2);
    tft.setTextColor(TFT_WHITE);
    tft.setCursor(5, 15);
    tft.print("< ");
    tft.println(label);
    
    tft.drawRect(5, 80, 160, 90, TFT_WHITE);
    
    tft.setTextSize(4);
    tft.setTextColor(COLOR_CYAN);
    tft.setCursor(40, 110);
    tft.print(value);
    
    tft.setTextSize(2);
    tft.setCursor(15, 140);
    tft.print(unit);
    
    int barWidth = map(constrain(value, 0, 255), 0, 255, 0, 150);
    tft.fillRect(10, 190, barWidth, 15, COLOR_GREEN);
    tft.drawRect(10, 190, 150, 15, TFT_WHITE);
    
    tft.setTextSize(1);
    tft.setTextColor(TFT_DARKGREY);
    tft.setCursor(15, 250);
    tft.println("Turn: Adjust");
    tft.setCursor(15, 265);
    tft.println("Press: Save & Exit");
}

void executeTopMenuItem(int item) {
    switch(item) {
        case TOP_LEVEL:
            currentMenu = MENU_LEVEL_SUBMENU;
            selectedLevelItem = 0;
            encoder.setPosition(0);
            drawLevelSubmenu();
            break;
            
        case TOP_TIMER:
            currentMenu = MENU_TIMER_SUBMENU;
            selectedTimerItem = 0;
            encoder.setPosition(0);
            drawTimerSubmenu();
            break;
            
        case TOP_DISPLAY:
            currentMenu = MENU_DISPLAY_SUBMENU;
            selectedDisplayItem = 0;
            encoder.setPosition(0);
            drawDisplaySubmenu();
            break;
            
        case TOP_EXIT:
            saveSettings();
            currentMenu = MAIN_DISPLAY;
            lastStatusColor = 0xFFFF;
            lastDisplayedAngle = 999;
            firstDraw = true;
            timerState = TIMER_IDLE;
            
            tft.setBrightness(displayBrightness);
            FastLED.setBrightness(ledBrightness);
            
            USBSerial.println("Exiting to main display");
            break;
    }
}

void executeLevelMenuItem(int item) {
    switch(item) {
        case LEVEL_CALIBRATE:
            tft.fillScreen(COLOR_CYAN);
            tft.setTextColor(TFT_BLACK);
            tft.setTextSize(1);
            tft.setCursor(10, 100);
            tft.println("CALIBRATING...");
            tft.setCursor(10, 120);
            tft.println("Hold LEVEL");
            
            delay(1000);
            performCalibration();
            
            tft.fillScreen(COLOR_GREEN);
            tft.setTextSize(2);
            tft.setCursor(30, 140);
            tft.println("DONE!");
            delay(1500);
            
            drawLevelSubmenu();
            break;
            
        case LEVEL_TOLERANCE:
            currentMenu = ADJUSTING_VALUE;
            adjustingFloatValue = &settingTolerance;
            adjustStep = 0.1;
            encoder.setPosition((int)(settingTolerance * 10));
            drawValueAdjustment("TOLERANCE", settingTolerance, "deg");
            break;
            
        case LEVEL_HYSTERESIS:
            currentMenu = ADJUSTING_VALUE;
            adjustingFloatValue = &settingHysteresis;
            adjustStep = 0.01;
            encoder.setPosition((int)(settingHysteresis * 100));
            drawValueAdjustment("HYSTERESIS", settingHysteresis, "deg");
            break;
            
        case LEVEL_BACK:
            currentMenu = MENU_TOP_LEVEL;
            selectedTopItem = 0;
            encoder.setPosition(0);
            drawTopMenu();
            break;
    }
}

void executeTimerMenuItem(int item) {
    switch(item) {
        case TIMER_PAR_TIME:
            currentMenu = ADJUSTING_VALUE;
            adjustingIntValue = &parTimeSeconds;
            adjustIntStep = 1;
            encoder.setPosition(parTimeSeconds);
            drawValueAdjustment("PAR TIME", parTimeSeconds, "sec");
            break;
            
        case TIMER_YELLOW_WARNING:
            currentMenu = ADJUSTING_VALUE;
            adjustingIntValue = &yellowWarningSeconds;
            adjustIntStep = 1;
            encoder.setPosition(yellowWarningSeconds);
            drawValueAdjustment("YELLOW WARN", yellowWarningSeconds, "sec");
            break;
            
        case TIMER_RED_WARNING:
            currentMenu = ADJUSTING_VALUE;
            adjustingIntValue = &redWarningSeconds;
            adjustIntStep = 1;
            encoder.setPosition(redWarningSeconds);
            drawValueAdjustment("RED WARNING", redWarningSeconds, "sec");
            break;
            
        case TIMER_BACK:
            currentMenu = MENU_TOP_LEVEL;
            selectedTopItem = 1;
            encoder.setPosition(1);
            drawTopMenu();
            break;
    }
}

void executeDisplayMenuItem(int item) {
    switch(item) {
        case DISPLAY_BRIGHTNESS:
            currentMenu = ADJUSTING_VALUE;
            adjustingIntValue = &displayBrightness;
            adjustIntStep = 10;
            encoder.setPosition(displayBrightness / 10);
            drawValueAdjustment("BRIGHTNESS", displayBrightness, "");
            break;
            
        case DISPLAY_LED_BRIGHTNESS:
            currentMenu = ADJUSTING_VALUE;
            adjustingIntValue = &ledBrightness;
            adjustIntStep = 5;
            encoder.setPosition(ledBrightness / 5);
            drawValueAdjustment("LED BRIGHTNESS", ledBrightness, "");
            break;
            
        case DISPLAY_BACK:
            currentMenu = MENU_TOP_LEVEL;
            selectedTopItem = 2;
            encoder.setPosition(2);
            drawTopMenu();
            break;
    }
}

void updateMainDisplay() {
    // Read accelerometer
    if (qmi.getDataReady()) {
        qmi.getAccelerometer(acc.x, acc.y, acc.z);
    }

    // Calculate angles
    float rawAngle = calculateSimpleTiltAngle();
    displayedAngle = settingAlpha * rawAngle + (1.0 - settingAlpha) * displayedAngle;

    // Hysteresis state logic
    uint16_t newColor;
    CRGB ledColor;
    const char* status;
    
    if (currentLevelColor == COLOR_GREEN) {
        if (rawAngle > (settingTolerance + settingHysteresis)) {
            newColor = COLOR_RED;
            ledColor = CRGB::Red;
            status = "CW";
        } else if (rawAngle < -(settingTolerance + settingHysteresis)) {
            newColor = COLOR_BLUE;
            ledColor = CRGB::Blue;
            status = "CCW";
        } else {
            newColor = COLOR_GREEN;
            ledColor = CRGB::Green;
            status = "LEVEL";
        }
    } else if (currentLevelColor == COLOR_RED) {
        if (rawAngle < (settingTolerance - settingHysteresis)) {
            newColor = COLOR_GREEN;
            ledColor = CRGB::Green;
            status = "LEVEL";
        } else if (rawAngle < -(settingTolerance + settingHysteresis)) {
            newColor = COLOR_BLUE;
            ledColor = CRGB::Blue;
            status = "CCW";
        } else {
            newColor = COLOR_RED;
            ledColor = CRGB::Red;
            status = "CW";
        }
    } else {
        if (rawAngle > -(settingTolerance - settingHysteresis)) {
            newColor = COLOR_GREEN;
            ledColor = CRGB::Green;
            status = "LEVEL";
        } else if (rawAngle > (settingTolerance + settingHysteresis)) {
            newColor = COLOR_RED;
            ledColor = CRGB::Red;
            status = "CW";
        } else {
            newColor = COLOR_BLUE;
            ledColor = CRGB::Blue;
            status = "CCW";
        }
    }

    if (newColor != currentLevelColor) {
        currentLevelColor = newColor;
        lastStatusColor = 0xFFFF;
    }

    // Update display at 10Hz
    if (millis() - lastUpdate > 100) {
        lastUpdate = millis();
        
        // Update level indicator (top 1/3)
        if (currentLevelColor != lastStatusColor || firstDraw) {
            tft.fillRect(0, 0, 170, 107, currentLevelColor);
            
            tft.setTextSize(5);
            if (currentLevelColor == COLOR_GREEN || currentLevelColor == COLOR_CYAN) {
                tft.setTextColor(TFT_BLACK);
            } else {
                tft.setTextColor(TFT_WHITE);
            }
            
            char buf[10];
            sprintf(buf, "%.1f", displayedAngle);
            int textWidth = strlen(buf) * 30;
            int x = (170 - textWidth) / 2;
            
            tft.setCursor(x, 15);
            tft.print(displayedAngle, 1);
            
            tft.setTextSize(2);
            int statusWidth = strlen(status) * 12;
            tft.setCursor((170 - statusWidth) / 2, 80);
            tft.println(status);
            
            lastStatusColor = currentLevelColor;
            lastDisplayedAngle = displayedAngle;
        } else if (abs(displayedAngle - lastDisplayedAngle) > 0.05) {
            tft.fillRect(0, 10, 170, 60, currentLevelColor);
            
            tft.setTextSize(5);
            if (currentLevelColor == COLOR_GREEN || currentLevelColor == COLOR_CYAN) {
                tft.setTextColor(TFT_BLACK);
            } else {
                tft.setTextColor(TFT_WHITE);
            }
            
            char buf[10];
            sprintf(buf, "%.1f", displayedAngle);
            int textWidth = strlen(buf) * 30;
            int x = (170 - textWidth) / 2;
            
            tft.setCursor(x, 15);
            tft.print(displayedAngle, 1);
            
            lastDisplayedAngle = displayedAngle;
        }
        
        // Update timer display (lower 2/3)
        int remainingSeconds;
        if (timerState == TIMER_RUNNING) {
            unsigned long elapsedMillis = millis() - timerStartMillis;
            int elapsedSeconds = elapsedMillis / 1000;
            remainingSeconds = parTimeSeconds - elapsedSeconds;
            
            if (remainingSeconds <= 0) {
                remainingSeconds = 0;
                timerState = TIMER_FINISHED;
                // Beep or flash LED
                leds[0] = CRGB::Red;
                FastLED.show();
            }
        } else {
            remainingSeconds = parTimeSeconds;
        }
        
        drawTimerDisplay(remainingSeconds);
        
        // Update LED for level indicator (unless timer finished)
        if (timerState != TIMER_FINISHED) {
            leds[0] = ledColor;
            FastLED.show();
        }
        
        firstDraw = false;
    }
}

void setup() {
    USBSerial.begin(115200);
    delay(2000);
    
    USBSerial.println("\n================================");
    USBSerial.println("=== Stage Timer v2.5 ===");
    USBSerial.println("=== With Countdown Timer ===");
    USBSerial.println("================================");

    loadSettings();

    // RGB LED
    USBSerial.println("Initializing RGB LED...");
    FastLED.addLeds<WS2812B, RGB_LED_PIN, RGB>(leds, NUM_LEDS);
    FastLED.setBrightness(ledBrightness);
    leds[0] = CRGB::Purple;
    FastLED.show();
    USBSerial.println("RGB LED: OK!");

    // Display
    USBSerial.println("Initializing display...");
    tft.init();
    tft.setRotation(0);
    tft.setBrightness(displayBrightness);
    tft.fillScreen(TFT_BLACK);
    USBSerial.println("Display: OK!");

    // IMU
    USBSerial.println("Initializing IMU...");
    Wire.begin(IIC_SDA, IIC_SCL);
    
    if (!qmi.begin(Wire, QMI8658_L_SLAVE_ADDRESS, IIC_SDA, IIC_SCL)) {
        USBSerial.println("ERROR: IMU not found!");
        tft.fillScreen(COLOR_RED);
        tft.setTextColor(TFT_WHITE);
        tft.setTextSize(2);
        tft.setCursor(10, 140);
        tft.println("IMU ERROR");
        while(1) delay(1000);
    }
    
    qmi.configAccelerometer(
        SensorQMI8658::ACC_RANGE_4G,
        SensorQMI8658::ACC_ODR_1000Hz,
        SensorQMI8658::LPF_MODE_3,
        true
    );
    qmi.enableAccelerometer();
    USBSerial.println("IMU: OK!");

    // Rotary Encoder
    USBSerial.println("Initializing encoder...");
    pinMode(ENCODER_CLK, INPUT_PULLUP);
    pinMode(ENCODER_DT, INPUT_PULLUP);
    pinMode(ENCODER_SW, INPUT_PULLUP);
    
    attachInterrupt(digitalPinToInterrupt(ENCODER_CLK), checkEncoder, CHANGE);
    attachInterrupt(digitalPinToInterrupt(ENCODER_DT), checkEncoder, CHANGE);
    USBSerial.println("Encoder: OK!");

    pinMode(BOOT_BUTTON, INPUT_PULLUP);

    if (!calibrated) {
        tft.fillScreen(COLOR_CYAN);
        tft.setTextColor(TFT_BLACK);
        tft.setTextSize(1);
        tft.setCursor(10, 100);
        tft.println("CALIBRATION NEEDED");
        tft.setCursor(10, 120);
        tft.println("Hold board LEVEL");
        tft.setCursor(10, 150);
        tft.println("Press BOOT button");
        
        USBSerial.println("\nWaiting for BOOT button...");
        
        while(digitalRead(BOOT_BUTTON) == HIGH) {
            delay(10);
        }
        delay(200);
        while(digitalRead(BOOT_BUTTON) == LOW) delay(10);
        
        performCalibration();
    }
    
    tft.fillScreen(COLOR_GREEN);
    tft.setTextColor(TFT_BLACK);
    tft.setTextSize(3);
    tft.setCursor(15, 140);
    tft.println("READY!");
    
    leds[0] = CRGB::Green;
    FastLED.show();
    delay(1500);
    
    USBSerial.println("\n=== READY! ===\n");
    lastStatusColor = 0xFFFF;
}

void loop() {
    // Handle encoder button for timer control AND menu
    static bool lastButtonState = HIGH;
    static unsigned long lastButtonTime = 0;
    bool buttonState = digitalRead(ENCODER_SW);
    
    // Detect button press
    if (buttonState == LOW && lastButtonState == HIGH) {
        buttonPressStartMillis = millis();
        longPressDetected = false;
    }
    
    // Detect long press (>1 second) in main display for timer ready
    if (buttonState == LOW && !longPressDetected && currentMenu == MAIN_DISPLAY) {
        if (millis() - buttonPressStartMillis > 1000) {
            longPressDetected = true;
            if (timerState == TIMER_IDLE || timerState == TIMER_FINISHED) {
                timerState = TIMER_READY;
                USBSerial.println("Timer READY");
                leds[0] = CRGB::Yellow;
                FastLED.show();
            }
        }
    }
    
    // Detect button release (short press)
    if (buttonState == HIGH && lastButtonState == LOW && (millis() - buttonPressStartMillis > 50)) {
        if (!longPressDetected) {
            // Short press
            if (currentMenu == MAIN_DISPLAY) {
                if (timerState == TIMER_READY) {
                    // Start timer
                    timerState = TIMER_RUNNING;
                    timerStartMillis = millis();
                    USBSerial.println("Timer STARTED");
                } else if (timerState == TIMER_RUNNING || timerState == TIMER_FINISHED) {
                    // Reset timer
                    timerState = TIMER_IDLE;
                    USBSerial.println("Timer RESET");
                } else {
                    // Enter menu
                    currentMenu = MENU_TOP_LEVEL;
                    selectedTopItem = 0;
                    encoder.setPosition(0);
                    drawTopMenu();
                    USBSerial.println("Entered top menu");
                }
            } else if (currentMenu == MENU_TOP_LEVEL) {
                executeTopMenuItem(selectedTopItem);
            } else if (currentMenu == MENU_LEVEL_SUBMENU) {
                executeLevelMenuItem(selectedLevelItem);
            } else if (currentMenu == MENU_TIMER_SUBMENU) {
                executeTimerMenuItem(selectedTimerItem);
            } else if (currentMenu == MENU_DISPLAY_SUBMENU) {
                executeDisplayMenuItem(selectedDisplayItem);
            } else if (currentMenu == ADJUSTING_VALUE) {
                // Save and return
                if (adjustingFloatValue != nullptr) {
                    if (adjustingFloatValue == &settingTolerance) {
                        currentMenu = MENU_LEVEL_SUBMENU;
                        drawLevelSubmenu();
                    } else if (adjustingFloatValue == &settingHysteresis) {
                        currentMenu = MENU_LEVEL_SUBMENU;
                        drawLevelSubmenu();
                    }
                    adjustingFloatValue = nullptr;
                } else if (adjustingIntValue != nullptr) {
                    if (adjustingIntValue == &displayBrightness) {
                        tft.setBrightness(displayBrightness);
                        currentMenu = MENU_DISPLAY_SUBMENU;
                        drawDisplaySubmenu();
                    } else if (adjustingIntValue == &ledBrightness) {
                        FastLED.setBrightness(ledBrightness);
                        currentMenu = MENU_DISPLAY_SUBMENU;
                        drawDisplaySubmenu();
                    } else if (adjustingIntValue == &parTimeSeconds) {
                        currentMenu = MENU_TIMER_SUBMENU;
                        drawTimerSubmenu();
                    } else if (adjustingIntValue == &yellowWarningSeconds) {
                        currentMenu = MENU_TIMER_SUBMENU;
                        drawTimerSubmenu();
                    } else if (adjustingIntValue == &redWarningSeconds) {
                        currentMenu = MENU_TIMER_SUBMENU;
                        drawTimerSubmenu();
                    }
                    adjustingIntValue = nullptr;
                }
                saveSettings();
                USBSerial.println("Value saved");
            }
        }
    }
    lastButtonState = buttonState;
    
    // Handle encoder rotation
    static int lastEncoderPos = 0;
    int newPos = encoder.getPosition();
    
    if (newPos != lastEncoderPos) {
        int delta = newPos - lastEncoderPos;
        
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
        } else if (currentMenu == ADJUSTING_VALUE) {
            // Adjust values
            if (adjustingFloatValue != nullptr) {
                if (adjustingFloatValue == &settingTolerance) {
                    *adjustingFloatValue = newPos * 0.1;
                    *adjustingFloatValue = constrain(*adjustingFloatValue, 0.1, 5.0);
                    drawValueAdjustment("TOLERANCE", *adjustingFloatValue, "deg");
                } else if (adjustingFloatValue == &settingHysteresis) {
                    *adjustingFloatValue = newPos * 0.01;
                    *adjustingFloatValue = constrain(*adjustingFloatValue, 0.01, 1.0);
                    drawValueAdjustment("HYSTERESIS", *adjustingFloatValue, "deg");
                }
            } else if (adjustingIntValue != nullptr) {
                if (adjustingIntValue == &displayBrightness) {
                    *adjustingIntValue = newPos * 10;
                    *adjustingIntValue = constrain(*adjustingIntValue, 10, 255);
                    tft.setBrightness(*adjustingIntValue);
                    drawValueAdjustment("BRIGHTNESS", *adjustingIntValue, "");
                } else if (adjustingIntValue == &ledBrightness) {
                    *adjustingIntValue = newPos * 5;
                    *adjustingIntValue = constrain(*adjustingIntValue, 5, 255);
                    FastLED.setBrightness(*adjustingIntValue);
                    leds[0] = CRGB::White;
                    FastLED.show();
                    drawValueAdjustment("LED BRIGHTNESS", *adjustingIntValue, "");
                } else if (adjustingIntValue == &parTimeSeconds) {
                    *adjustingIntValue = newPos;
                    *adjustingIntValue = constrain(*adjustingIntValue, 5, 600);
                    drawValueAdjustment("PAR TIME", *adjustingIntValue, "sec");
                } else if (adjustingIntValue == &yellowWarningSeconds) {
                    *adjustingIntValue = newPos;
                    *adjustingIntValue = constrain(*adjustingIntValue, 5, parTimeSeconds - 5);
                    drawValueAdjustment("YELLOW WARN", *adjustingIntValue, "sec");
                } else if (adjustingIntValue == &redWarningSeconds) {
                    *adjustingIntValue = newPos;
                    *adjustingIntValue = constrain(*adjustingIntValue, 1, yellowWarningSeconds - 5);
                    drawValueAdjustment("RED WARNING", *adjustingIntValue, "sec");
                }
            }
        }
        
        lastEncoderPos = newPos;
    }
    
    // Update display
    if (currentMenu == MAIN_DISPLAY) {
        updateMainDisplay();
    }
    
    delay(5);
}