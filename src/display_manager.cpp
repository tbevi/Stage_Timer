#include "display_manager.h"
#include "settings.h" 

    // Left arrow bitmap (50x50)
    #define ARROW_LEFT_WIDTH 50
    #define ARROW_LEFT_HEIGHT 50
    static const unsigned char arrow_left_bits[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF8, 0x7F, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0xFF, 0xFF, 0x03, 0x00, 0x00, 0x00, 0xC0, 0xFF, 0xFF, 0x0F, 0x00, 
    0x00, 0x00, 0xE0, 0xFF, 0xFF, 0x1F, 0x00, 0x00, 0x00, 0xF8, 0x3F, 0xF0, 
    0x7F, 0x00, 0x00, 0x00, 0xFC, 0x07, 0x80, 0xFF, 0x00, 0x00, 0x00, 0xFE, 
    0x01, 0x00, 0xFE, 0x01, 0x00, 0x00, 0xFF, 0x00, 0x00, 0xFC, 0x03, 0x00, 
    0x00, 0x3F, 0x00, 0x00, 0xF0, 0x03, 0x00, 0x80, 0x1F, 0x00, 0x00, 0xE0, 
    0x07, 0x00, 0xC0, 0x0F, 0x00, 0x00, 0xC0, 0x0F, 0x00, 0xC0, 0x0F, 0x00, 
    0x00, 0xC0, 0x0F, 0x00, 0xE0, 0x07, 0x00, 0x00, 0x80, 0x1F, 0x00, 0xE0, 
    0x03, 0x00, 0x00, 0x00, 0x1F, 0x00, 0xE0, 0x03, 0x00, 0x00, 0x00, 0x1E, 
    0x00, 0xF0, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x01, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0xFF, 0x1F, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x0F, 
    0x00, 0x00, 0x00, 0x18, 0x00, 0xFE, 0x07, 0x00, 0x00, 0x00, 0x3C, 0x00, 
    0xFC, 0x03, 0x00, 0x00, 0x00, 0x7E, 0x00, 0xF8, 0x01, 0x00, 0x00, 0x00, 
    0xFF, 0x00, 0xF0, 0x00, 0x00, 0x00, 0x80, 0xFF, 0x01, 0x60, 0x00, 0x00, 
    0x00, 0xC0, 0xFF, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3E, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x3E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3E, 
    0x00, 0xE0, 0x01, 0x00, 0x00, 0x00, 0x1F, 0x00, 0xE0, 0x03, 0x00, 0x00, 
    0x00, 0x1F, 0x00, 0xE0, 0x07, 0x00, 0x00, 0x80, 0x1F, 0x00, 0xC0, 0x0F, 
    0x00, 0x00, 0xC0, 0x0F, 0x00, 0xC0, 0x0F, 0x00, 0x00, 0xC0, 0x0F, 0x00, 
    0x80, 0x1F, 0x00, 0x00, 0xE0, 0x07, 0x00, 0x00, 0x3F, 0x00, 0x00, 0xF0, 
    0x03, 0x00, 0x00, 0xFF, 0x00, 0x00, 0xFC, 0x03, 0x00, 0x00, 0xFE, 0x01, 
    0x00, 0xFE, 0x01, 0x00, 0x00, 0xFC, 0x07, 0x80, 0xFF, 0x00, 0x00, 0x00, 
    0xF8, 0x3F, 0xF0, 0x7F, 0x00, 0x00, 0x00, 0xE0, 0xFF, 0xFF, 0x1F, 0x00, 
    0x00, 0x00, 0xC0, 0xFF, 0xFF, 0x0F, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 
    0x03, 0x00, 0x00, 0x00, 0x00, 0xF8, 0x7F, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, };

    // Right arrow bitmap (50x50)
    #define ARROW_RIGHT_WIDTH 50
    #define ARROW_RIGHT_HEIGHT 50
    static const unsigned char arrow_right_bits[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF8, 0x7F, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0xFF, 0xFF, 0x03, 0x00, 0x00, 0x00, 0xC0, 0xFF, 0xFF, 0x0F, 0x00, 
    0x00, 0x00, 0xE0, 0xFF, 0xFF, 0x1F, 0x00, 0x00, 0x00, 0xF8, 0x3F, 0xF0, 
    0x7F, 0x00, 0x00, 0x00, 0xFC, 0x07, 0x80, 0xFF, 0x00, 0x00, 0x00, 0xFE, 
    0x01, 0x00, 0xFE, 0x01, 0x00, 0x00, 0xFF, 0x00, 0x00, 0xFC, 0x03, 0x00, 
    0x00, 0x3F, 0x00, 0x00, 0xF0, 0x03, 0x00, 0x80, 0x1F, 0x00, 0x00, 0xE0, 
    0x07, 0x00, 0xC0, 0x0F, 0x00, 0x00, 0xC0, 0x0F, 0x00, 0xC0, 0x0F, 0x00, 
    0x00, 0xC0, 0x0F, 0x00, 0xE0, 0x07, 0x00, 0x00, 0x80, 0x1F, 0x00, 0xE0, 
    0x03, 0x00, 0x00, 0x00, 0x1F, 0x00, 0xE0, 0x01, 0x00, 0x00, 0x00, 0x1F, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3E, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x3E, 0x00, 0x00, 0x00, 0x00, 0x00, 0xE0, 0xFF, 0x03, 0x60, 0x00, 
    0x00, 0x00, 0xC0, 0xFF, 0x03, 0xF0, 0x00, 0x00, 0x00, 0x80, 0xFF, 0x01, 
    0xF8, 0x01, 0x00, 0x00, 0x00, 0xFF, 0x00, 0xFC, 0x03, 0x00, 0x00, 0x00, 
    0x7E, 0x00, 0xFE, 0x07, 0x00, 0x00, 0x00, 0x3C, 0x00, 0xFF, 0x0F, 0x00, 
    0x00, 0x00, 0x18, 0x00, 0xF0, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 
    0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x01, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0xE0, 0x03, 0x00, 0x00, 0x00, 0x1E, 0x00, 0xE0, 0x03, 0x00, 0x00, 
    0x00, 0x1F, 0x00, 0xE0, 0x07, 0x00, 0x00, 0x80, 0x1F, 0x00, 0xC0, 0x0F, 
    0x00, 0x00, 0xC0, 0x0F, 0x00, 0xC0, 0x0F, 0x00, 0x00, 0xC0, 0x0F, 0x00, 
    0x80, 0x1F, 0x00, 0x00, 0xE0, 0x07, 0x00, 0x00, 0x3F, 0x00, 0x00, 0xF0, 
    0x03, 0x00, 0x00, 0xFF, 0x00, 0x00, 0xFC, 0x03, 0x00, 0x00, 0xFE, 0x01, 
    0x00, 0xFE, 0x01, 0x00, 0x00, 0xFC, 0x07, 0x80, 0xFF, 0x00, 0x00, 0x00, 
    0xF8, 0x3F, 0xF0, 0x7F, 0x00, 0x00, 0x00, 0xE0, 0xFF, 0xFF, 0x1F, 0x00, 
    0x00, 0x00, 0xC0, 0xFF, 0xFF, 0x0F, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 
    0x03, 0x00, 0x00, 0x00, 0x00, 0xF8, 0x7F, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, };

