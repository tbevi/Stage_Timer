#include "mic_detector.h"
#include "pin_config.h"
#include <cmath>

// Global instance
MicDetector micDetector;

MicDetector::MicDetector()
    : listening(false)
    , diagnosticMode(false)
    , lastMagnitude(0.0)
    , detectionThreshold(1500.0)
    , detectedFrequency(0.0)
    , snrThreshold(2.5)  // Signal must be 2.5x noise floor (increased from 2.0)
    , noiseFloor(0.0)
    , peakMagnitude(0.0)
    , avgMagnitude(0.0)
    , detectionCount(0)
    , statsStartTime(0)
    , sampleCount(0)
    , magnitudeSum(0.0)
    , lastDetectionTime(0)
    , detectionDebounceMs(500)  // 500ms debounce to prevent false positives
{
}

bool MicDetector::begin() {
    Serial.println("Initializing I2S microphone...");

    // Calculate Goertzel coefficients for all target frequencies
    calculateCoefficients();

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

    // Initialize noise floor estimation
    estimateNoiseFloor();

    Serial.println("I2S Microphone: OK!");
    Serial.printf("Detecting beeps from %.0fHz to %.0fHz\n", MIN_FREQ, MAX_FREQ);
    Serial.printf("Noise floor: %.1f, Detection threshold: %.1f\n", 
                  noiseFloor, detectionThreshold);

    return true;
}

void MicDetector::calculateCoefficients() {
    // Calculate Goertzel coefficients for multiple frequencies
    // We'll check every 100Hz from 1400Hz to 2300Hz
    int freqIndex = 0;
    for (float freq = MIN_FREQ; freq <= MAX_FREQ; freq += FREQ_STEP) {
        float normalizedFreq = freq / SAMPLE_RATE;
        coefficients[freqIndex] = 2.0 * cos(2.0 * PI * normalizedFreq);
        targetFrequencies[freqIndex] = freq;
        freqIndex++;
        if (freqIndex >= MAX_FREQ_BINS) break;
    }
    numFrequencies = freqIndex;
    
    Serial.printf("Monitoring %d frequencies\n", numFrequencies);
}

void MicDetector::estimateNoiseFloor() {
    Serial.println("Estimating noise floor...");

    float sumMagnitude = 0.0;
    int samples = 0;
    const int calibrationTime = 1000; // 1 second calibration (increased from 500ms)
    unsigned long startTime = millis();

    while (millis() - startTime < calibrationTime) {
        size_t bytesRead = 0;
        esp_err_t err = i2s_read(I2S_PORT, audioBuffer, sizeof(audioBuffer),
                                 &bytesRead, 10);  // 10ms timeout

        if (err == ESP_OK && bytesRead > 0) {
            int samplesRead = bytesRead / sizeof(int32_t);

            // Process with middle frequency for noise estimation
            float magnitude = processBlock(audioBuffer, samplesRead,
                                          coefficients[numFrequencies/2]);
            sumMagnitude += magnitude;
            samples++;
        }
    }

    if (samples > 0) {
        noiseFloor = sumMagnitude / samples;
        // Add 30% margin to noise floor (increased from 20%)
        noiseFloor *= 1.3;
    } else {
        noiseFloor = 100.0; // Default if calibration fails
    }

    Serial.printf("Noise floor calibrated: %.1f (from %d samples)\n", noiseFloor, samples);
}

