#ifndef POWER_MANAGER_H
#define POWER_MANAGER_H

#include <Arduino.h>
#include <SensorQMI8658.hpp>
#include <FastLED.h>
#include <driver/i2s.h>
#include "pin_config.h"

// RTC memory structure to preserve state across sleep cycles
typedef struct {
    uint32_t magic;              // Magic number to verify valid data
    uint32_t sleepCount;         // Number of sleep cycles
    uint32_t lastTimerValue;     // Timer value before sleep
    uint8_t lastTimerState;      // Timer state before sleep
    bool wasSleeping;            // Flag to indicate we woke from sleep
} RTC_DATA_ATTR SleepState;

// Wake-up causes
enum WakeupReason {
    WAKEUP_COLD_BOOT,           // Fresh power-on or reset
    WAKEUP_MOTION,              // IMU detected motion
    WAKEUP_BUTTON,              // Button pressed
    WAKEUP_TIMER,               // Auto-wake timeout
    WAKEUP_UNKNOWN              // Unknown reason
};

class PowerManager {
public:
    PowerManager();

    /**
     * Initialize power manager and check wake reason
     * @param qmiPtr Pointer to QMI8658 sensor object
     * @param ledsPtr Pointer to LED array
     */
    void begin(SensorQMI8658* qmiPtr, CRGB* ledsPtr);

    /**
     * Configure IMU for wake-on-motion detection
     * @param threshold Motion threshold in mg (milligravity), default 200mg
     * @param blankingTime Number of samples to ignore after wake, default 0x20
     * @return true if configuration successful
     */
    bool setupWakeOnMotion(uint8_t threshold = 200, uint8_t blankingTime = 0x20);

    /**
     * Enter deep sleep mode with motion wake capability
     * Shuts down all peripherals, configures wake sources, and enters sleep
     */
    void enterSleep();

    /**
     * Get the reason device woke up (or cold boot)
     */
    WakeupReason getWakeupReason() const { return wakeReason; }

    /**
     * Get wake reason as human-readable string
     */
    const char* getWakeupReasonString() const;

    /**
     * Check if device woke from sleep (vs cold boot)
     */
    bool isWakeFromSleep() const { return sleepState.wasSleeping; }

    /**
     * Get number of sleep cycles
     */
    uint32_t getSleepCount() const { return sleepState.sleepCount; }

    /**
     * Save current state to RTC memory before sleep
     * @param timerValue Current timer value to preserve
     * @param timerState Current timer state to preserve
     */
    void saveState(uint32_t timerValue, uint8_t timerState);

    /**
     * Restore state from RTC memory after wake
     * @param timerValue Output: restored timer value
     * @param timerState Output: restored timer state
     * @return true if valid state was restored
     */
    bool restoreState(uint32_t &timerValue, uint8_t &timerState);

    /**
     * Clear RTC memory state
     */
    void clearState();

    /**
     * Shutdown all peripherals in preparation for sleep
     */
    void shutdownPeripherals();

    /**
     * Restore peripherals after wake
     */
    void restorePeripherals();

    /**
     * Print wake statistics to serial
     */
    void printWakeStats() const;

private:
    SensorQMI8658* qmi;
    CRGB* leds;
    WakeupReason wakeReason;
    SleepState sleepState;

    static const uint32_t SLEEP_STATE_MAGIC = 0xDEADBEEF;

    /**
     * Determine wake-up reason from ESP32 sleep cause
     */
    void determineWakeReason();

    /**
     * Configure ESP32 wake sources
     */
    void configureWakeSources();

    /**
     * Initialize RTC memory structure
     */
    void initRTCMemory();
};

extern PowerManager powerManager;

#endif // POWER_MANAGER_H
