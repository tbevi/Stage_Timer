#include "power_manager.h"
#include "display_manager.h"
#include "mic_detector.h"
#include <esp_sleep.h>
#include <esp_system.h>
#include <Wire.h>

#define USBSerial Serial

// Global instance
PowerManager powerManager;

// RTC memory variable - persists across deep sleep
RTC_DATA_ATTR SleepState rtcSleepState = {0};

PowerManager::PowerManager() {
    qmi = nullptr;
    leds = nullptr;
    wakeReason = WAKEUP_COLD_BOOT;
    memset(&sleepState, 0, sizeof(SleepState));
}

void PowerManager::begin(SensorQMI8658* qmiPtr, CRGB* ledsPtr) {
    qmi = qmiPtr;
    leds = ledsPtr;

    // Load state from RTC memory
    if (rtcSleepState.magic == SLEEP_STATE_MAGIC) {
        memcpy(&sleepState, &rtcSleepState, sizeof(SleepState));
        sleepState.wasSleeping = true;
    } else {
        initRTCMemory();
        sleepState.wasSleeping = false;
    }

    // Determine why we woke up
    determineWakeReason();

    // Print wake statistics
    printWakeStats();
}

void PowerManager::initRTCMemory() {
    memset(&rtcSleepState, 0, sizeof(SleepState));
    rtcSleepState.magic = SLEEP_STATE_MAGIC;
    rtcSleepState.sleepCount = 0;
    rtcSleepState.wasSleeping = false;
}

void PowerManager::determineWakeReason() {
    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();

    switch (cause) {
        case ESP_SLEEP_WAKEUP_EXT0:
            // Could be motion (IMU) or button
            // Check which pin triggered the wake
            if (esp_sleep_get_ext0_wakeup_pin() == IMU_INT1) {
                wakeReason = WAKEUP_MOTION;
                USBSerial.println("Wake source: IMU Motion Detection");
            } else {
                wakeReason = WAKEUP_BUTTON;
                USBSerial.println("Wake source: Button Press");
            }
            break;

        case ESP_SLEEP_WAKEUP_TIMER:
            wakeReason = WAKEUP_TIMER;
            USBSerial.println("Wake source: Timer");
            break;

        case ESP_SLEEP_WAKEUP_UNDEFINED:
        default:
            wakeReason = WAKEUP_COLD_BOOT;
            USBSerial.println("Wake source: Cold Boot / Reset");
            break;
    }
}

bool PowerManager::setupWakeOnMotion(uint8_t threshold, uint8_t blankingTime) {
    if (!qmi) {
        USBSerial.println("ERROR: QMI sensor not initialized!");
        return false;
    }

    USBSerial.println("\n=== Configuring Wake-on-Motion ===");
    USBSerial.print("Motion threshold: ");
    USBSerial.print(threshold);
    USBSerial.println(" mg");

    // Configure wake-on-motion with interrupt on INT1 pin
    // Pin default value HIGH (interrupt goes HIGH on motion)
    int result = qmi->configWakeOnMotion(
        threshold,                              // Threshold in mg
        SensorQMI8658::ACC_ODR_LOWPOWER_128Hz, // Low power mode for sleep
        SensorQMI8658::INTERRUPT_PIN_1,        // Use INT1 (GPIO 8)
        1,                                      // Pin defaults to HIGH
        blankingTime                            // Blanking time
    );

    if (result != DEV_WIRE_NONE) {
        USBSerial.println("ERROR: Failed to configure wake-on-motion!");
        return false;
    }

    // Enable the interrupt
    qmi->enableINT(SensorQMI8658::INTERRUPT_PIN_1, true);

    USBSerial.println("Wake-on-Motion configured successfully!");
    USBSerial.print("INT1 Pin (GPIO ");
    USBSerial.print(IMU_INT1);
    USBSerial.println(") ready for wake-up");

    return true;
}

void PowerManager::shutdownPeripherals() {
    USBSerial.println("\n=== Shutting down peripherals ===");

    // 1. Stop microphone and I2S driver
    USBSerial.println("Stopping microphone...");
    micDetector.stopListening();
    delay(100); // Allow time for I2S to finish
    i2s_driver_uninstall(I2S_NUM_0);
    USBSerial.println("I2S driver stopped");

    // 2. Detach encoder interrupts (non-RTC pins)
    USBSerial.println("Detaching encoder interrupts...");
    detachInterrupt(digitalPinToInterrupt(ENCODER_CLK));
    detachInterrupt(digitalPinToInterrupt(ENCODER_DT));

    // 3. Turn off display backlight
    USBSerial.println("Powering down display...");
    digitalWrite(LCD_BL, LOW);
    delay(10);

    // 4. Put display in sleep mode (if supported by LovyanGFX)
    display.getTFT()->sleep();

    // 5. Clear and turn off LEDs
    USBSerial.println("Clearing LEDs...");
    FastLED.clear();
    FastLED.show();

    // 6. Flush serial before sleep
    USBSerial.flush();
    delay(100);
}

