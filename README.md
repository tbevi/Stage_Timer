# Precision Rifle Competition Timer

Professional shot timer for PRS/NRL/DMR competitions.

## Features


- Phase 1:
  - **Real-time level indicator** using IMU (unique feature!)
- Phase 1.5:
  - Integrate gyro to avoid false level change readings due to left right movements
- Phase 2
  - Integrate EC11 click rotory encoder.
  - **Countdown timer** with adjustable par time presets (60/90/120/150/180s)
  - Consider adding "trainer" mode where when selected a random count down timer for start of part time.
- Phase 2.5
  - Possible integration of beeper/buzzer/speaker TBD
- Phase 3
  - Integrate SPH0645 Digital microphone for shot and RO beeper detection.
  - Set up data logging (SD card?) for mic and IMU recording for offline testing of code.
  - **RO timer sync** via acoustic beep detection
- Phase 3.5
  - **Dual-sensor shot detection** (accelerometer + microphone)
  - **Wireless start button** using ESP-NOW



## Hardware

- ESP32-S3 with 1.9" color LCD (170x320) non touch
  -   https://www.waveshare.com/product/arduino/boards-kits/esp32-s3/esp32-s3-lcd-1.9.htm
- QMI8658 6-axis IMU
- I2S MEMS microphone (SPH0645)
- EC11 rotary encoder with push button
- Piezo buzzer (85+ dB) (PN TBD)
- MicroSD card slot
- Wireless remote button (TBD but leaning toward ESP32-C3 based)

## Status

 **Work in Progress** 

Currently in active development.
Working on Phase 1

## Getting Started

### Requirements

- VS Code with PlatformIO
- ESP32-S3 board support https://www.waveshare.com/product/arduino/boards-kits/esp32-s3/esp32-s3-lcd-1.9.htm
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
- [x] Display driver implementation
- [x] IMU level indicator
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