void MicDetector::startListening() {
    if (!listening) {
        listening = true;
        lastMagnitude = 0.0;
        detectedFrequency = 0.0;
        
        // Re-estimate noise floor for current conditions
        estimateNoiseFloor();
        
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
    if (!listening && !diagnosticMode) {
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

    // Process block with multiple Goertzel filters
    float magnitude = processMultiFrequency(audioBuffer, samplesRead);
    lastMagnitude = magnitude;

    // Detection logic: magnitude threshold AND signal-to-noise ratio
    float snr = (noiseFloor > 0) ? (magnitude / noiseFloor) : 0;
    
    // In diagnostic mode, don't auto-stop
    if (diagnosticMode) {
        // Update peak tracking
        if (magnitude > peakMagnitude) {
            peakMagnitude = magnitude;
        }
        return false;  // Never return true in diagnostic mode
    }
    
    // Check if we have a valid beep detection (normal listening mode)
    // Add debouncing to prevent rapid false triggers
    unsigned long now = millis();
    if (magnitude > detectionThreshold && snr > snrThreshold) {
        // Debounce: only detect if enough time has passed since last detection
        if (now - lastDetectionTime > detectionDebounceMs) {
            Serial.printf("BEEP DETECTED! Freq: %.0fHz, Mag: %.1f, SNR: %.2f\n",
                         detectedFrequency, magnitude, snr);
            detectionCount++;
            lastDetectionTime = now;
            stopListening();  // Stop after detection
            return true;
        }
    }

    return false;
}

float MicDetector::updateDiagnostic() {
    // Read audio samples from I2S
    size_t bytesRead = 0;
    esp_err_t err = i2s_read(I2S_PORT, audioBuffer, sizeof(audioBuffer),
                             &bytesRead, 0);  // Non-blocking read

    if (err != ESP_OK || bytesRead == 0) {
        return lastMagnitude;  // Return last value if no new data
    }

    int samplesRead = bytesRead / sizeof(int32_t);

    // Process block with multiple frequencies
    float magnitude = processMultiFrequency(audioBuffer, samplesRead);
    lastMagnitude = magnitude;
    
    // Update statistics
    sampleCount++;
    magnitudeSum += magnitude;
    avgMagnitude = magnitudeSum / sampleCount;
    
    // Update peak
    if (magnitude > peakMagnitude) {
        peakMagnitude = magnitude;
    }
    
    // Update noise floor (running minimum with slow decay)
    if (noiseFloor == 0.0) {
        noiseFloor = magnitude;
    } else {
        noiseFloor = noiseFloor * 0.999 + magnitude * 0.001;
        if (magnitude < noiseFloor) {
            noiseFloor = magnitude;
        }
    }
    
    // Count detections above threshold
    if (magnitude > detectionThreshold) {
        // Only count if it's been at least 100ms since we started tracking
        if (millis() - statsStartTime > 100) {
            detectionCount++;
        }
    }
    
    return magnitude;
}

void MicDetector::resetStats() {
    noiseFloor = 0.0;
    peakMagnitude = 0.0;
    avgMagnitude = 0.0;
    detectionCount = 0;
    statsStartTime = millis();
    sampleCount = 0;
    magnitudeSum = 0.0;
    detectedFrequency = 0.0;
    Serial.println("Mic stats reset");
}

MicStats MicDetector::getStats() const {
    MicStats stats;
    stats.currentMagnitude = lastMagnitude;
    stats.peakMagnitude = peakMagnitude;
    stats.noiseFloor = noiseFloor;
    stats.snr = (noiseFloor > 0) ? (lastMagnitude / noiseFloor) : 0;
    stats.detectedFrequency = detectedFrequency;
    stats.threshold = detectionThreshold;
    stats.snrThreshold = snrThreshold;
    return stats;
}

void MicDetector::startDiagnostic() {
    diagnosticMode = true;
    listening = true;  // Enable audio processing
    resetStats();
    Serial.println("=== MIC DIAGNOSTIC MODE ===");
    Serial.println("Monitoring frequencies 1400-2300Hz");
    Serial.println("Watch for peaks when beeper sounds");
}

void MicDetector::stopDiagnostic() {
    diagnosticMode = false;
    listening = false;
    Serial.println("=== DIAGNOSTIC MODE STOPPED ===");
    Serial.printf("Peak detected: %.1f @ %.0fHz\n", peakMagnitude, detectedFrequency);
    Serial.printf("Current threshold: %.1f, SNR required: %.1f\n", 
                  detectionThreshold, snrThreshold);
}

void MicDetector::adjustThreshold(float newThreshold) {
    detectionThreshold = constrain(newThreshold, 100.0, 10000.0);
    Serial.printf("Detection threshold set to: %.1f\n", detectionThreshold);
}

void MicDetector::adjustSNRThreshold(float newSNR) {
    snrThreshold = constrain(newSNR, 1.0, 10.0);
    Serial.printf("SNR threshold set to: %.1f\n", snrThreshold);
}

float MicDetector::processBlock(int32_t* samples, int numSamples, float coeff) {
    // Goertzel algorithm implementation
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
    float real = q1 - q2 * coeff * 0.5;
    float imag = q2 * sin(2.0 * PI / numSamples);  // Simplified for efficiency
    float magnitude = sqrt(real * real + imag * imag);

    // Scale magnitude
    magnitude *= numSamples;

    return magnitude;
}

float MicDetector::processMultiFrequency(int32_t* samples, int numSamples) {
    float maxMagnitude = 0.0;
    float maxFrequency = 0.0;
    
    // Process with all frequency filters
    for (int i = 0; i < numFrequencies; i++) {
        float magnitude = processBlock(samples, numSamples, coefficients[i]);
        
        if (magnitude > maxMagnitude) {
            maxMagnitude = magnitude;
            maxFrequency = targetFrequencies[i];
        }
    }
    
    detectedFrequency = maxFrequency;
    return maxMagnitude;
}