#include "mic_detector.h"
#include "pin_config.h"
#include <cmath>

// Global instance
MicDetector micDetector;

MicDetector::MicDetector()
    : listening(false)
    , lastMagnitude(0.0)
    , detectionThreshold(5000.0)
    , coeff(0.0)
{
}

bool MicDetector::begin() {
    Serial.println("Initializing I2S microphone...");

    // Calculate Goertzel coefficient for target frequency
    calculateCoefficient();

    // I2S configuration for SPH0645LM4H
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,  // Mono, left channel
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 4,
        .dma_buf_len = 1024,
        .use_apll = false,
        .tx_desc_auto_clear = false,
        .fixed_mclk = 0
    };

    // I2S pin configuration
    i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_BCLK,
        .ws_io_num = I2S_WS,
        .data_out_num = I2S_PIN_NO_CHANGE,  // Not used for RX
        .data_in_num = I2S_DIN
    };

    // Install and start I2S driver
    esp_err_t err = i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
    if (err != ESP_OK) {
        Serial.printf("ERROR: Failed to install I2S driver: %d\n", err);
        return false;
    }

    err = i2s_set_pin(I2S_PORT, &pin_config);
    if (err != ESP_OK) {
        Serial.printf("ERROR: Failed to set I2S pins: %d\n", err);
        i2s_driver_uninstall(I2S_PORT);
        return false;
    }

    // Start I2S
    i2s_start(I2S_PORT);

    Serial.println("I2S Microphone: OK!");
    Serial.printf("Listening for %.0f Hz beep (threshold: %.1f)\n",
                  TARGET_FREQ, detectionThreshold);

    return true;
}

void MicDetector::calculateCoefficient() {
    // Goertzel coefficient calculation
    // coeff = 2 * cos(2 * π * target_freq / sample_rate)
    float normalizedFreq = TARGET_FREQ / SAMPLE_RATE;
    coeff = 2.0 * cos(2.0 * PI * normalizedFreq);
}

void MicDetector::startListening() {
    if (!listening) {
        listening = true;
        lastMagnitude = 0.0;
        Serial.println("Mic: Listening for beep...");
    }
}

void MicDetector::stopListening() {
    if (listening) {
        listening = false;
        Serial.println("Mic: Stopped listening");
    }
}

bool MicDetector::update() {
    if (!listening) {
        return false;
    }

    // Read audio samples from I2S
    size_t bytesRead = 0;
    esp_err_t err = i2s_read(I2S_PORT, audioBuffer, sizeof(audioBuffer),
                             &bytesRead, 0);  // Non-blocking read

    if (err != ESP_OK || bytesRead == 0) {
        return false;  // No data available yet
    }

    int samplesRead = bytesRead / sizeof(int32_t);

    // Process block with Goertzel algorithm
    float magnitude = processBlock(audioBuffer, samplesRead);
    lastMagnitude = magnitude;

    // Check if magnitude exceeds threshold
    if (magnitude > detectionThreshold) {
        Serial.printf("BEEP DETECTED! Magnitude: %.1f\n", magnitude);
        stopListening();  // Stop after detection
        return true;
    }

    return false;
}

float MicDetector::processBlock(int32_t* samples, int numSamples) {
    // Goertzel algorithm implementation
    // Detects the magnitude of a specific frequency in a signal

    float q0 = 0.0;
    float q1 = 0.0;
    float q2 = 0.0;

    // Process each sample
    for (int i = 0; i < numSamples; i++) {
        // Convert 32-bit I2S sample to float and normalize
        // SPH0645 outputs 18-bit data in upper bits of 32-bit word
        float sample = (float)(samples[i] >> 14) / 131072.0;  // Normalize to roughly -1 to 1

        q0 = coeff * q1 - q2 + sample;
        q2 = q1;
        q1 = q0;
    }

    // Calculate magnitude
    // magnitude = sqrt(q1^2 + q2^2 - q1*q2*coeff)
    float real = q1 - q2 * coeff * 0.5;
    float imag = q2 * sin(2.0 * PI * TARGET_FREQ / SAMPLE_RATE);
    float magnitude = sqrt(real * real + imag * imag);

    // Scale magnitude
    magnitude *= numSamples;

    return magnitude;
}
