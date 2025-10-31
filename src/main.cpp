#include <Arduino.h>
#include <Wire.h>
#include <LovyanGFX.hpp>
#include "pin_config.h"
#include <SensorQMI8658.hpp>  // From SensorLib
#include <FastLED.h>

#define USBSerial Serial

CRGB leds[NUM_LEDS];

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
            
            cfg.pin_sclk = LCD_SCLK;   // Pin 10
            cfg.pin_mosi = LCD_MOSI;   // Pin 13
            cfg.pin_miso = -1;
            cfg.pin_dc = LCD_DC;       // Pin 11
            
            _bus_instance.config(cfg);
            _panel_instance.setBus(&_bus_instance);
        }

        {
            auto cfg = _panel_instance.config();
            
            cfg.pin_cs = LCD_CS;       // Pin 12
            cfg.pin_rst = LCD_RST;     // Pin 9
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
            
            cfg.pin_bl = LCD_BL;       // Pin 2
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

// Display update variables (global scope)
unsigned long lastUpdate = 0;
uint16_t lastStatusColor = 0;
float lastDisplayedAngle = 999;
bool firstDraw = true;

// CORRECTED color definitions - display shows standard RGB565 colors correctly:
#define DISPLAY_RED    0xF800   // Red (R=31, G=0, B=0)
#define DISPLAY_GREEN  0x07E0   // Green (R=0, G=63, B=0)  
#define DISPLAY_BLUE   0x001F   // Blue (R=0, G=0, B=31)
// But based on your test, it seems colors are getting mixed up somewhere
// Let's use the standard TFT color definitions since your display is normal:
// Actually, TFT_RED, TFT_GREEN, TFT_BLUE should work fine!
// Define colors - let's try different values since colors are swapped
// Standard RGB565: RRRRRGGG GGGBBBBB
// If colors are wrong, the display might have BGR order
#define COLOR_RED    0xF800   // Standard red
#define COLOR_GREEN  0x07E0   // Standard green  
#define COLOR_BLUE   0x001F   // Standard blue
#define COLOR_CYAN   0x07FF   // Cyan for calibration


void performCalibration() {
    USBSerial.println("\n*** CALIBRATION ***");
    USBSerial.println("Hold board LEVEL (horizontal)");
    
    // Take average of 100 readings to get stable gravity vector
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
    
    // Store the gravity vector when level
    gravityRef.x = xSum / 100.0;
    gravityRef.y = ySum / 100.0;
    gravityRef.z = zSum / 100.0;
    
    // Calculate magnitude of gravity vector
    gravityRef.magnitude = sqrt(gravityRef.x * gravityRef.x + 
                               gravityRef.y * gravityRef.y + 
                               gravityRef.z * gravityRef.z);
    
    calibrated = true;
    
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

// Simple angle calculation that works
float calculateSimpleTiltAngle() {
    float xCal = acc.x - gravityRef.x;  // Remove X offset when level
    float angle = atan2(xCal, -acc.y) * 180.0 / PI;
    return angle;
}

void setup() {
    USBSerial.begin(115200);
    delay(2000);
    
    USBSerial.println("\n================================");
    USBSerial.println("=== Stage Timer v1.9 ===");
    USBSerial.println("=== Final Color Fix ===");
    USBSerial.println("================================");

    // RGB LED - Initialize and test
    USBSerial.println("Initializing RGB LED...");
    FastLED.addLeds<WS2812B, RGB_LED_PIN, RGB>(leds, NUM_LEDS);
    FastLED.setBrightness(50);
    
    // Test LED colors
    leds[0] = CRGB::Red;
    FastLED.show();
    delay(500);
    leds[0] = CRGB::Green;
    FastLED.show();
    delay(500);
    leds[0] = CRGB::Blue;
    FastLED.show();
    delay(500);
    leds[0] = CRGB::Purple;
    FastLED.show();
    USBSerial.println("RGB LED: OK!");

    // Display - PORTRAIT MODE
    USBSerial.println("Initializing display...");
    tft.init();
    tft.setRotation(0);  // Portrait: 170x320
    tft.setBrightness(255);
    tft.fillScreen(TFT_BLACK);
        
    // Test display colors with labels
    tft.fillRect(0, 0, 170, 80, COLOR_RED);
    tft.setTextColor(TFT_WHITE);
    tft.setCursor(60, 30);
    tft.print("RED");
    
    tft.fillRect(0, 80, 170, 80, COLOR_GREEN);
    tft.setCursor(60, 110);
    tft.print("GREEN");
    
    tft.fillRect(0, 160, 170, 80, COLOR_BLUE);
    tft.setCursor(60, 190);
    tft.print("BLUE");

    delay(3000 ); //show test pattern for 3 seconds
    
    USBSerial.println("Display: OK!");

    // IMU - Using your actual I2C pins
    USBSerial.println("Initializing IMU...");
    Wire.begin(IIC_SDA, IIC_SCL);  // Pins 47 and 48
    
    if (!qmi.begin(Wire, QMI8658_L_SLAVE_ADDRESS, IIC_SDA, IIC_SCL)) {
        USBSerial.println("ERROR: IMU not found!");
        tft.fillScreen(COLOR_RED);
        tft.setTextColor(TFT_WHITE);
        tft.setTextSize(2);
        tft.setCursor(10, 140);
        tft.println("IMU ERROR");
        while(1) delay(1000);
    }
    
    // Configure accelerometer
    qmi.configAccelerometer(
        SensorQMI8658::ACC_RANGE_4G,
        SensorQMI8658::ACC_ODR_1000Hz,
        SensorQMI8658::LPF_MODE_0,
        true
    );
    qmi.enableAccelerometer();
    USBSerial.println("IMU: OK!");

    pinMode(BOOT_BUTTON, INPUT_PULLUP);

    // Calibration screen - use cyan or bright blue
    tft.fillScreen(0x07FF);  // Cyan
    tft.setTextColor(TFT_BLACK);
    tft.setTextSize(2);
    tft.setCursor(10, 100);
    tft.println("CALIBRATION");
    tft.setCursor(10, 130);
    tft.println("Hold board");
    tft.setCursor(10, 150);
    tft.println("LEVEL");
    tft.setCursor(10, 170);
    tft.println("(horizontal)");
    tft.setCursor(10, 210);
    tft.println("Press BOOT");
    tft.setCursor(10, 230);
    tft.println("when ready");
    
    USBSerial.println("\nWaiting for BOOT button...");
    
    // Wait for button press
    while(digitalRead(BOOT_BUTTON) == HIGH) {
        delay(10);
    }
    delay(200);
    while(digitalRead(BOOT_BUTTON) == LOW) delay(10);
    
    // Perform calibration
    performCalibration();
    
    // Show success
    tft.fillScreen(COLOR_GREEN);
    tft.setTextColor(TFT_BLACK);
    tft.setTextSize(3);
    tft.setCursor(15, 140);
    tft.println("READY!");
    
    leds[0] = CRGB::Green;
    FastLED.show();
    
    delay(1500);
    
    USBSerial.println("\n=== READY! ===\n");
    
    // Force initial state
    lastStatusColor = 0xFFFF;  // Force first update
}

void loop() {
    // Read accelerometer data
    if (qmi.getDataReady()) {
        qmi.getAccelerometer(acc.x, acc.y, acc.z);
    }

    // Calculate angle
    float angle = calculateSimpleTiltAngle();

    // Print debug info every 500ms
    static unsigned long lastPrint = 0;
    if (millis() - lastPrint > 500) {
        lastPrint = millis();
        
        USBSerial.print("Angle: ");
        USBSerial.print(angle, 2);
        USBSerial.print("° | X:");
        USBSerial.print(acc.x, 3);
        USBSerial.print(" Y:");
        USBSerial.print(acc.y, 3);
        USBSerial.print(" Z:");
        USBSerial.println(acc.z, 3);
    }

    // Determine color - use standard TFT colors since display works normally
    uint16_t statusColor;
    CRGB ledColor;
    const char* status;
    
    if (angle > -0.5 && angle < 0.5) {
        // LEVEL - GREEN
        statusColor = COLOR_GREEN;  // Green
        ledColor = CRGB::Green;
        status = "LEVEL";
    } else if (angle >= 0.5) {
        // CW (positive) - RED
        statusColor = COLOR_RED;    // Red
        ledColor = CRGB::Red;
        status = "CW";
    } else {  // angle <= -0.5
        // CCW (negative) - BLUE (or cyan for brighter)
        statusColor = COLOR_BLUE;     // Blue
        ledColor = CRGB::Blue;
        status = "CCW";
    }

    // Update display at 10Hz
    if (millis() - lastUpdate > 100) {
        lastUpdate = millis();
        
        // Always redraw if color changed OR first time
        if (statusColor != lastStatusColor || firstDraw) {
            // Clear and redraw the status area
            tft.fillRect(0, 0, 170, 107, statusColor);
            
            // Draw angle with better contrast
            tft.setTextSize(5);
            // Use black text on bright backgrounds, white on dark
            if (statusColor == COLOR_GREEN || statusColor == 0x07FF) {
                tft.setTextColor(TFT_BLACK);
            } else {
                tft.setTextColor(TFT_WHITE);
            }
            
            char buf[10];
            sprintf(buf, "%.1f", angle);
            int textWidth = strlen(buf) * 30;
            int x = (170 - textWidth) / 2;
            
            tft.setCursor(x, 15);
            tft.print(angle, 1);
            
            // Draw status text
            tft.setTextSize(2);
            int statusWidth = strlen(status) * 12;
            tft.setCursor((170 - statusWidth) / 2, 80);
            tft.println(status);
            
            lastStatusColor = statusColor;
            lastDisplayedAngle = angle;
        }
        // Update angle if it changed significantly
        else if (abs(angle - lastDisplayedAngle) > 0.05) {
            // Just update the angle text
            tft.fillRect(0, 10, 170, 60, statusColor);
            
            tft.setTextSize(5);
            if (statusColor == COLOR_GREEN || statusColor == 0x07FF) {
                tft.setTextColor(TFT_BLACK);
            } else {
                tft.setTextColor(TFT_WHITE);
            }
            
            char buf[10];
            sprintf(buf, "%.1f", angle);
            int textWidth = strlen(buf) * 30;
            int x = (170 - textWidth) / 2;
            
            tft.setCursor(x, 15);
            tft.print(angle, 1);
            
            lastDisplayedAngle = angle;
        }
        
        // Draw lower section on first draw
        if (firstDraw) {
            tft.fillRect(0, 107, 170, 213, TFT_BLACK);
            
            tft.setTextSize(1);
            tft.setTextColor(TFT_DARKGREY);
            tft.setCursor(10, 300);
            tft.println("Stage Timer v1.9");
            
            firstDraw = false;
        }
        
        // Update LED
        leds[0] = ledColor;
        FastLED.show();
    }

    // Handle recalibration on button press
    if (digitalRead(BOOT_BUTTON) == LOW) {
        USBSerial.println("\n*** RECALIBRATING ***");
        
        // Flash LED yellow during calibration
        leds[0] = CRGB::Yellow;
        FastLED.show();
        
        // Show calibrating screen
        tft.fillScreen(TFT_YELLOW);
        tft.setTextColor(TFT_BLACK);
        tft.setTextSize(2);
        tft.setCursor(10, 140);
        tft.println("CALIBRATING...");
        
        // Wait for button release
        delay(200);
        while(digitalRead(BOOT_BUTTON) == LOW) delay(10);
        delay(500);
        
        // Perform calibration
        performCalibration();
        
        // Show success
        tft.fillScreen(COLOR_GREEN);
        tft.setTextColor(TFT_BLACK);
        tft.setTextSize(3);
        tft.setCursor(10, 140);
        tft.println("CALIBRATED!");
        
        leds[0] = CRGB::Green;
        FastLED.show();
        
        delay(1000);
        
        // Force full redraw
        lastStatusColor = 0xFFFF;  // Force color update
        lastDisplayedAngle = 999;
        firstDraw = true;
    }

    delay(5);
}