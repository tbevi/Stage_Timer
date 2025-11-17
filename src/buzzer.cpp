#include "buzzer.h"
#include "pin_config.h"
#include "settings.h"

Buzzer buzzer;

Buzzer::Buzzer() {
    isPlaying = false;
    toneEndTime = 0;
    playingPattern = false;
    patternCount = 0;
    patternIndex = 0;
    patternFreq = 0;
    patternBeepDuration = 0;
    patternGap = 0;
    patternNextTime = 0;
}

void Buzzer::begin() {
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);
    Serial.println("Buzzer: OK!");
}

void Buzzer::tone(uint16_t frequency, unsigned long duration) {
    // Convert volume (0-100) to duty cycle (0-255)
    // Using 50% duty cycle for square wave, scaled by volume
    int dutyCycle = map(settings.buzzerVolume, 0, 100, 0, 128);  // Max 50% duty cycle

    // Use ledc (LED PWM controller) for tone generation
    // Channel 2 to avoid conflict with LCD backlight (channel 0)
    ledcSetup(2, frequency, 8);  // Channel 2, frequency, 8-bit resolution
    ledcAttachPin(BUZZER_PIN, 2);
    ledcWrite(2, dutyCycle);

    isPlaying = true;
    toneEndTime = millis() + duration;
}

void Buzzer::noTone() {
    ledcWrite(2, 0);
    ledcDetachPin(BUZZER_PIN);
    digitalWrite(BUZZER_PIN, LOW);
    isPlaying = false;
}

void Buzzer::update() {
    // Handle single tone completion
    if (isPlaying && millis() >= toneEndTime) {
        noTone();
    }

    // Handle pattern playback
    if (playingPattern && !isPlaying && millis() >= patternNextTime) {
        if (patternIndex < patternCount) {
            tone(patternFreq, patternBeepDuration);
            patternIndex++;
            patternNextTime = millis() + patternBeepDuration + patternGap;
        } else {
            playingPattern = false;
        }
    }
}

void Buzzer::playPattern(uint16_t frequency, unsigned long beepDuration, int count, unsigned long gap) {
    playingPattern = true;
    patternFreq = frequency;
    patternBeepDuration = beepDuration;
    patternCount = count;
    patternGap = gap;
    patternIndex = 0;
    patternNextTime = millis();
}

void Buzzer::beepStart() {
    // Single high-pitched chirp - "GO!"
    tone(2000, 150);
    Serial.println("Buzzer: Start beep");
}

void Buzzer::beepYellowWarning() {
    // Two medium beeps - "Warning!"
    playPattern(1500, 100, 2, 80);
    Serial.println("Buzzer: Yellow warning");
}

void Buzzer::beepRedWarning() {
    // Three urgent beeps - "Hurry!"
    playPattern(1200, 80, 3, 60);
    Serial.println("Buzzer: Red warning");
}

void Buzzer::beepFinished() {
    // Long alarm - "TIME!"
    tone(800, 1000);
    Serial.println("Buzzer: Finished alarm");
}