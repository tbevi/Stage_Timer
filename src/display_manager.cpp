#include "display_manager.h"
#include "settings.h" 
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
    
    // Draw angle
    tft.setTextSize(5);
    if (color == COLOR_GREEN || color == COLOR_CYAN) {
        tft.setTextColor(TFT_BLACK);
    } else {
        tft.setTextColor(TFT_WHITE);
    }
    
    char buf[10];
    sprintf(buf, "%.1f", angle);
    int textWidth = strlen(buf) * 30;
    int x = (170 - textWidth) / 2;
    
    tft.setCursor(x, 15);
    tft.print(angle, 1);
    
    // Draw status text
    tft.setTextSize(2);
    int statusWidth = strlen(status) * 12;
    tft.setCursor((170 - statusWidth) / 2, 80);
    tft.println(status);
    
    lastStatusColor = color;
    lastDisplayedAngle = angle;
}

void DisplayManager::updateLevelAngle(float angle, uint16_t color) {
    if (abs(angle - lastDisplayedAngle) > 0.05) {
        // Just update the angle text
        tft.fillRect(0, 10, 170, 60, color);
        
        tft.setTextSize(5);
        if (color == COLOR_GREEN || color == COLOR_CYAN) {
            tft.setTextColor(TFT_BLACK);
        } else {
            tft.setTextColor(TFT_WHITE);
        }
        
        char buf[10];
        sprintf(buf, "%.1f", angle);
        int textWidth = strlen(buf) * 30;
        int x = (170 - textWidth) / 2;
        
        tft.setCursor(x, 15);
        tft.print(angle, 1);
        
        lastDisplayedAngle = angle;
    }
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
        tft.println("Stage Timer v3.0");
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