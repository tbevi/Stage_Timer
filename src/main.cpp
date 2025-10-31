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

// Menu states
enum MenuState {
    MAIN_DISPLAY,
    MENU_TOP_LEVEL,
    MENU_LEVEL_SUBMENU,
    MENU_DISPLAY_SUBMENU,
    ADJUSTING_VALUE
};

enum TopMenuItem {
    TOP_LEVEL,
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

MenuState currentMenu = MAIN_DISPLAY;
int selectedTopItem = 0;
int selectedLevelItem = 0;
int selectedDisplayItem = 0;

// Settings (loaded from/saved to flash)
float settingTolerance = 0.5;
float settingHysteresis = 0.1;
int displayBrightness = 255;
int ledBrightness = 50;
float settingAlpha = 0.2;

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
    USBSerial.print("  Tolerance: "); USBSerial.println(settingTolerance);
    USBSerial.print("  Hysteresis: "); USBSerial.println(settingHysteresis);
    USBSerial.print("  Display Brightness: "); USBSerial.println(displayBrightness);
    USBSerial.print("  LED Brightness: "); USBSerial.println(ledBrightness);
}

// Save settings to flash
void saveSettings() {
    preferences.begin("stage_timer", false);
    
    preferences.putFloat("tolerance", settingTolerance);
    preferences.putFloat("hysteresis", settingHysteresis);
    preferences.putInt("disp_bright", displayBrightness);
    preferences.putInt("led_bright", ledBrightness);
    preferences.putFloat("alpha", settingAlpha);
    
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
    USBSerial.print("Gravity vector: X:");
    USBSerial.print(gravityRef.x, 3);
    USBSerial.print(" Y:");
    USBSerial.print(gravityRef.y, 3);
    USBSerial.print(" Z:");
    USBSerial.print(gravityRef.z, 3);
    USBSerial.print(" Mag:");
    USBSerial.println(gravityRef.magnitude, 3);
}

float calculateSimpleTiltAngle() {
    float xCal = acc.x - gravityRef.x;
    float angle = atan2(xCal, -acc.y) * 180.0 / PI;
    return angle;
}

void drawTopMenu() {
    tft.fillScreen(TFT_BLACK);
    tft.setTextSize(1);
    
    // Title
    tft.setTextColor(TFT_WHITE);
    tft.setCursor(50, 10);
    tft.println("SETTINGS");
    
    // Menu items
    const char* menuItems[] = {
        "Level",
        "Display",
        "Exit"
    };
    
    int startY = 50;
    for (int i = 0; i < TOP_ITEM_COUNT; i++) {
        int y = startY + (i * 25);
        
        // Highlight selected item
        if (i == selectedTopItem) {
            tft.fillRect(10, y - 2, 150, 20, TFT_BLUE);
            tft.setTextColor(TFT_WHITE);
        } else {
            tft.setTextColor(TFT_DARKGREY);
        }
        
        tft.setCursor(15, y + 3);
        tft.println(menuItems[i]);
    }
    
    // Instructions
    tft.setTextColor(TFT_DARKGREY);
    tft.setCursor(10, 280);
    tft.println("Turn: Select");
    tft.setCursor(10, 295);
    tft.println("Press: Confirm");
}

void drawLevelSubmenu() {
    tft.fillScreen(TFT_BLACK);
    tft.setTextSize(1);
    
    // Title
    tft.setTextColor(TFT_WHITE);
    tft.setCursor(10, 10);
    tft.println("< LEVEL");
    
    // Menu items
    const char* menuItems[] = {
        "Calibrate",
        "Tolerance",
        "Hysteresis",
        "Back"
    };
    
    int startY = 40;
    for (int i = 0; i < LEVEL_ITEM_COUNT; i++) {
        int y = startY + (i * 25);
        
        // Highlight selected item
        if (i == selectedLevelItem) {
            tft.fillRect(10, y - 2, 150, 20, TFT_BLUE);
            tft.setTextColor(TFT_WHITE);
        } else {
            tft.setTextColor(TFT_DARKGREY);
        }
        
        tft.setCursor(15, y + 3);
        tft.print(menuItems[i]);
        
        // Show current values
        if (i == LEVEL_TOLERANCE) {
            tft.setCursor(110, y + 3);
            tft.print(settingTolerance, 1);
            tft.print("deg");
        } else if (i == LEVEL_HYSTERESIS) {
            tft.setCursor(110, y + 3);
            tft.print(settingHysteresis, 2);
            tft.print("deg");
        }
    }
    
    // Instructions
    tft.setTextColor(TFT_DARKGREY);
    tft.setCursor(10, 280);
    tft.println("Turn: Select");
    tft.setCursor(10, 295);
    tft.println("Press: Confirm");
}

