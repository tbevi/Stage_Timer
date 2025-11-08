# SPH0645LM4H Microphone Wiring Guide

## Overview
This document describes how to wire the SPH0645LM4H I2S MEMS microphone to the ESP32-S3 for beep detection functionality.

## Pin Connections

| Microphone Pin | ESP32-S3 GPIO | Description |
|----------------|---------------|-------------|
| 3V | 3.3V | Power supply |
| GND | GND | Ground |
| BCLK | GPIO 6  | Bit clock (I2S_BCLK) (BCLK)|
| DOUT | GPIO 5  | Data output (I2S_DIN) (DOUT)|
| LRCL | GPIO 33 | Left/Right clock / Word Select (I2S_WS) (LRCL)|
| SEL | GND or 3V | Channel select: GND=Left, 3V=Right |

### Notes:
- Connect SEL to GND for left channel (recommended)
- Connect SEL to 3.3V for right channel
- The current configuration expects left channel (SEL=GND)

## Functionality

### How It Works
1. When you long-press the encoder button (>1 second), the timer enters "SHOOTER READY" state
2. The microphone automatically starts listening for a 1.5kHz beep
3. When the beep is detected, the timer automatically starts counting down
4. Manual start still works - pressing the button will also start the timer

### Detection Parameters
- **Target Frequency:** 1.5kHz (1500 Hz)
- **Sample Rate:** 16kHz
- **Detection Threshold:** 5000.0 (adjustable via `micDetector.setThreshold()`)
- **Algorithm:** Goertzel algorithm for efficient frequency detection

### Usage
1. Connect the microphone as shown above
2. Upload the firmware to ESP32-S3
3. Long-press encoder button to enter SHOOTER READY mode
4. The display shows "SHOOTER READY" with yellow background
5. Play a 1.5kHz beep from the Range Officer's beeper
6. Timer automatically starts on beep detection
7. Serial monitor shows "Beep detected - auto-starting timer!"

### Troubleshooting

**Mic not detecting beeps:**
- Check wiring connections
- Verify beep frequency is actually 1.5kHz ±50Hz
- Increase detection threshold if too sensitive: `micDetector.setThreshold(8000.0)`
- Check serial monitor for magnitude readings during beep

**False triggers:**
- Decrease threshold for less sensitivity: `micDetector.setThreshold(3000.0)`
- Ensure environment is relatively quiet during READY period
- Check SEL pin is properly connected to GND or 3V (not floating)

**Mic initialization failed:**
- Verify pin connections match pin_config.h
- Check for I2S conflicts with other peripherals
- Ensure GPIO pins 42, 43, 44 are not used elsewhere

### Serial Debug Output
Monitor the serial output (115200 baud) for debugging:
```
Initializing microphone...
I2S Microphone: OK!
Listening for 1500 Hz beep (threshold: 5000.0)
Mic: Listening for beep...
BEEP DETECTED! Magnitude: 7234.5
Beep detected - auto-starting timer!
```

## Technical Details

### Goertzel Algorithm
The implementation uses the Goertzel algorithm for efficient single-frequency detection. This is more efficient than FFT when only detecting one specific frequency.

### I2S Configuration
- Mode: I2S Master RX
- Bits per sample: 32-bit
- Channel format: Mono (left channel only)
- Communication format: Standard I2S
- DMA buffers: 4 buffers × 1024 bytes

### Performance
- Non-blocking audio processing
- Approximately 32ms processing latency per 512-sample block
- Minimal CPU overhead (~5% at 16kHz sample rate)
