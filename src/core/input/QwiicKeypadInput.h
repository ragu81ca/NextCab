// QwiicKeypadInput.h – SparkFun Qwiic Keypad (I2C, default 0x4B) driver
// Implements IInputDevice; reads key presses from the FIFO and emits
// KeypadChar / KeypadSpecial events plus synthetic release events.
#pragma once

#include <Wire.h>
#include <SparkFun_Qwiic_Keypad_Arduino_Library.h>
#include "IInputDevice.h"
#include "InputEvents.h"

#ifndef QWIIC_KEYPAD_ADDRESS
  #define QWIIC_KEYPAD_ADDRESS 0x4B
#endif

// Time (ms) after which a synthetic key-release is emitted.
// The Qwiic Keypad hardware has no physical release detection.
#ifndef QWIIC_KEYPAD_RELEASE_MS
  #define QWIIC_KEYPAD_RELEASE_MS 150
#endif

class QwiicKeypadInput : public IInputDevice {
public:
    explicit QwiicKeypadInput(IInputDevice::DispatchFn dispatchFn,
                              TwoWire &wire = Wire,
                              uint8_t address = QWIIC_KEYPAD_ADDRESS)
        : _dispatch(dispatchFn), _wire(wire), _address(address) {}

    void begin() override {
        Serial.println("========== [QwiicKeypad] begin ==========");

        // Run an I2C scan so we can see what's on the bus
        // (Wire.begin() and I2C power already handled in setup())
        Serial.println("[QwiicKeypad] Scanning I2C bus...");
        uint8_t found = 0;
        for (uint8_t addr = 1; addr < 127; addr++) {
            _wire.beginTransmission(addr);
            if (_wire.endTransmission() == 0) {
                Serial.printf("[QwiicKeypad]   Device at 0x%02X\n", addr);
                found++;
            }
        }
        Serial.printf("[QwiicKeypad] Scan complete – %u device(s) found\n", found);

        if (_qkeypad.begin(_wire, _address)) {
            _ready = true;
            Serial.printf("[QwiicKeypad] Found at 0x%02X  FW %s\n",
                          _address, _qkeypad.getVersion().c_str());
        } else {
            _ready = false;
            Serial.printf("[QwiicKeypad] NOT found at 0x%02X – disabled\n", _address);
        }
    }

    void poll() override {
        if (!_ready) return;

        // 1. If a synthetic release is pending, check whether it's time to fire it.
        if (_releasePending && (millis() - _pressMs >= QWIIC_KEYPAD_RELEASE_MS)) {
            emitRelease(_pendingKey);
            _releasePending = false;
        }

        // 2. Read the keypad FIFO – updateFIFO tells the ATtiny to load the
        //    next button, then getButton reads it.
        _qkeypad.updateFIFO();
        char button = (char)_qkeypad.getButton();

        if (button != 0) {
            Serial.printf("[QwiicKeypad] Got key: '%c' (0x%02X)\n", button, (uint8_t)button);

            // If a previous key has a pending release, fire it immediately first.
            if (_releasePending) {
                emitRelease(_pendingKey);
                _releasePending = false;
            }

            emitPress(button);

            // Schedule a synthetic release.
            _pendingKey = button;
            _pressMs = millis();
            _releasePending = true;
        }
    }

    const char* name() const override { return "QwiicKeypad"; }

private:
    void emitPress(char k) {
        InputEvent ev;
        ev.timestamp = millis();
        ev.cvalue = k;
        bool isSpecial = (k == '*' || k == '#');
        ev.type = isSpecial ? InputEventType::KeypadSpecial : InputEventType::KeypadChar;
        if (_dispatch) _dispatch(ev);
    }

    void emitRelease(char k) {
        InputEvent ev;
        ev.timestamp = millis();
        ev.cvalue = k;
        bool isSpecial = (k == '*' || k == '#');
        ev.type = isSpecial ? InputEventType::KeypadSpecialRelease : InputEventType::KeypadCharRelease;
        if (_dispatch) _dispatch(ev);
    }

    KEYPAD _qkeypad;                        // SparkFun library object
    IInputDevice::DispatchFn _dispatch { nullptr };
    TwoWire &_wire;
    uint8_t _address;
    bool _ready { false };

    // Synthetic release tracking
    bool _releasePending { false };
    char _pendingKey { 0 };
    unsigned long _pressMs { 0 };
};
