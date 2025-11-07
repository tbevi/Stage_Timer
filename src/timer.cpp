#include "timer.h"
#include "settings.h"

CountdownTimer timer;

// Color definitions
#define COLOR_WHITE  0xFFFF
#define COLOR_YELLOW 0xFFE0
#define COLOR_RED    0xF800

CountdownTimer::CountdownTimer() {
    state = TIMER_IDLE;
    startMillis = 0;
    redrawNeeded = true;
}

void CountdownTimer::setReady() {
    if (state == TIMER_IDLE || state == TIMER_FINISHED) {
        state = TIMER_READY;
        redrawNeeded = true;
        Serial.println("Timer READY");
    }
}

void CountdownTimer::start() {
    if (state == TIMER_READY) {
        state = TIMER_RUNNING;
        startMillis = millis();
        redrawNeeded = true;
        Serial.println("Timer STARTED");
    }
}

void CountdownTimer::reset() {
    state = TIMER_IDLE;
    redrawNeeded = true;
    Serial.println("Timer RESET");
}

void CountdownTimer::update() {
    if (state == TIMER_RUNNING) {
        int remaining = getRemainingSeconds();
        if (remaining <= 0) {
            state = TIMER_FINISHED;
            redrawNeeded = true;
            Serial.println("Timer FINISHED!");
        }
    }
}

int CountdownTimer::getRemainingSeconds() const {
    if (state == TIMER_RUNNING) {
        unsigned long elapsed = millis() - startMillis;
        int elapsedSec = elapsed / 1000;
        int remaining = settings.parTimeSeconds - elapsedSec;
        return max(0, remaining);
    }
    return settings.parTimeSeconds;
}

float CountdownTimer::getPercentRemaining() const {
    int remaining = getRemainingSeconds();
    return (float)remaining / (float)settings.parTimeSeconds;
}

uint16_t CountdownTimer::getTimerColor() const {
    int remaining = getRemainingSeconds();
    
    if (remaining > settings.yellowWarningSeconds) {
        return COLOR_WHITE;
    } else if (remaining > settings.redWarningSeconds) {
        return COLOR_YELLOW;
    } else {
        return COLOR_RED;
    }
}