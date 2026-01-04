#ifndef VERSION_H
#define VERSION_H

// Stage Timer Version Information
// Update these values with each significant change

#define VERSION_MAJOR 3
#define VERSION_MINOR 3
#define VERSION_PATCH 1

// Build metadata (optional)
#define VERSION_BUILD "20250103"  // YYYYMMDD format

// Feature flags for this version
#define FEATURE_GYRO_FUSION     1  // Gyroscope sensor fusion enabled
#define FEATURE_AUTO_HYSTERESIS 1  // Automatic hysteresis calculation
#define FEATURE_MIC_DETECTION   1  // Microphone beep detection
#define FEATURE_SLEEP_MODE      0  // Deep sleep mode (not implemented)

// Version string for display
#define VERSION_STRING "3.3.0"
#define VERSION_FULL "Stage Timer v3.3.0 (Gyro+Auto-Hyst)"

// Helper macro to get version as single number for comparisons
#define VERSION_NUMBER ((VERSION_MAJOR * 10000) + (VERSION_MINOR * 100) + VERSION_PATCH)

/*
 * VERSION HISTORY:
 * 
 *  * 3.3.1 (2025-01-03)
 *   - fixed virsion number on home screen
 * 
 * 3.3.0 (2025-01-03)
 *   - Added gyroscope sensor fusion (98% gyro, 2% accel)
 *   - Removed manual hysteresis adjustment
 *   - Hysteresis now auto-calculated as 10% of tolerance
 *   - Improved level stability with movement immunity
 * 
 * 3.2.0 (2024-12-XX)
 *   - Multi-frequency microphone detection (1400-2300Hz)
 *   - Microphone diagnostic mode (hold BOOT 2s)
 *   - Added mic threshold adjustment
 * 
 * 3.1.0 (2024-XX-XX)
 *   - Basic microphone integration
 *   - Acoustic beep detection
 * 
 * 3.0.0 (2024-XX-XX)
 *   - Countdown timer with par time
 *   - Buzzer integration
 *   - Menu system with encoder
 * 
 * 2.0.0 (2024-XX-XX)
 *   - IMU-based level indicator
 *   - Accelerometer integration
 * 
 * 1.0.0 (2024-XX-XX)
 *   - Initial release
 *   - Basic display functionality
 */

#endif // VERSION_H