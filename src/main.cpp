#include <Arduino.h>
#include <Wire.h>
#include <FastLED.h>
#include <RotaryEncoder.h>
#include "pin_config.h"

// Include our modules
#include "settings.h"
#include "level_monitor.h"
#include "timer.h"
#include "menu_system.h"
#include "display_manager.h"
#include "buzzer.h"  // <-- Add this
#include <SensorQMI8658.hpp>

#define USBSerial Serial

// Hardware objects
CRGB leds[NUM_LEDS];
SensorQMI8658 qmi;
RotaryEncoder encoder(ENCODER_CLK, ENCODER_DT, RotaryEncoder::LatchMode::FOUR3);

// Encoder interrupt
void IRAM_ATTR checkEncoder() {
    encoder.tick();
}

void setup() {
    USBSerial.begin(115200);
    delay(2000);
    
    USBSerial.println("\n================================");
    USBSerial.println("=== Stage Timer v3.1 ===");
    USBSerial.println("=== With Buzzer ===");
    USBSerial.println("================================");

    // Load settings from flash
    settings.load();

    // Initialize RGB LED
    USBSerial.println("Initializing RGB LED...");
    FastLED.addLeds<WS2812B, RGB_LED_PIN, RGB>(leds, NUM_LEDS);
    FastLED.setBrightness(settings.ledBrightness);
    leds[0] = CRGB::Purple;
    FastLED.show();
    USBSerial.println("RGB LED: OK!");

    // Initialize Buzzer
    buzzer.begin();  // <-- Add this

    // Initialize Display
    USBSerial.println("Initializing display...");
    display.begin();
    display.setBrightness(settings.displayBrightness);
    USBSerial.println("Display: OK!");

    // Initialize IMU
    USBSerial.println("Initializing IMU...");
    Wire.begin(IIC_SDA, IIC_SCL);
    
    if (!qmi.begin(Wire, QMI8658_L_SLAVE_ADDRESS, IIC_SDA, IIC_SCL)) {
        USBSerial.println("ERROR: IMU not found!");
        display.getTFT()->fillScreen(COLOR_RED);
        display.getTFT()->setTextColor(TFT_WHITE);
        display.getTFT()->setTextSize(2);
        display.getTFT()->setCursor(10, 140);
        display.getTFT()->println("IMU ERROR");
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

    // Initialize Level Monitor
    levelMonitor.begin(&qmi, leds);

    // Initialize Rotary Encoder
    USBSerial.println("Initializing encoder...");
    pinMode(ENCODER_CLK, INPUT_PULLUP);
    pinMode(ENCODER_DT, INPUT_PULLUP);
    pinMode(ENCODER_SW, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(ENCODER_CLK), checkEncoder, CHANGE);
    attachInterrupt(digitalPinToInterrupt(ENCODER_DT), checkEncoder, CHANGE);
    USBSerial.println("Encoder: OK!");

    // Initialize Menu System
    menu.begin(display.getTFT(), &encoder);

    pinMode(BOOT_BUTTON, INPUT_PULLUP);

    // Calibration check
    if (!settings.gravity.isCalibrated) {
        display.getTFT()->fillScreen(COLOR_CYAN);
        display.getTFT()->setTextColor(TFT_BLACK);
        display.getTFT()->setTextSize(1);
        display.getTFT()->setCursor(10, 100);
        display.getTFT()->println("CALIBRATION NEEDED");
        display.getTFT()->setCursor(10, 120);
        display.getTFT()->println("Hold board LEVEL");
        display.getTFT()->setCursor(10, 150);
        display.getTFT()->println("Press BOOT button");
        
        USBSerial.println("\nWaiting for BOOT button...");
        
        while(digitalRead(BOOT_BUTTON) == HIGH) delay(10);
        delay(200);
        while(digitalRead(BOOT_BUTTON) == LOW) delay(10);
        
        levelMonitor.calibrate();
    }
    
    // Ready screen
    display.getTFT()->fillScreen(COLOR_GREEN);
    display.getTFT()->setTextColor(TFT_BLACK);
    display.getTFT()->setTextSize(3);
    display.getTFT()->setCursor(15, 140);
    display.getTFT()->println("READY!");
    
    leds[0] = CRGB::Green;
    FastLED.show();
    delay(1500);
    
    USBSerial.println("\n=== READY! ===\n");
}

void loop() {
    // Update modules
    levelMonitor.update();
    timer.update();
    buzzer.update();  // <-- Add this
    
    // Handle encoder button
    static bool lastButtonState = HIGH;
    static unsigned long buttonPressStart = 0;
    static bool longPressDetected = false;
    bool buttonState = digitalRead(ENCODER_SW);
    
    // Button press started
    if (buttonState == LOW && lastButtonState == HIGH) {
        buttonPressStart = millis();
        longPressDetected = false;
    }
    
    // Long press detection (>1 second) - only in main display for timer
    if (buttonState == LOW && !longPressDetected && !menu.isInMenu()) {
        if (millis() - buttonPressStart > 1000) {
            longPressDetected = true;
            timer.setReady();
            leds[0] = CRGB::Yellow;
            FastLED.show();
        }
    }
    
    // Button released (short press)
    if (buttonState == HIGH && lastButtonState == LOW && (millis() - buttonPressStart > 50)) {
        if (!longPressDetected) {
            // Short press
            if (!menu.isInMenu()) {
                if (timer.getState() == TIMER_READY) {
                    timer.start();
                } else if (timer.getState() == TIMER_RUNNING || timer.getState() == TIMER_FINISHED) {
                    timer.reset();
                } else {
                    // Enter menu
                    menu.handleButton();
                }
            } else {
                // In menu
                menu.handleButton();
            }
        }
    }
    lastButtonState = buttonState;
    
    // Handle encoder rotation
    static int lastEncoderPos = 0;
    int newPos = encoder.getPosition();
    
    if (newPos != lastEncoderPos) {
        int delta = newPos - lastEncoderPos;
        
        if (menu.isInMenu()) {
            menu.handleRotation(delta);
        }
        
        lastEncoderPos = newPos;
    }
    
    // Update display if in main mode
    if (!menu.isInMenu()) {
        static unsigned long lastDisplayUpdate = 0;
        if (millis() - lastDisplayUpdate > 100) {
            lastDisplayUpdate = millis();
            
            // Check if we're in SHOOTER READY mode
            static TimerState lastTimerState = TIMER_IDLE;
            TimerState currentTimerState = timer.getState();

           if (currentTimerState == TIMER_READY) {
                // Draw full-screen SHOOTER READY if state just changed
                if (lastTimerState != TIMER_READY) {
                    display.drawShooterReady();
                }
                // Don't update anything else while in READY mode
                lastTimerState = currentTimerState;
            } else {
                // Normal display updates
                
                // If we just left READY state, force full redraw
                if (lastTimerState == TIMER_READY) {
                    display.getTFT()->fillScreen(TFT_BLACK);
                }
                
                // Update level indicator
                if (levelMonitor.needsRedraw() || lastTimerState == TIMER_READY) {
                    display.drawLevelIndicator(
                        levelMonitor.getFilteredAngle(),
                        levelMonitor.getStatusColor(),
                        levelMonitor.getStatusText()
                    );
                    levelMonitor.clearRedrawFlag();
                } else {
                    display.updateLevelAngle(
                        levelMonitor.getFilteredAngle(),
                        levelMonitor.getStatusColor()
                    );
                }
                
                // Update timer display
                const char* timerStateText;
                switch(currentTimerState) {
                    case TIMER_IDLE:
                        timerStateText = "Long press to ready";
                        break;
                    case TIMER_READY:
                        timerStateText = "READY - Press to start";
                        break;
                    case TIMER_RUNNING:
                        timerStateText = "RUNNING";
                        break;
                    case TIMER_FINISHED:
                        timerStateText = "TIME!";
                        break;
                    default:
                        timerStateText = "";
                }
                
                display.drawTimerDisplay(
                    timer.getRemainingSeconds(),
                    timer.getPercentRemaining(),
                    timer.getTimerColor(),
                    timerStateText
                );
                
                lastTimerState = currentTimerState;
            }
            
            // Update LED (unless timer finished)
            if (timer.getState() != TIMER_FINISHED) {
                leds[0] = levelMonitor.getLEDColor();
                FastLED.show();
            } else {
                leds[0] = CRGB::Red;
                FastLED.show();
            }
        }
    }
    
    delay(5);
}