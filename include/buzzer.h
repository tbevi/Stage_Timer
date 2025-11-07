#ifndef BUZZER_H
#define BUZZER_H

#include <Arduino.h>

class Buzzer {
public:
    Buzzer();
    
    void begin();
    
    // Different beep patterns
    void beepStart();        // Start timer - single chirp
    void beepYellowWarning(); // Yellow warning - two beeps
    void beepRedWarning();    // Red warning - three urgent beeps
    void beepFinished();      // Time's up - continuous alarm
    
    void update();  // Call in loop to handle non-blocking beeps
    
private:
    void tone(uint16_t frequency, unsigned long duration);
    void noTone();
    
    bool isPlaying;
    unsigned long toneEndTime;
};

extern Buzzer buzzer;

#endif