// DisplayDriver_TFT_eSPI.h - TFT_eSPI implementation of DisplayDriver
// Drives ILI9341 (and other TFT_eSPI-supported panels) via SPI.
// Uses U8g2_for_TFT_eSPI to render U8g2 fonts on the TFT, keeping full
// font compatibility with the existing FontSet system.
//
// Requires: bodmer/TFT_eSPI, Bodmer/U8g2_for_TFT_eSPI, olikraus/U8g2
#pragma once

#include "../core/DisplayDriver.h"
#include <TFT_eSPI.h>
#include <U8g2_for_TFT_eSPI.h>

class DisplayDriver_TFT_eSPI : public DisplayDriver {
public:
    /// Construct with an optional sprite for double-buffered (flicker-free) rendering.
    /// If usePsramSprite is true and PSRAM is available, a full-screen sprite is
    /// allocated in PSRAM and all drawing goes there, then pushSprite() on sendBuffer().
    /// If false, drawing goes directly to the TFT (may flicker on full redraws).
    explicit DisplayDriver_TFT_eSPI(bool usePsramSprite = true)
        : _sprite(&_tft), _usePsramSprite(usePsramSprite) {}

    int width()  override { return _tft.width(); }
    int height() override { return _tft.height(); }
    int colorDepth() override { return 16; } // RGB565

    void begin() override {
        _tft.init();
        _tft.setRotation(1);           // landscape: 320 wide × 240 tall
        _tft.fillScreen(TFT_BLACK);

        _u8f.begin(_tft);              // attach U8g2 font engine to TFT

        // Allocate full-screen sprite in PSRAM for flicker-free rendering
        if (_usePsramSprite && psramFound()) {
            Serial.printf("[TFT] PSRAM found: %d bytes free\n", ESP.getFreePsram());
            _spriteOk = _sprite.createSprite(_tft.width(), _tft.height()) != nullptr;
            if (_spriteOk) {
                _u8f.begin(_sprite);   // redirect font rendering into the sprite
                Serial.println("[TFT] Sprite allocated, double-buffered mode");
            } else {
                Serial.println("[TFT] Sprite alloc failed, direct mode");
            }
        } else {
            Serial.println("[TFT] No PSRAM detected, direct mode");
        }

        _u8f.setFontMode(1);           // transparent mode: don't draw background behind text
    }

    void clearBuffer() override {
        if (_spriteOk) {
            _sprite.fillSprite(_bgColor);
        } else {
            _tft.fillScreen(_bgColor);
        }
    }

    void sendBuffer() override {
        if (_spriteOk) {
            _sprite.pushSprite(0, 0);
        }
        // If no sprite, drawing already went to TFT — nothing extra needed
    }

    void setPowerSave(bool on) override {
        // ILI9341 sleep commands via TFT_eSPI
        if (on) {
            _tft.writecommand(0x10); // SLPIN
        } else {
            _tft.writecommand(0x11); // SLPOUT
            delay(120);
        }
    }

    void firstPage() override {
        // Not meaningful for TFT — provided for interface compatibility
        clearBuffer();
    }

    // ── Font selection ──
    void setFont(const uint8_t* font) override {
        _u8f.setFont(font);
    }

    // ── Draw color mapping ──
    // 0 = background, 1 = foreground, 2 = XOR (approximated as foreground on TFT)
    void setDrawColor(uint8_t color) override {
        _drawColor = color;
        uint16_t c = (color == 0) ? _bgColor : _fgColor;
        _u8f.setForegroundColor(c);
        _u8f.setBackgroundColor((color == 0) ? _fgColor : _bgColor);
    }

    void setFontMode(uint8_t mode) override {
        _u8f.setFontMode(mode);
    }

    void setTextBackgroundColor(uint16_t color) override {
        _u8f.setBackgroundColor(color);
    }

    void setForegroundColor(uint16_t color) override {
        _u8f.setForegroundColor(color);
    }

    // ── Text drawing ──
    void drawStr(int x, int y, const char* str) override {
        _u8f.setCursor(x, y);
        _u8f.print(str);
    }

    void drawUTF8(int x, int y, const char* str) override {
        _u8f.setCursor(x, y);
        _u8f.print(str);
    }

    void drawGlyph(int x, int y, uint16_t encoding) override {
        _u8f.drawGlyph(x, y, encoding);
    }

    // ── Text metrics ──
    int getStrWidth(const char* str) override {
        return _u8f.getUTF8Width(str);
    }

    int getUTF8Width(const char* str) override {
        return _u8f.getUTF8Width(str);
    }

    int getFontAscent() override {
        return _u8f.getFontAscent();
    }

    // ── Drawing primitives ──
    void drawBox(int x, int y, int w, int h) override {
        uint16_t c = (_drawColor == 0) ? _bgColor : _fgColor;
        canvas().fillRect(x, y, w, h, c);
    }

    void drawRBox(int x, int y, int w, int h, int r) override {
        uint16_t c = (_drawColor == 0) ? _bgColor : _fgColor;
        canvas().fillRoundRect(x, y, w, h, r, c);
    }

    void drawFrame(int x, int y, int w, int h) override {
        uint16_t c = (_drawColor == 0) ? _bgColor : _fgColor;
        canvas().drawRect(x, y, w, h, c);
    }

    void drawHLine(int x, int y, int w) override {
        uint16_t c = (_drawColor == 0) ? _bgColor : _fgColor;
        canvas().drawFastHLine(x, y, w, c);
    }

    void drawLine(int x1, int y1, int x2, int y2) override {
        uint16_t c = (_drawColor == 0) ? _bgColor : _fgColor;
        canvas().drawLine(x1, y1, x2, y2, c);
    }

    void fillRect(int x, int y, int w, int h, uint16_t color) override {
        canvas().fillRect(x, y, w, h, color);
    }

    void fillRoundRect(int x, int y, int w, int h, int r, uint16_t color) override {
        canvas().fillRoundRect(x, y, w, h, r, color);
    }

    // ── Color configuration (can be called before begin) ──
    void setColors(uint16_t fg, uint16_t bg) {
        _fgColor = fg;
        _bgColor = bg;
    }

    // Provide access to underlying TFT for any code that needs it
    TFT_eSPI& raw() { return _tft; }

private:
    /// Return the canvas to draw on — sprite if available, else raw TFT
    TFT_eSPI& canvas() { return _spriteOk ? (TFT_eSPI&)_sprite : _tft; }

    TFT_eSPI       _tft;
    TFT_eSprite    _sprite;
    U8g2_for_TFT_eSPI _u8f;

    bool     _usePsramSprite = true;
    bool     _spriteOk       = false;
    uint8_t  _drawColor      = 1;
    uint16_t _fgColor        = TFT_WHITE;
    uint16_t _bgColor        = TFT_BLACK;
};
