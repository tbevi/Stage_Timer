# Changelog

All notable changes to the Stage Timer project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Planned
- Deep sleep mode for battery conservation
- SD card logging for diagnostics
- Wireless remote button (ESP-NOW)
- Shot counter with remaining shots display
- Battery level indicator
- Trainer mode with random start delay

---
## [3.3.1] - 2025-01-03

### Changed
- Changed sensor fusion ratio and loop timings to increase responsiveness


### Fixed
- version number on home screen shows correct version. 


### Known Bugs
- Version number does not re appear after entering settings menue.
- Hysteresis value show in setting menue overlaps box, should be removed all together.


## [3.3.0] - 2025-01-03

### Added
- **Gyroscope sensor fusion** for level detection
  - 98% gyroscope, 2% accelerometer complementary filter
  - Immunity to linear acceleration and vibration
  - Smooth, stable level readings during movement
- Automatic hysteresis calculation (10% of tolerance)
- Version tracking system (version.h)
- Changelog (this file)

### Changed
- Level detection now uses both gyroscope and accelerometer
- Hysteresis automatically scales with tolerance setting
- Improved level stability during rifle movement

### Removed
- Manual hysteresis adjustment from menu
  - Now auto-calculated, eliminating user configuration errors
  - Reduces menu complexity

### Fixed
- False level changes from shouldering rifle
- Jittery readings from recoil and vibration
- User confusion from manual hysteresis tuning

### Technical Details
- Using QMI8658 gyroscope at 896.8 Hz ODR
- Complementary filter implementation in level_monitor.cpp
- Hysteresis calculation: `tolerance * 0.1f`
- Business logic properly separated from settings storage

---

## [3.2.0] - 2024-12-XX

### Added
- Multi-frequency microphone detection (1400-2300Hz range)
- Microphone diagnostic mode (hold BOOT button for 2 seconds)
- Real-time magnitude and SNR display in diagnostic mode
- Adjustable microphone threshold via menu
- Encoder-based threshold tuning in diagnostic mode

### Changed
- Improved beep detection with multiple Goertzel filters
- Enhanced noise floor estimation
- Better false positive rejection with SNR threshold

### Technical Details
- 10 frequency bins covering 1400-2300Hz in 100Hz steps
- Goertzel algorithm for efficient single-frequency detection
- SNR threshold requirement (default 2.0x noise floor)

---

## [3.1.0] - 2024-XX-XX

### Added
- SPH0645LM4H I2S MEMS microphone integration
- Acoustic beep detection for RO timer sync
- Auto-start timer on beep detection in SHOOTER READY mode
- Manual timer start still available as fallback

### Technical Details
- I2S interface at 16kHz sample rate
- 1500Hz target frequency detection
- Goertzel algorithm implementation

---

## [3.0.0] - 2024-XX-XX

### Added
- EC11 rotary encoder with push button
- Full menu system for settings adjustment
- Countdown timer with adjustable par time (5-600 seconds)
- Color-coded timer warnings (yellow/red)
- CMT-1209-390T piezoelectric buzzer
- Audio feedback for timer events
- Settings persistence in flash memory
- Calibration system for level indicator

### Changed
- Complete UI redesign with menu navigation
- Timer display occupies bottom 2/3 of screen
- Level indicator in top 1/3 of screen

### Technical Details
- Settings stored using ESP32 Preferences library
- Encoder uses interrupt-based reading
- LEDC PWM for buzzer tone generation

---

## [2.0.0] - 2024-XX-XX

### Added
- QMI8658 6-axis IMU integration
- Real-time level indicator using accelerometer
- Color-coded level status (green/red/blue)
- RGB LED status indication
- Adjustable tolerance and hysteresis settings

### Technical Details
- IIR filtering for smooth angle readings
- Hysteresis-based state machine to prevent flicker
- Tilt angle calculation using atan2

---

## [1.0.0] - 2024-XX-XX

### Added
- Initial project setup
- ESP32-S3-LCD-1.9 board support
- 170x320 ST7789 display driver (LovyanGFX)
- Basic display functionality
- Project structure and build system

### Technical Details
- PlatformIO build system
- LovyanGFX library for display
- Custom display manager class

---

## Version Numbering Scheme

This project uses [Semantic Versioning](https://semver.org/):

**MAJOR.MINOR.PATCH**

- **MAJOR**: Incompatible API/hardware changes
- **MINOR**: New features, backward-compatible
- **PATCH**: Bug fixes, backward-compatible

### Examples:
- `3.3.0` → Major version 3, minor version 3, patch 0
- `4.0.0` → Breaking change (new hardware, API redesign)
- `3.3.1` → Bug fix for 3.3.0
- `3.4.0` → New feature added to 3.x line

---

## How to Update This Changelog

When making changes:

1. Add entry under `[Unreleased]` section
2. Categorize under: Added, Changed, Deprecated, Removed, Fixed, Security
3. When releasing, move `[Unreleased]` items to new version section
4. Update version number in `version.h`
5. Update display version string in code
6. Create git tag: `git tag v3.3.0`

---

## Links

- [Project Repository](https://github.com/tbevi/Stage_Timer)
- [Keep a Changelog](https://keepachangelog.com/)
- [Semantic Versioning](https://semver.org/)