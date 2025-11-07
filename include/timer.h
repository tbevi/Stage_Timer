#ifndef TIMER_H
#define TIMER_H

#include <Arduino.h>

enum TimerState {
    TIMER_IDLE,
    TIMER_READY,
    TIMER_RUNNING,
    TIMER_FINISHED
};

class CountdownTimer {
public:
    CountdownTimer();
    
    void setReady();
    void start();
    void reset();
    void update();
    
    TimerState getState() const { return state; }
    int getRemainingSeconds() const;
    float getPercentRemaining() const;
    uint16_t getTimerColor() const;
    
    bool needsRedraw() const { return redrawNeeded; }
    void clearRedrawFlag() { redrawNeeded = false; }
    
private:
    TimerState state;
    unsigned long startMillis;
    bool redrawNeeded;
};

extern CountdownTimer timer;

#endif