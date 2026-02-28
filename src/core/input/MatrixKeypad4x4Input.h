// MatrixKeypad4x4Input.h — Self-contained 4×4 GPIO matrix keypad driver.
//
// Owns its own Keypad object, key map, and pin configuration.
// Pin defaults can be overridden via macros in config_buttons.h:
//   KEYPAD_ROW_PINS, KEYPAD_COLUMN_PINS, KEYPAD_DEBOUNCE_TIME, KEYPAD_HOLD_TIME
#pragma once

#include <Keypad.h>
#include "IInputDevice.h"
#include "InputEvents.h"
#include "../../../config_buttons.h"

// ── Dimensions ──────────────────────────────────────────────────
static constexpr byte MK4X4_ROWS = 4;
static constexpr byte MK4X4_COLS = 4;

// ── Key map ─────────────────────────────────────────────────────
static char _mk4x4Keys[MK4X4_ROWS][MK4X4_COLS] = {
    {'1','2','3','A'},
    {'4','5','6','B'},
    {'7','8','9','C'},
    {'*','0','#','D'}
};

// ── Pin defaults (platform-specific, overrideable via config_buttons.h) ─
#ifdef USE_TFT_ESPI
  // ESP32-S3 safe GPIOs
  #ifndef KEYPAD_ROW_PINS
    static byte _mk4x4RowPins[MK4X4_ROWS]  = {38, 39, 40, 41};
  #else
    static byte _mk4x4RowPins[MK4X4_ROWS]  = KEYPAD_ROW_PINS;
  #endif
  #ifndef KEYPAD_COLUMN_PINS
    static byte _mk4x4ColPins[MK4X4_COLS]  = {42, 2, 4, 8};
  #else
    static byte _mk4x4ColPins[MK4X4_COLS]  = KEYPAD_COLUMN_PINS;
  #endif
#else
  // Original ESP32 (lolin32_lite etc.)
  #ifndef KEYPAD_ROW_PINS
    static byte _mk4x4RowPins[MK4X4_ROWS]  = {19, 18, 17, 16};
  #else
    static byte _mk4x4RowPins[MK4X4_ROWS]  = KEYPAD_ROW_PINS;
  #endif
  #ifndef KEYPAD_COLUMN_PINS
    static byte _mk4x4ColPins[MK4X4_COLS]  = {4, 0, 2, 33};
  #else
    static byte _mk4x4ColPins[MK4X4_COLS]  = KEYPAD_COLUMN_PINS;
  #endif
#endif

// ── Timing defaults ─────────────────────────────────────────────
#ifndef KEYPAD_DEBOUNCE_TIME
  #define KEYPAD_DEBOUNCE_TIME 10
#endif
#ifndef KEYPAD_HOLD_TIME
  #define KEYPAD_HOLD_TIME 200
#endif

class MatrixKeypad4x4Input : public IInputDevice {
public:
    explicit MatrixKeypad4x4Input(IInputDevice::DispatchFn dispatchFn)
        : _dispatch(dispatchFn),
          _keypad(makeKeymap((char*)_mk4x4Keys),
                  _mk4x4RowPins, _mk4x4ColPins,
                  MK4X4_ROWS, MK4X4_COLS) {}

    void begin() override {
        _keypad.setDebounceTime(KEYPAD_DEBOUNCE_TIME);
        _keypad.setHoldTime(KEYPAD_HOLD_TIME);
        Serial.println("[MatrixKeypad4x4] Initialised");
    }

    void poll() override {
        if (_keypad.getKeys()) {
            for (byte i = 0; i < LIST_MAX; i++) {
                if (_keypad.key[i].stateChanged) {
                    char k = _keypad.key[i].kchar;
                    KeyState s = _keypad.key[i].kstate;
                    if (s == PRESSED || s == RELEASED) {
                        InputEvent ev;
                        ev.timestamp = millis();
                        ev.cvalue = k;
                        bool isSpecial = (k == '*' || k == '#');
                        if (s == PRESSED) {
                            ev.type = isSpecial ? InputEventType::KeypadSpecial
                                                : InputEventType::KeypadChar;
                        } else {
                            ev.type = isSpecial ? InputEventType::KeypadSpecialRelease
                                                : InputEventType::KeypadCharRelease;
                        }
                        if (_dispatch) _dispatch(ev);
                    }
                }
            }
        }
    }

    const char* name() const override { return "MatrixKeypad4x4"; }

private:
    IInputDevice::DispatchFn _dispatch { nullptr };
    Keypad _keypad;
};