DisplayManager display;

LGFX::LGFX(void) {
    {
        auto cfg = _bus_instance.config();
        cfg.spi_host = SPI2_HOST;
        cfg.spi_mode = 0;
        cfg.freq_write = 80000000;
        cfg.freq_read = 16000000;
        cfg.spi_3wire = false;
        cfg.use_lock = true;
        cfg.dma_channel = SPI_DMA_CH_AUTO;
        cfg.pin_sclk = LCD_SCLK;
        cfg.pin_mosi = LCD_MOSI;
        cfg.pin_miso = -1;
        cfg.pin_dc = LCD_DC;
        
        _bus_instance.config(cfg);
        _panel_instance.setBus(&_bus_instance);
    }
    
    {
        auto cfg = _panel_instance.config();
        cfg.pin_cs = LCD_CS;
        cfg.pin_rst = LCD_RST;
        cfg.pin_busy = -1;
        cfg.panel_width = 170;
        cfg.panel_height = 320;
        cfg.offset_x = 35;
        cfg.offset_y = 0;
        cfg.offset_rotation = 0;
        cfg.dummy_read_pixel = 8;
        cfg.dummy_read_bits = 1;
        cfg.readable = false;
        cfg.invert = true;
        cfg.rgb_order = false;
        cfg.dlen_16bit = false;
        cfg.bus_shared = false;
        
        _panel_instance.config(cfg);
    }
    
    {
        auto cfg = _light_instance.config();
        cfg.pin_bl = LCD_BL;
        cfg.invert = false;
        cfg.freq = 44100;
        cfg.pwm_channel = 0;
        
        _light_instance.config(cfg);
        _panel_instance.setLight(&_light_instance);
    }
    
    setPanel(&_panel_instance);
}

