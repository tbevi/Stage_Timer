#ifndef MIC_DETECTOR_H
#define MIC_DETECTOR_H

#include <Arduino.h>
#include <driver/i2s.h>

/**
 * MicDetector - I2S Microphone interface for detecting 1.5kHz beep
 *
 * Uses SPH0645LM4H I2S MEMS microphone to listen for a 1.5kHz beep
 * during the shooter ready period. Implements Goertzel algorithm for
 * efficient frequency detection.
 */
class MicDetector {
public:
    MicDetector();

    /**
     * Initialize I2S peripheral and configure microphone
     * @return true if initialization successful
     */
    bool begin();

    /**
     * Start listening for beep (call when entering TIMER_READY state)
     */
    void startListening();

    /**
     * Stop listening for beep
     */
    void stopListening();

    /**
     * Update - call frequently in main loop
     * Processes audio samples and detects 1.5kHz beep
     * @return true if beep detected
     */
    bool update();

    /**
     * Check if currently listening for beep
     */
    bool isListening() const { return listening; }

    /**
     * Get the current detection magnitude (for debugging)
     */
    float getMagnitude() const { return lastMagnitude; }

    /**
     * Set detection threshold (default 5000.0)
     * Higher values = less sensitive, fewer false positives
     */
    void setThreshold(float threshold) { detectionThreshold = threshold; }

private:
    bool listening;
    float lastMagnitude;
    float detectionThreshold;

    // I2S configuration
    static constexpr i2s_port_t I2S_PORT = I2S_NUM_0;
    static constexpr int SAMPLE_RATE = 16000;  // 16kHz sample rate
    static constexpr int BLOCK_SIZE = 512;     // Samples per block
    static constexpr float TARGET_FREQ = 1500.0; // 1.5kHz target frequency

    // Goertzel algorithm state
    float coeff;           // Goertzel coefficient for target frequency
    int32_t audioBuffer[BLOCK_SIZE];

    /**
     * Calculate Goertzel coefficient for target frequency
     */
    void calculateCoefficient();

    /**
     * Process audio block using Goertzel algorithm
     * Returns magnitude of TARGET_FREQ component
     */
    float processBlock(int32_t* samples, int numSamples);
};

// Global instance
extern MicDetector micDetector;

#endif // MIC_DETECTOR_H