void drawDisplaySubmenu() {
    tft.fillScreen(TFT_BLACK);
    tft.setTextSize(1);
    
    // Title
    tft.setTextColor(TFT_WHITE);
    tft.setCursor(10, 10);
    tft.println("< DISPLAY");
    
    // Menu items
    const char* menuItems[] = {
        "Brightness",
        "LED Brightness",
        "Back"
    };
    
    int startY = 40;
    for (int i = 0; i < DISPLAY_ITEM_COUNT; i++) {
        int y = startY + (i * 25);
        
        // Highlight selected item
        if (i == selectedDisplayItem) {
            tft.fillRect(10, y - 2, 150, 20, TFT_BLUE);
            tft.setTextColor(TFT_WHITE);
        } else {
            tft.setTextColor(TFT_DARKGREY);
        }
        
        tft.setCursor(15, y + 3);
        tft.print(menuItems[i]);
        
        // Show current values
        if (i == DISPLAY_BRIGHTNESS) {
            tft.setCursor(130, y + 3);
            tft.print(displayBrightness);
        } else if (i == DISPLAY_LED_BRIGHTNESS) {
            tft.setCursor(130, y + 3);
            tft.print(ledBrightness);
        }
    }
    
    // Instructions
    tft.setTextColor(TFT_DARKGREY);
    tft.setCursor(10, 280);
    tft.println("Turn: Select");
    tft.setCursor(10, 295);
    tft.println("Press: Confirm");
}

void drawValueAdjustment(const char* label, float value, const char* unit) {
    tft.fillScreen(TFT_BLACK);
    tft.setTextSize(1);
    
    // Title
    tft.setTextColor(TFT_WHITE);
    tft.setCursor(10, 10);
    tft.print("< ");
    tft.println(label);
    
    // Large value display
    tft.setTextSize(3);
    tft.setTextColor(COLOR_CYAN);
    tft.setCursor(20, 100);
    tft.print(value, 2);
    tft.print(" ");
    tft.setTextSize(2);
    tft.println(unit);
    
    // Instructions
    tft.setTextSize(1);
    tft.setTextColor(TFT_DARKGREY);
    tft.setCursor(10, 250);
    tft.println("Turn: Adjust");
    tft.setCursor(10, 265);
    tft.println("Press: Save");
}

void drawValueAdjustment(const char* label, int value, const char* unit) {
    tft.fillScreen(TFT_BLACK);
    tft.setTextSize(1);
    
    // Title
    tft.setTextColor(TFT_WHITE);
    tft.setCursor(10, 10);
    tft.print("< ");
    tft.println(label);
    
    // Large value display
    tft.setTextSize(3);
    tft.setTextColor(COLOR_CYAN);
    tft.setCursor(50, 100);
    tft.print(value);
    tft.print(" ");
    tft.setTextSize(2);
    tft.println(unit);
    
    // Visual bar
    int barWidth = map(value, 0, 255, 0, 150);
    tft.fillRect(10, 180, barWidth, 20, COLOR_GREEN);
    tft.drawRect(10, 180, 150, 20, TFT_WHITE);
    
    // Instructions
    tft.setTextSize(1);
    tft.setTextColor(TFT_DARKGREY);
    tft.setCursor(10, 250);
    tft.println("Turn: Adjust");
    tft.setCursor(10, 265);
    tft.println("Press: Save");
}

void executeTopMenuItem(int item) {
    switch(item) {
        case TOP_LEVEL:
            currentMenu = MENU_LEVEL_SUBMENU;
            selectedLevelItem = 0;
            encoder.setPosition(0);
            drawLevelSubmenu();
            break;
            
        case TOP_DISPLAY:
            currentMenu = MENU_DISPLAY_SUBMENU;
            selectedDisplayItem = 0;
            encoder.setPosition(0);
            drawDisplaySubmenu();
            break;
            
        case TOP_EXIT:
            // Save and exit
            saveSettings();
            currentMenu = MAIN_DISPLAY;
            lastStatusColor = 0xFFFF;
            lastDisplayedAngle = 999;
            firstDraw = true;
            
            // Apply brightness settings
            tft.setBrightness(displayBrightness);
            FastLED.setBrightness(ledBrightness);
            
            USBSerial.println("Exiting to main display");
            break;
    }
}