DisplayManager::DisplayManager() {
    firstDraw = true;
    lastDrawnSeconds = -1;
    lastDisplayedAngle = 999;
    lastStatusColor = 0xFFFF;
    lastArrowDirection = 999;
}

void DisplayManager::begin() {
    tft.init();
    tft.setRotation(0);
    tft.fillScreen(TFT_BLACK);
}

void DisplayManager::setBrightness(int brightness) {
    tft.setBrightness(brightness);
}

void DisplayManager::drawLevelIndicator(float angle, uint16_t color, const char* status) {
    // Clear and redraw full level area
    tft.fillRect(0, 0, 170, 107, color);
    
    // Choose display mode
    if (settings.levelDisplayMode == LEVEL_DISPLAY_DEGREES) {
        // DEGREES MODE - Smart formatting
        tft.setTextSize(5);
        if (color == COLOR_GREEN || color == COLOR_CYAN) {
            tft.setTextColor(TFT_BLACK);
        } else {
            tft.setTextColor(TFT_WHITE);
        }
        
        // Smart formatting: show tenths only when < 10 degrees
        char buf[10];
        if (fabs(angle) < 10.0) {
            sprintf(buf, "%.1f", angle);  // Show tenths: 1.1, 5.3, 9.9
        } else {
            sprintf(buf, "%.0f", angle);  // No decimal: 10, 59, 134
        }
        
        int textWidth = strlen(buf) * 30;
        int x = (170 - textWidth) / 2;
        
        tft.setCursor(x, 15);
        tft.print(buf);
        
    } else {
        // ARROW MODE - Draw directional arrow
        drawDirectionalArrow(angle);
    }
    
    // Draw status text (only show "LEVEL" when centered, no CW/CCW)
    if (color == COLOR_GREEN || color == COLOR_CYAN) {
        tft.setTextSize(2);
        tft.setTextColor(TFT_BLACK);
        int statusWidth = 5 * 12;  // "LEVEL" is 5 chars
        tft.setCursor((170 - statusWidth) / 2, 80);
        tft.println("LEVEL");
    }
    // No text when red/blue - arrow/color shows direction
    
    lastStatusColor = color;
    lastDisplayedAngle = angle;
}

void DisplayManager::updateLevelAngle(float angle, uint16_t color) {
    if (settings.levelDisplayMode == LEVEL_DISPLAY_DEGREES) {
        // DEGREES MODE - update if angle changed enough
        if (abs(angle - lastDisplayedAngle) > 0.05) {
            tft.fillRect(0, 10, 170, 60, color);
            
            tft.setTextSize(5);
            if (color == COLOR_GREEN || color == COLOR_CYAN) {
                tft.setTextColor(TFT_BLACK);
            } else {
                tft.setTextColor(TFT_WHITE);
            }
            
            char buf[10];
            if (fabs(angle) < 10.0) {
                sprintf(buf, "%.1f", angle);
            } else {
                sprintf(buf, "%.0f", angle);
            }
            
            int textWidth = strlen(buf) * 30;
            int x = (170 - textWidth) / 2;
            
            tft.setCursor(x, 15);
            tft.print(buf);
            
            lastDisplayedAngle = angle;
        }
    } else {
        // ARROW MODE - Only redraw when direction changes
        
        // Determine current arrow direction
        int currentDirection;
        if (fabs(angle) < 0.3) {
            currentDirection = 0;  // Center (level)
        } else if (angle > 0) {
            currentDirection = -1; // Left arrow (CW tilt)
        } else {
            currentDirection = 1;  // Right arrow (CCW tilt)
        }
        
        // Only redraw if direction changed (eliminates flicker!)
        if (currentDirection != lastArrowDirection) {
            tft.fillRect(0, 10, 170, 60, color);
            drawDirectionalArrow(angle);
            lastArrowDirection = currentDirection;
            lastDisplayedAngle = angle;
        }
    }
}

