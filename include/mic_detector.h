#ifndef MIC_DETECTOR_H
#define MIC_DETECTOR_H

#include <Arduino.h>
#include <driver/i2s.h>

/**
 * Statistics structure for diagnostic display
 */
struct MicStats {
    float currentMagnitude;
    float peakMagnitude;
    float noiseFloor;
    float snr;
    float detectedFrequency;
    float threshold;
    float snrThreshold;
};

/**
 * MicDetector - I2S Microphone interface for detecting beeps
 *
 * Uses SPH0645LM4H I2S MEMS microphone to listen for beeps in the
 * 1400-2300Hz range during the shooter ready period. Implements 
 * multiple Goertzel filters for efficient frequency detection.
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
     * Processes audio samples and detects beeps in target range
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
     * Set detection threshold
     * Higher values = less sensitive, fewer false positives
     */
    void setThreshold(float threshold) { detectionThreshold = threshold; }
    
    /**
     * Get current threshold value
     */
    float getThreshold() const { return detectionThreshold; }
    
    /**
     * Diagnostic mode - updates magnitude without detection logic
     * Call this in monitor mode instead of update()
     * @return current magnitude for display
     */
    float updateDiagnostic();
    
    /**
     * Get statistics for diagnostic display
     */
    float getNoiseFloor() const { return noiseFloor; }
    float getPeakMagnitude() const { return peakMagnitude; }
    float getAvgMagnitude() const { return avgMagnitude; }
    int getDetectionCount() const { return detectionCount; }
    float getDetectedFrequency() const { return detectedFrequency; }
    float getSNR() const { return (noiseFloor > 0) ? (lastMagnitude / noiseFloor) : 0; }
    
    /**
     * Reset statistics counters
     */
    void resetStats();
    
    /**
     * Get all stats at once for diagnostic display
     */
    MicStats getStats() const;
    
    /**
     * Start diagnostic mode (for BOOT button diagnostic)
     */
    void startDiagnostic();
    
    /**
     * Stop diagnostic mode
     */
    void stopDiagnostic();
    
    /**
     * Adjust detection threshold (for encoder in diagnostic)
     */
    void adjustThreshold(float newThreshold);
    
    /**
     * Adjust SNR threshold
     */
    void adjustSNRThreshold(float newSNR);

private:
    bool listening;
    bool diagnosticMode;  // For BOOT button diagnostic mode
    float lastMagnitude;
    float detectionThreshold;
    float detectedFrequency;
    float snrThreshold;
    
    // Statistics for diagnostics
    float noiseFloor;
    float peakMagnitude;
    float avgMagnitude;
    int detectionCount;
    unsigned long statsStartTime;
    int sampleCount;
    float magnitudeSum;

    // Debouncing to prevent false positives
    unsigned long lastDetectionTime;
    unsigned long detectionDebounceMs;

    // I2S configuration
    static constexpr i2s_port_t I2S_PORT = I2S_NUM_0;
    static constexpr int SAMPLE_RATE = 16000;  // 16kHz sample rate
    static constexpr int BLOCK_SIZE = 512;     // Samples per block
    
    // Frequency detection range
    static constexpr float MIN_FREQ = 1400.0;  // Minimum frequency to detect
    static constexpr float MAX_FREQ = 2300.0;  // Maximum frequency to detect
    static constexpr float FREQ_STEP = 100.0;  // Frequency resolution (100Hz steps)
    static constexpr int MAX_FREQ_BINS = 10;   // (2300-1400)/100 + 1

    // Goertzel algorithm state for multiple frequencies
    float coefficients[MAX_FREQ_BINS];
    float targetFrequencies[MAX_FREQ_BINS];
    int numFrequencies;
    int32_t audioBuffer[BLOCK_SIZE];

    /**
     * Calculate Goertzel coefficients for all target frequencies
     */
    void calculateCoefficients();

    /**
     * Process audio block using Goertzel algorithm
     * Returns magnitude of specified frequency component
     */
    float processBlock(int32_t* samples, int numSamples, float coeff);
    
    /**
     * Process block with all frequency filters
     * Returns the peak magnitude and sets detectedFrequency
     */
    float processMultiFrequency(int32_t* samples, int numSamples);

    /**
     * Estimate noise floor from ambient audio
     */
    void estimateNoiseFloor();
};

// Global instance
extern MicDetector micDetector;

#endif // MIC_DETECTOR_H