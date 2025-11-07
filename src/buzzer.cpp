#include "buzzer.h"
#include "pin_config.h"
#include "settings.h"

Buzzer buzzer;

Buzzer::Buzzer() {
    isPlaying = false;
    toneEndTime = 0;
}

void Buzzer::begin() {
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);
    Serial.println("Buzzer: OK!");
}

void Buzzer::tone(uint16_t frequency, unsigned long duration) {
    // Convert volume (0-100) to duty cycle (0-255)
    // Using square root for more linear perceived volume
    int dutyCycle = map(settings.buzzerVolume, 0, 100, 0, 255);
    
    // Use ledc (LED PWM controller) for tone generation
    ledcSetup(1, frequency, 8);  // Channel 1, frequency, 8-bit resolution
    ledcAttachPin(BUZZER_PIN, 1);
    ledcWrite(1, dutyCycle);  // <-- Use calculated duty cycle
    
    isPlaying = true;
    toneEndTime = millis() + duration;
}

void Buzzer::noTone() {
    ledcWrite(1, 0);
    ledcDetachPin(BUZZER_PIN);
    digitalWrite(BUZZER_PIN, LOW);
    isPlaying = false;
}

void Buzzer::update() {
    if (isPlaying && millis() >= toneEndTime) {
        noTone();
    }
}

void Buzzer::beepStart() {
    // Single high-pitched chirp - "GO!"
    tone(2000, 150);
    Serial.println("Buzzer: Start beep");
}

void Buzzer::beepYellowWarning() {
    // Two medium beeps - "Warning!"
    tone(1500, 100);
    // Note: For multiple beeps, we'd need a more sophisticated system
    // For now, this gives one beep. We'll enhance this below.
    Serial.println("Buzzer: Yellow warning");
}

void Buzzer::beepRedWarning() {
    // Three urgent beeps - "Hurry!"
    tone(1200, 80);
    Serial.println("Buzzer: Red warning");
}

void Buzzer::beepFinished() {
    // Long alarm - "TIME!"
    tone(800, 1000);
    Serial.println("Buzzer: Finished alarm");
}