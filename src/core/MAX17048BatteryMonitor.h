// MAX17048BatteryMonitor.h — I2C fuel gauge implementation of IBatteryMonitor.
//
// Uses the Adafruit MAX1704X library to read state-of-charge directly from
// the MAX17048 chip built into the Adafruit Feather ESP32-S3 (I2C addr 0x36).
//
// Guarded by USE_MAX17048 so the Adafruit library is only required in
// environments that declare it in lib_deps.
#pragma once

#include "IBatteryMonitor.h"

#if USE_MAX17048

#include <Wire.h>
#include <Adafruit_MAX1704X.h>

class MAX17048BatteryMonitor : public IBatteryMonitor {
public:
    explicit MAX17048BatteryMonitor(TwoWire &wire = Wire)
        : _wire(wire) {}

    bool begin() override {
        if (_gauge.begin(&_wire)) {
            _enabled = true;
            bool showPct = USE_BATTERY_PERCENT_AS_WELL_AS_ICON;
            _displayMode = showPct ? ICON_AND_PERCENT : ICON_ONLY;
            Serial.printf("[MAX17048] Found — chip 0x%04X\n", _gauge.getICversion());
            initialSample();
            return true;
        }
        _enabled = false;
        _displayMode = NONE;
        Serial.println("[MAX17048] NOT found on I2C bus");
        return false;
    }

    void loop() override {
        if (!_enabled) return;
        if (millis() - _lastCheckMs < SAMPLE_INTERVAL_MS) return;
        _lastCheckMs = millis();

        float pct = _gauge.cellPercent();
        _voltage = _gauge.cellVoltage();
        int raw = constrain((int)(pct + 0.5f), 0, 100);
        if (raw != _rawPercent) _rawPercent = raw;
        applySmoothing();
    }

    bool enabled()           const override { return _enabled; }
    int  percent()           const override { return BATTERY_SMOOTHING_ENABLED ? _smoothedPercent : _rawPercent; }
    int  percentRaw()        const override { return _rawPercent; }
    float voltage()          const override { return _voltage; }
    double lastCheckMillis() const override { return _lastCheckMs; }
    ShowBattery displayMode()const override { return _displayMode; }
    const char* name()       const override { return "MAX17048"; }

    void toggleDisplayMode() override {
        switch (_displayMode) {
            case ICON_ONLY:        _displayMode = ICON_AND_PERCENT; break;
            case ICON_AND_PERCENT: _displayMode = NONE;             break;
            case NONE: default:    _displayMode = ICON_ONLY;        break;
        }
    }

    bool shouldSleepForLowBattery() const override {
        return _enabled && (_rawPercent < USE_BATTERY_SLEEP_AT_PERCENT);
    }

private:
    void initialSample() {
        _lastCheckMs = millis();
        float pct = _gauge.cellPercent();
        _voltage = _gauge.cellVoltage();
        _rawPercent = constrain((int)(pct + 0.5f), 0, 100);
        _smoothedPercent = _rawPercent; // seed EMA
        Serial.printf("[MAX17048] Initial: %d%% @ %.2fV\n", _rawPercent, _voltage);
    }

    void applySmoothing() {
        if (BATTERY_SMOOTHING_ENABLED) {
            float alpha = BATTERY_SMOOTHING_ALPHA;
            float ema = (float)_smoothedPercent * (1.0f - alpha)
                      + (float)_rawPercent * alpha;
            _smoothedPercent = (int)(ema + 0.5f);
        } else {
            _smoothedPercent = _rawPercent;
        }
    }

    static constexpr unsigned long SAMPLE_INTERVAL_MS = 10000;

    TwoWire &_wire;
    Adafruit_MAX17048 _gauge;
    bool _enabled { false };
    ShowBattery _displayMode { NONE };
    int _rawPercent { 100 };
    int _smoothedPercent { 100 };
    float _voltage { 0.0f };
    double _lastCheckMs { -10000.0 };
};

#endif // USE_MAX17048