void executeLevelMenuItem(int item) {
    switch(item) {
        case LEVEL_CALIBRATE:
            // Show calibration screen
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
            selectedTopItem = 1;
            encoder.setPosition(1);
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

    // Debug output
    static unsigned long lastPrint = 0;
    if (millis() - lastPrint > 500) {
        lastPrint = millis();
        USBSerial.print("Raw: ");
        USBSerial.print(rawAngle, 2);
        USBSerial.print("° Filtered: ");
        USBSerial.print(displayedAngle, 2);
        USBSerial.println("°");
    }

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

    // Update display
    if (millis() - lastUpdate > 100) {
        lastUpdate = millis();
        
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
        
        if (firstDraw) {
            tft.fillRect(0, 107, 170, 213, TFT_BLACK);
            tft.setTextSize(1);
            tft.setTextColor(TFT_DARKGREY);
            tft.setCursor(10, 300);
            tft.println("Stage Timer v2.0");
            firstDraw = false;
        }
        
        leds[0] = ledColor;
        FastLED.show();
    }
}

void setup() {
    USBSerial.begin(115200);
    delay(2000);
    
    USBSerial.println("\n================================");
    USBSerial.println("=== Stage Timer v2.0 ===");
    USBSerial.println("=== Two-Level Menu ===");
    USBSerial.println("================================");

    // Load settings from flash first
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

    // Show calibration prompt if not calibrated
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
    
    // Show ready screen
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
    // Check encoder button
    static bool lastButtonState = HIGH;
    static unsigned long lastButtonTime = 0;
    bool buttonState = digitalRead(ENCODER_SW);
    
    if (buttonState == LOW && lastButtonState == HIGH && (millis() - lastButtonTime > 50)) {
        lastButtonTime = millis();
        
        if (currentMenu == MAIN_DISPLAY) {
            // Enter settings menu
            currentMenu = MENU_TOP_LEVEL;
            selectedTopItem = 0;
            encoder.setPosition(0);
            drawTopMenu();
            USBSerial.println("Entered top menu");
        } else if (currentMenu == MENU_TOP_LEVEL) {
            executeTopMenuItem(selectedTopItem);
        } else if (currentMenu == MENU_LEVEL_SUBMENU) {
            executeLevelMenuItem(selectedLevelItem);
        } else if (currentMenu == MENU_DISPLAY_SUBMENU) {
            executeDisplayMenuItem(selectedDisplayItem);
        } else if (currentMenu == ADJUSTING_VALUE) {
            // Save value and go back
            if (adjustingFloatValue != nullptr) {
                // Already adjusted in real-time
                if (adjustingFloatValue == &settingTolerance) {
                    currentMenu = MENU_LEVEL_SUBMENU;
                    drawLevelSubmenu();
                } else if (adjustingFloatValue == &settingHysteresis) {
                    currentMenu = MENU_LEVEL_SUBMENU;
                    drawLevelSubmenu();
                }
                adjustingFloatValue = nullptr;
            } else if (adjustingIntValue != nullptr) {
                // Apply brightness immediately
                if (adjustingIntValue == &displayBrightness) {
                    tft.setBrightness(displayBrightness);
                    currentMenu = MENU_DISPLAY_SUBMENU;
                    drawDisplaySubmenu();
                } else if (adjustingIntValue == &ledBrightness) {
                    FastLED.setBrightness(ledBrightness);
                    currentMenu = MENU_DISPLAY_SUBMENU;
                    drawDisplaySubmenu();
                }
                adjustingIntValue = nullptr;
            }
            saveSettings();
            USBSerial.println("Value saved");
        }
    }
    lastButtonState = buttonState;
    
    // Check encoder rotation
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
        } else if (currentMenu == MENU_DISPLAY_SUBMENU) {
            selectedDisplayItem += delta;
            if (selectedDisplayItem < 0) selectedDisplayItem = DISPLAY_ITEM_COUNT - 1;
            if (selectedDisplayItem >= DISPLAY_ITEM_COUNT) selectedDisplayItem = 0;
            drawDisplaySubmenu();
        } else if (currentMenu == ADJUSTING_VALUE) {
            // Adjust value in real-time
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
                }
            }
        }
        
        lastEncoderPos = newPos;
    }
    
    // Update appropriate display
    if (currentMenu == MAIN_DISPLAY) {
        updateMainDisplay();
    }
    
    delay(5);
}