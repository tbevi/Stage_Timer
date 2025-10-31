#ifndef PIN_CONFIG_H
#define PIN_CONFIG_H

// ===== DISPLAY PINS (Verified from example) =====
#define LCD_SCLK      10   // LCD_CLK - SPI Clock
#define LCD_MOSI      13   // LCD_DIN - SPI Data
#define LCD_DC        11   // LCD_DC - Data/Command
#define LCD_CS        12   // LCD_CS - Chip Select
#define LCD_RST       9    // LCD_RST - Reset
#define LCD_BL        2   // LCD_BL - Backlight (14 did not work)

// ===== IMU PINS (QMI8658C) =====
#define IIC_SDA       47
#define IIC_SCL       48
#define IMU_INT1      8
#define IMU_INT2      7

// ===== RGB LED - CORRECTED =====
#define RGB_LED_PIN   15    // 38 did not work, 2 did not work!
#define NUM_LEDS      2    // Or 8 if you have multiple
#define CHANNEL       0   // unknown

// ===== BUTTONS =====
#define BOOT_BUTTON   0

// ===== BATTERY MONITORING =====
#define BAT_ADC       4

// ===== SD CARD PINS =====
#define SD_MOSI       39
#define SD_MISO       40
#define SD_SCLK       41

// ===== ROTARY ENCODER (EC11) =====
#define ENCODER_CLK   16    // Encoder A
#define ENCODER_DT    17    // Encoder B
#define ENCODER_SW    18    // Push button

#endif // PIN_CONFIG_H