void PowerManager::restorePeripherals() {
    USBSerial.println("\n=== Restoring peripherals ===");

    // 1. Wake up display
    display.getTFT()->wakeup();
    digitalWrite(LCD_BL, HIGH);
    USBSerial.println("Display powered on");

    // 2. Reattach encoder interrupts
    // Note: This will be done in main.cpp setup
    USBSerial.println("Encoder interrupts will be reattached in setup");

    // 3. Reinitialize I2S if needed
    // Note: This will be done when microphone is needed
    USBSerial.println("I2S will be reinitialized when needed");

    // 4. Clear IMU interrupt flag
    if (qmi) {
        uint8_t status = qmi->getIrqStatus();
        USBSerial.print("IMU interrupt status cleared: 0x");
        USBSerial.println(status, HEX);
    }
}

void PowerManager::configureWakeSources() {
    USBSerial.println("\n=== Configuring wake sources ===");

    // Wake source 1: IMU Motion Detection (INT1 on GPIO 8)
    // The pin will go HIGH when motion is detected
    esp_sleep_enable_ext0_wakeup((gpio_num_t)IMU_INT1, 1);
    USBSerial.print("Wake on motion: GPIO ");
    USBSerial.print(IMU_INT1);
    USBSerial.println(" (HIGH)");

    // Wake source 2: Optional - BOOT button (GPIO 0)
    // Note: Can only have one EXT0 wake source, so commenting this out
    // If you want button wake too, use EXT1 (multiple GPIO wake)
    // esp_sleep_enable_ext0_wakeup(GPIO_NUM_0, 0);

    // Wake source 3: Timer wake (safety fallback)
    // Wake after 5 minutes if no motion detected
    esp_sleep_enable_timer_wakeup(5 * 60 * 1000000ULL);
    USBSerial.println("Timer wake: 5 minutes");

    USBSerial.println("Wake sources configured!");
}

void PowerManager::saveState(uint32_t timerValue, uint8_t timerState) {
    rtcSleepState.lastTimerValue = timerValue;
    rtcSleepState.lastTimerState = timerState;
    rtcSleepState.magic = SLEEP_STATE_MAGIC;

    USBSerial.println("State saved to RTC memory");
}

bool PowerManager::restoreState(uint32_t &timerValue, uint8_t &timerState) {
    if (rtcSleepState.magic != SLEEP_STATE_MAGIC) {
        USBSerial.println("No valid RTC state found");
        return false;
    }

    timerValue = rtcSleepState.lastTimerValue;
    timerState = rtcSleepState.lastTimerState;

    USBSerial.println("State restored from RTC memory");
    return true;
}

void PowerManager::clearState() {
    initRTCMemory();
    memset(&sleepState, 0, sizeof(SleepState));
    USBSerial.println("RTC state cleared");
}

void PowerManager::enterSleep() {
    USBSerial.println("\n");
    USBSerial.println("====================================");
    USBSerial.println("===   ENTERING DEEP SLEEP MODE   ===");
    USBSerial.println("====================================");

    // Increment sleep counter
    rtcSleepState.sleepCount++;
    rtcSleepState.wasSleeping = true;

    USBSerial.print("Sleep cycle #");
    USBSerial.println(rtcSleepState.sleepCount);

    // Shutdown all peripherals
    shutdownPeripherals();

    // Configure IMU for wake-on-motion
    setupWakeOnMotion();

    // Configure ESP32 wake sources
    configureWakeSources();

    // Final message before sleep
    USBSerial.println("\nDevice will wake on:");
    USBSerial.println("  - Motion detected by IMU");
    USBSerial.println("  - 5 minute timeout");
    USBSerial.println("\nGoing to sleep...");
    USBSerial.flush();

    delay(100);

    // Enter deep sleep
    esp_deep_sleep_start();

    // Code never reaches here - ESP32 resets on wake
}

const char* PowerManager::getWakeupReasonString() const {
    switch (wakeReason) {
        case WAKEUP_COLD_BOOT: return "Cold Boot";
        case WAKEUP_MOTION:    return "Motion Detected";
        case WAKEUP_BUTTON:    return "Button Press";
        case WAKEUP_TIMER:     return "Timer Timeout";
        case WAKEUP_UNKNOWN:   return "Unknown";
        default:               return "Undefined";
    }
}

void PowerManager::printWakeStats() const {
    USBSerial.println("\n====================================");
    USBSerial.println("===    WAKE STATUS & STATS       ===");
    USBSerial.println("====================================");
    USBSerial.print("Wake Reason: ");
    USBSerial.println(getWakeupReasonString());
    USBSerial.print("Sleep Cycles: ");
    USBSerial.println(sleepState.sleepCount);
    USBSerial.print("Woke from sleep: ");
    USBSerial.println(sleepState.wasSleeping ? "YES" : "NO (Cold Boot)");
    USBSerial.println("====================================\n");
}
