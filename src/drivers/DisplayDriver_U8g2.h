// DisplayDriver_U8g2.h - U8g2 implementation of DisplayDriver
// Wraps an existing U8G2 instance for monochrome OLED displays (128×64, etc.)
#pragma once

#include "../core/DisplayDriver.h"
#include <U8g2lib.h>

class DisplayDriver_U8g2 : public DisplayDriver {
public:
    explicit DisplayDriver_U8g2(U8G2& u8g2) : _u8g2(u8g2) {}

    int width() override  { return _u8g2.getDisplayWidth(); }
    int height() override { return _u8g2.getDisplayHeight(); }
    int colorDepth() override { return 1; }

    void begin() override       { _u8g2.begin(); }
    void clearBuffer() override { _u8g2.clearBuffer(); }
    void sendBuffer() override  { _u8g2.sendBuffer(); }
    void setPowerSave(bool on) override { _u8g2.setPowerSave(on ? 1 : 0); }
    void firstPage() override   { _u8g2.firstPage(); }

    void setFont(const uint8_t* font) override { _u8g2.setFont(font); }
    void setDrawColor(uint8_t color) override  { _u8g2.setDrawColor(color); }
    void setFontMode(uint8_t mode) override    { _u8g2.setFontMode(mode); }

    void drawStr(int x, int y, const char* str) override     { _u8g2.drawStr(x, y, str); }
    void drawUTF8(int x, int y, const char* str) override    { _u8g2.drawUTF8(x, y, str); }
    void drawGlyph(int x, int y, uint16_t encoding) override { _u8g2.drawGlyph(x, y, encoding); }

    int getStrWidth(const char* str) override   { return _u8g2.getStrWidth(str); }
    int getUTF8Width(const char* str) override  { return _u8g2.getUTF8Width(str); }
    int getFontAscent() override                { return _u8g2.getFontAscent(); }

    void drawBox(int x, int y, int w, int h) override            { _u8g2.drawBox(x, y, w, h); }
    void drawRBox(int x, int y, int w, int h, int r) override    { _u8g2.drawRBox(x, y, w, h, r); }
    void drawFrame(int x, int y, int w, int h) override          { _u8g2.drawFrame(x, y, w, h); }
    void drawHLine(int x, int y, int w) override                 { _u8g2.drawHLine(x, y, w); }
    void drawLine(int x1, int y1, int x2, int y2) override       { _u8g2.drawLine(x1, y1, x2, y2); }

    // Provide access to underlying U8G2 for any legacy code that still needs it
    U8G2& raw() { return _u8g2; }

private:
    U8G2& _u8g2;
};