void DisplayManager::drawDirectionalArrow(float angle) {
    // Arrow is always white regardless of background color
    tft.setTextColor(TFT_WHITE);
    
    // Center position for arrow
    int centerX = 85;  // 170 / 2
    int centerY = 35;  // Middle of upper section
    
    // Determine arrow direction
    // If angle is positive (CW), arrow points CCW (left)
    // If angle is negative (CCW), arrow points CW (right)
    // If near zero, show checkmark or no arrow
    
    if (fabs(angle) < 0.3) {
        // Near level - draw checkmark or level symbol
        drawCheckmark(centerX, centerY);
    } else if (angle > 0) {
        // Tilted CW - show arrow pointing CCW (left)
        drawCurvedArrow(centerX, centerY, true);  // true = left
    } else {
        // Tilted CCW - show arrow pointing CW (right)
        drawCurvedArrow(centerX, centerY, false);  // false = right
    }
}


void DisplayManager::drawCurvedArrow(int cx, int cy, bool pointLeft) {
    if (pointLeft) {
        tft.drawXBitmap(
            cx - ARROW_LEFT_WIDTH/2, 
            cy - ARROW_LEFT_HEIGHT/2,
            arrow_left_bits,
            ARROW_LEFT_WIDTH,
            ARROW_LEFT_HEIGHT,
            TFT_WHITE
        );
    } else {
        tft.drawXBitmap(
            cx - ARROW_RIGHT_WIDTH/2,
            cy - ARROW_RIGHT_HEIGHT/2,
            arrow_right_bits,
            ARROW_RIGHT_WIDTH,
            ARROW_RIGHT_HEIGHT,
            TFT_WHITE
        );
    }
}

void DisplayManager::drawCheckmark(int cx, int cy) {
    // Do nothing, leave screen green when level
}

void DisplayManager::drawProgressBar(int x, int y, int width, int height, float percentage, uint16_t color) {
    tft.drawRect(x, y, width, height, TFT_DARKGREY);
    
    int fillWidth = (int)((width - 4) * percentage);
    if (fillWidth > 0) {
        tft.fillRect(x + 2, y + 2, fillWidth, height - 4, color);
    }
    
    int remainWidth = (width - 4) - fillWidth;
    if (remainWidth > 0) {
        tft.fillRect(x + 2 + fillWidth, y + 2, remainWidth, height - 4, TFT_BLACK);
    }
}

void DisplayManager::drawTimerDisplay(int remainingSeconds, float percentage, uint16_t timerColor, const char* stateText) {
    int timerY = 107;
    
    // Clear if seconds changed
    if (remainingSeconds != lastDrawnSeconds) {
        tft.fillRect(0, timerY, 170, 213, TFT_BLACK);
        lastDrawnSeconds = remainingSeconds;
    }
    
    // Draw state text
    tft.setTextSize(1);
    tft.setTextColor(TFT_DARKGREY);
    tft.setCursor(10, timerY + 10);
    tft.println(stateText);
    
    // Draw large time
    tft.setTextSize(5);
    tft.setTextColor(timerColor);
    
    int minutes = remainingSeconds / 60;
    int seconds = remainingSeconds % 60;
    
    char timeStr[10];
    sprintf(timeStr, "%d:%02d", minutes, seconds);
    
    int textWidth = strlen(timeStr) * 30;
    int x = (170 - textWidth) / 2;
    tft.setCursor(x, timerY + 50);
    tft.print(timeStr);
    
    // Draw progress bar
    drawProgressBar(10, timerY + 130, 150, 30, percentage, timerColor);
    
    // Draw par time reference
    tft.setTextSize(1);
    tft.setTextColor(TFT_DARKGREY);
    tft.setCursor(10, timerY + 175);
    tft.print("Par: ");
    tft.print(settings.parTimeSeconds);
    tft.print("s");
    
    // Draw version at bottom
    if (firstDraw) {
        tft.setTextSize(1);
        tft.setTextColor(TFT_DARKGREY);
        tft.setCursor(10, 300);
        tft.println("Stage Timer v3.3");
        firstDraw = false;
    }
}

void DisplayManager::drawShooterReady() {
    // Full screen yellow with black text
    tft.fillScreen(COLOR_YELLOW);
    
    // Large "SHOOTER READY" text
    tft.setTextSize(3);
    tft.setTextColor(TFT_BLACK);
    
    // Center "SHOOTER" on line 1
    tft.setCursor(10, 100);
    tft.println("SHOOTER");
    
    // Center "READY" on line 2
    tft.setCursor(30, 140);
    tft.println("READY");
    
    // Instructions at bottom
    tft.setTextSize(2);
    tft.setCursor(15, 240);
    tft.println("Press to");
    tft.setCursor(30, 265);
    tft.println("START");
    
    // Force redraw flag reset so next state redraws completely
    firstDraw = true;
    lastDrawnSeconds = -1;
    lastDisplayedAngle = 999;
    lastStatusColor = 0xFFFF;
}

