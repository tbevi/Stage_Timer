# Precision Rifle Competition Timer

Professional shot timer for PRS/NRL/DMR competitions.
## STATUS
 **Work in Progress** 

Currently in active development.
Phase 3 published. Trainer mode not yet integrated. SD Card Logging or any kind of logging not integrated
Working on Phase 3

## KNOWN BUGS

- Buzzer volume adjustment not working
- Screen brightness setting not working
- LED sticks red when timer ends until you enter and exit menu
- Beeps could use tweeking
- Microphone sensativity adjustment may not be working
- Microphone may suffer from false positives when triggering from shooter ready mode.
                                                                           
## Features

- Phase 1:
  - [x] **Real-time level indicator** using IMU (unique feature!)
- Phase 1.5:
  - [x] ~~Integrate gyro to avoid false level change readings due to left right movements~~
  - [x] Added filtering/hysterisis to help smoth out level reading.
- Phase 2
  - [x] Integrate EC11 click rotory encoder.
  - [x] **Countdown timer** with adjustable par time presets (60/90/120/150/180s)
- Phase 2.5
  - [x] Possible integration of beeper/buzzer/speaker 
- Phase 3
  - [x] Integrate SPH0645 Digital microphone for shot and RO beeper detection.
  - [x] Set up data logging (SD card?) for mic and IMU recording for offline testing of code.
  - [x] **RO timer sync** via acoustic beep detection
  - [x] Long hold boot button to activate microphone diagnostic view
- Future possible features
  - [ ] **Dual-sensor shot detection** (accelerometer + microphone)
  - [ ] **Wireless start button** using ESP-NOW
  - [ ] Setting to dissable mic
      

- Optional development
  - SW 
    - [ ] Consider adding "trainer" mode where when selected a random count down timer for start of part time.
    - [ ] Count shots and include shots remaining bar
    - [x] Battery bar
    - [x] Use gyro or something else to prevent left right movement form affecting level.
  - HW 
    - [ ] Remove volume control resistor from daughter board
    - [ ] Add mic controll mosfet to daughter board
    - [ ] Consider external start wired button (https://www.digikey.com/en/products/detail/panasonic-electronic-components/EVP-AWED4A/8568275)

- To Do
  - [ ] Upload pin diagrams
    
## Hardware

- ESP32-S3 with 1.9" color LCD (170x320) non touch
  -   https://www.waveshare.com/product/arduino/boards-kits/esp32-s3/esp32-s3-lcd-1.9.htm
- QMI8658 6-axis IMU
- I2S MEMS microphone (SPH0645) (https://www.amazon.com/dp/B0BBLW362R?ref=ppx_yo2ov_dt_b_fed_asin_title)
- EC11 rotary encoder with push button (https://www.amazon.com/dp/B07D3D64X7?ref=ppx_yo2ov_dt_b_fed_asin_title)
  - Integrated, connect pins as follows:
  - GPIO 16    // Encoder A
  - GPIO 17    // Encoder B
  - GPIO 18    // Push button
  - GND        // GND PINS
- Piezo buzzer (85+ dB) (https://www.amazon.com/dp/B0DHGP95K4?ref=ppx_yo2ov_dt_b_fed_asin_title&th=1)
- MicroSD card slot
- MX1.25 3.7V battery
- Wireless remote button (TBD but leaning toward ESP32-C3 based)

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
- [x] Buzzer integration
- [x] I2S microphone integration
- [ ] Shot detection algorithm
- [x] Countdown timer logic
- [ ] SD card recording
- [ ] Wireless button
- [ ] Battery optimization
- [ ] Field testing

## License

MIT License 

## Author

Tbev - [GitHub Profile](https://github.com/tbevi)
