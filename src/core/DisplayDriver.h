// DisplayDriver.h - Abstract display driver interface
// Hides the difference between U8g2 (monochrome OLED) and TFT_eSPI (color TFT) displays.
#pragma once
#include <Arduino.h>

class DisplayDriver {
public:
    virtual ~DisplayDriver() = default;

    // Screen geometry
    virtual int width() = 0;
    virtual int height() = 0;
    virtual int colorDepth() = 0; // 1 = monochrome, 16 = RGB565 color

    // Frame buffer lifecycle
    virtual void begin() = 0;
    virtual void clearBuffer() = 0;
    virtual void sendBuffer() = 0;
    virtual void setPowerSave(bool on) = 0;
    virtual void firstPage() = 0;

    // Font selection (implementation maps to library-specific font types)
    virtual void setFont(const uint8_t* font) = 0;

    // Draw color: 0=clear/background, 1=set/foreground, 2=XOR
    virtual void setDrawColor(uint8_t color) = 0;

    // Text drawing
    virtual void drawStr(int x, int y, const char* str) = 0;
    virtual void drawUTF8(int x, int y, const char* str) = 0;
    virtual void drawGlyph(int x, int y, uint16_t encoding) = 0;

    // Text metrics
    virtual int getStrWidth(const char* str) = 0;
    virtual int getUTF8Width(const char* str) = 0;
    virtual int getFontAscent() = 0;

    // Primitives
    virtual void drawBox(int x, int y, int w, int h) = 0;
    virtual void drawRBox(int x, int y, int w, int h, int r) = 0;
    virtual void drawHLine(int x, int y, int w) = 0;
    virtual void drawLine(int x1, int y1, int x2, int y2) = 0;
};