void DisplayManager::drawMicDiagnostics(float magnitude, float threshold, float noiseFloor, 
                                        float peakMag, float avgMag, int detections) {
    // Clear screen
    tft.fillScreen(TFT_BLACK);
    
    // Title
    tft.setTextSize(2);
    tft.setTextColor(COLOR_CYAN);
    tft.setCursor(10, 10);
    tft.println("MIC DIAGNOSTIC");
    
    // Current magnitude - large display
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE);
    tft.setCursor(10, 40);
    tft.print("Magnitude:");
    
    tft.setTextSize(3);
    tft.setTextColor(COLOR_CYAN);
    tft.setCursor(10, 55);
    tft.printf("%.0f", magnitude);
    
    // Bar graph for magnitude (0-5000 scale)
    int barY = 95;
    int barHeight = 25;
    int barMaxWidth = 150;
    
    // Background bar
    tft.fillRect(10, barY, barMaxWidth, barHeight, TFT_DARKGREY);
    
    // Magnitude bar (green if below threshold, red if above)
    float normalizedMag = constrain(magnitude / 5000.0, 0.0, 1.0);
    int magBarWidth = (int)(barMaxWidth * normalizedMag);
    uint16_t barColor = (magnitude > threshold) ? COLOR_RED : COLOR_GREEN;
    
    if (magBarWidth > 0) {
        tft.fillRect(10, barY, magBarWidth, barHeight, barColor);
    }
    
    // Threshold line
    float normalizedThresh = constrain(threshold / 5000.0, 0.0, 1.0);
    int threshX = 10 + (int)(barMaxWidth * normalizedThresh);
    tft.drawFastVLine(threshX, barY - 5, barHeight + 10, COLOR_YELLOW);
    tft.drawFastVLine(threshX + 1, barY - 5, barHeight + 10, COLOR_YELLOW);
    
    // Detection flash
    if (magnitude > threshold) {
        tft.setTextSize(2);
        tft.setTextColor(COLOR_RED);
        tft.setCursor(15, 130);
        tft.println("DETECTED!");
    }
    
    // Statistics section
    int statsY = 160;
    tft.setTextSize(1);
    tft.setTextColor(TFT_LIGHTGREY);
    
    tft.setCursor(10, statsY);
    tft.print("Threshold:");
    tft.setTextColor(COLOR_YELLOW);
    tft.printf(" %.0f", threshold);
    
    tft.setTextColor(TFT_LIGHTGREY);
    tft.setCursor(10, statsY + 15);
    tft.print("Noise Floor:");
    tft.setTextColor(TFT_WHITE);
    tft.printf(" %.0f", noiseFloor);
    
    tft.setTextColor(TFT_LIGHTGREY);
    tft.setCursor(10, statsY + 30);
    tft.print("Peak:");
    tft.setTextColor(TFT_WHITE);
    tft.printf(" %.0f", peakMag);
    
    tft.setTextColor(TFT_LIGHTGREY);
    tft.setCursor(10, statsY + 45);
    tft.print("Average:");
    tft.setTextColor(TFT_WHITE);
    tft.printf(" %.0f", avgMag);
    
    tft.setTextColor(TFT_LIGHTGREY);
    tft.setCursor(10, statsY + 60);
    tft.print("Detections:");
    tft.setTextColor(TFT_WHITE);
    tft.printf(" %d", detections);
    
    // Signal to Noise Ratio
    if (noiseFloor > 0) {
        float snr = peakMag / noiseFloor;
        tft.setTextColor(TFT_LIGHTGREY);
        tft.setCursor(10, statsY + 75);
        tft.print("SNR:");
        
        // Color code SNR: green > 10, yellow > 5, red < 5
        if (snr > 10) {
            tft.setTextColor(COLOR_GREEN);
        } else if (snr > 5) {
            tft.setTextColor(COLOR_YELLOW);
        } else {
            tft.setTextColor(COLOR_RED);
        }
        tft.printf(" %.1f:1", snr);
    }
    
    // Instructions at bottom
    tft.setTextSize(1);
    tft.setTextColor(TFT_DARKGREY);
    tft.setCursor(10, 285);
    tft.println("Turn: Adj Threshold");
    tft.setCursor(10, 300);
    tft.println("Press: Exit");
}