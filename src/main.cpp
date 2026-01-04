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
#include "buzzer.h"
#include "mic_detector.h"
#include <SensorQMI8658.hpp>

#define USBSerial Serial

// Hardware objects
CRGB leds[NUM_LEDS];
SensorQMI8658 qmi;
RotaryEncoder encoder(ENCODER_CLK, ENCODER_DT, RotaryEncoder::LatchMode::FOUR3);

// Diagnostic mode flag
bool micDiagnosticMode = false;

// Encoder interrupt
void IRAM_ATTR checkEncoder() {
    encoder.tick();
}

void setup() {
    USBSerial.begin(115200);
    delay(2000);
    
    USBSerial.println("\n================================");
    USBSerial.println("=== Stage Timer v3.2 ===");
    USBSerial.println("=== Multi-Freq Detection ===");
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
    buzzer.begin();

    // Initialize Microphone
    USBSerial.println("Initializing microphone...");
    if (!micDetector.begin()) {
        USBSerial.println("WARNING: Microphone initialization failed!");
        USBSerial.println("Manual timer start will still work.");
    }

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
    
    // *** NEW: Configure and enable gyroscope for sensor fusion ***
    qmi.configGyroscope(
        SensorQMI8658::GYR_RANGE_512DPS,   // ±512 deg/sec (good for rifle movement)
        SensorQMI8658::GYR_ODR_896_8Hz,     // Match accelerometer rate
        SensorQMI8658::LPF_MODE_3,         // Low-pass filter
        true
    );
    qmi.enableGyroscope();
    USBSerial.println("IMU: Accel + Gyro OK!");

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
    USBSerial.println("TIP: Hold BOOT button for 2s to enter mic diagnostic mode");
}

void loop() {
    // Check BOOT button for diagnostic mode
    static unsigned long bootButtonPressStart = 0;
    static bool bootButtonPressed = false;
    static bool lastBootState = HIGH;
    bool bootState = digitalRead(BOOT_BUTTON);
    
    if (bootState == LOW && lastBootState == HIGH) {
        bootButtonPressStart = millis();
        bootButtonPressed = true;
    }
    
    // 2-second hold for diagnostic mode
    if (bootButtonPressed && bootState == LOW && 
        millis() - bootButtonPressStart > 2000 && !menu.isInMenu()) {
        
        bootButtonPressed = false;
        
        if (!micDiagnosticMode) {
            // Enter diagnostic mode
            micDiagnosticMode = true;
            micDetector.startDiagnostic();
            
            // Display diagnostic screen
            display.getTFT()->fillScreen(TFT_BLACK);
            display.getTFT()->setTextColor(COLOR_CYAN);
            display.getTFT()->setTextSize(2);
            display.getTFT()->setCursor(10, 10);
            display.getTFT()->println("MIC DIAGNOSTIC");
            
            leds[0] = CRGB::Cyan;
            FastLED.show();
        } else {
            // Exit diagnostic mode
            micDiagnosticMode = false;
            micDetector.stopDiagnostic();
            
            display.getTFT()->fillScreen(TFT_BLACK);
            leds[0] = CRGB::Green;
            FastLED.show();
        }
    }
    
    if (bootState == HIGH) {
        bootButtonPressed = false;
    }
    lastBootState = bootState;

    // Update modules
    levelMonitor.update();
    timer.update();
    buzzer.update();

    // Diagnostic mode display update
    if (micDiagnosticMode) {
        micDetector.update();  // Keep processing audio
        
        static unsigned long lastDiagUpdate = 0;
        if (millis() - lastDiagUpdate > 100) {  // Update display every 100ms
            lastDiagUpdate = millis();
            
            MicStats stats = micDetector.getStats();
            
            // Clear and redraw stats
            display.getTFT()->fillRect(0, 40, 170, 250, TFT_BLACK);
            
            display.getTFT()->setTextSize(1);
            display.getTFT()->setTextColor(TFT_WHITE);
            
            // Current magnitude bar
            display.getTFT()->setCursor(10, 50);
            display.getTFT()->print("Level: ");
            display.getTFT()->print(stats.currentMagnitude, 0);
            
            int barLength = constrain(stats.currentMagnitude / 50, 0, 150);
            display.getTFT()->fillRect(10, 65, barLength, 10, COLOR_GREEN);
            display.getTFT()->drawRect(10, 65, 150, 10, TFT_WHITE);
            
            // Peak magnitude
            display.getTFT()->setCursor(10, 90);
            display.getTFT()->print("Peak: ");
            display.getTFT()->print(stats.peakMagnitude, 0);
            display.getTFT()->print(" @ ");
            display.getTFT()->print(stats.detectedFrequency, 0);
            display.getTFT()->print("Hz");
            
            // Noise floor
            display.getTFT()->setCursor(10, 110);
            display.getTFT()->print("Noise: ");
            display.getTFT()->print(stats.noiseFloor, 0);
            
            // SNR
            display.getTFT()->setCursor(10, 130);
            display.getTFT()->print("SNR: ");
            display.getTFT()->print(stats.snr, 1);
            display.getTFT()->print("x");
            
            // Thresholds
            display.getTFT()->setCursor(10, 150);
            display.getTFT()->print("Thresh: ");
            display.getTFT()->print(stats.threshold, 0);
            
            display.getTFT()->setCursor(10, 170);
            display.getTFT()->print("SNR Req: ");
            display.getTFT()->print(stats.snrThreshold, 1);
            display.getTFT()->print("x");
            
            // Instructions
            display.getTFT()->setTextColor(TFT_DARKGREY);
            display.getTFT()->setCursor(10, 200);
            display.getTFT()->println("Test your beeper!");
            display.getTFT()->setCursor(10, 215);
            display.getTFT()->println("Watch for peaks");
            display.getTFT()->setCursor(10, 230);
            display.getTFT()->println("1400-2300Hz range");
            
            display.getTFT()->setCursor(10, 260);
            display.getTFT()->println("Hold BOOT 2s exit");
            display.getTFT()->setCursor(10, 275);
            display.getTFT()->println("Encoder: adjust");
            
            // LED color based on signal strength
            if (stats.snr > stats.snrThreshold) {
                leds[0] = CRGB::Red;  // Would trigger
            } else if (stats.snr > 1.5) {
                leds[0] = CRGB::Yellow;  // Getting close
            } else {
                leds[0] = CRGB::Cyan;  // Baseline
            }
            FastLED.show();
        }
        
        // Allow threshold adjustment with encoder in diagnostic mode
        static int lastEncoderDiag = 0;
        int newPosDiag = encoder.getPosition();
        if (newPosDiag != lastEncoderDiag) {
            int delta = newPosDiag - lastEncoderDiag;
            float currentThresh = micDetector.getStats().threshold;
            micDetector.adjustThreshold(currentThresh + (delta * 100));
            lastEncoderDiag = newPosDiag;
        }
        
        return;  // Skip normal operation in diagnostic mode
    }

    // Normal operation (non-diagnostic)
    // Check for beep detection in READY state
    if (timer.getState() == TIMER_READY && micDetector.update()) {
        // Beep detected! Auto-start timer
        USBSerial.println("Beep detected - auto-starting timer!");
        timer.start();
    }
    
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
            micDetector.startListening();  // Start listening for beep
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
                    micDetector.stopListening();  // Stop listening when manually starting
                    timer.start();
                } else if (timer.getState() == TIMER_RUNNING || timer.getState() == TIMER_FINISHED) {
                    micDetector.stopListening();  // Ensure mic is stopped
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
        if (millis() - lastDisplayUpdate > 20) {
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
    
    yield();
}