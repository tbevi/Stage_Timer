# Precision Rifle Competition Timer

Professional shot timer for PRS/NRL/DMR competitions.

## Features

- ⏱️ **Countdown timer** with multiple par time presets (60/90/120/150/180s)
- 📐 **Real-time level indicator** using IMU (unique feature!)
- 🎯 **Dual-sensor shot detection** (accelerometer + microphone)
- 🔊 **RO timer sync** via acoustic beep detection
- 📡 **Wireless start button** using ESP-NOW
- 💾 **SD card recording** for post-match analysis
- 🔋 **Long battery life** with deep sleep modes

## Hardware

- ESP32-S3 with 1.69" color LCD (240×280)
- QMI8658 6-axis IMU
- I2S MEMS microphone
- EC11 rotary encoder with push button
- Piezo buzzer (85+ dB)
- MicroSD card slot
- Wireless remote button (ESP32-C3 based)

## Status

🚧 **Work in Progress** 🚧

Currently in active development. Stay tuned!

## Getting Started

### Requirements

- VS Code with PlatformIO
- ESP32-S3 board support
- USB-C cable

### Build
```bash
git clone https://github.com/YOUR-USERNAME/precision-rifle-timer.git
cd precision-rifle-timer
```

Open in VS Code, PlatformIO will install dependencies automatically.

Click Upload (→) button in bottom toolbar.

## Roadmap

- [x] Project setup
- [ ] Display driver implementation
- [ ] IMU level indicator
- [ ] I2S microphone integration
- [ ] Shot detection algorithm
- [ ] Countdown timer logic
- [ ] SD card recording
- [ ] Wireless button
- [ ] Battery optimization
- [ ] Field testing

## License

MIT License 

## Author

Tbev - [GitHub Profile](https://github.com/tbevi)