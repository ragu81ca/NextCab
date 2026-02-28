// AnalogBatteryMonitor.h — ADC voltage-divider implementation of IBatteryMonitor.
//
// Wraps the existing Pangodream_18650_CL library, which reads an analog pin
// through a voltage divider and maps the voltage to a LiPo charge percentage.
//
// Guarded by USE_BATTERY_TEST so the Pangodream library is only compiled in
// when the user has configured an ADC pin and conversion factor.
#pragma once

#include "IBatteryMonitor.h"

#if USE_BATTERY_TEST

#include "../../Pangodream_18650_CL.h"

class AnalogBatteryMonitor : public IBatteryMonitor {
public:
    bool begin() override {
        Pangodream_18650_CL temp(BATTERY_TEST_PIN, BATTERY_CONVERSION_FACTOR);
        _battery = temp;
        _enabled = true;
        bool showPct = USE_BATTERY_PERCENT_AS_WELL_AS_ICON;
        _displayMode = showPct ? ICON_AND_PERCENT : ICON_ONLY;
        initialSample();
        Serial.printf("[AnalogBattery] Pin %d, CF %.2f — initial %d%%\n",
                      BATTERY_TEST_PIN, BATTERY_CONVERSION_FACTOR, _rawPercent);
        return true;
    }

    void loop() override {
        if (!_enabled) return;
        if (millis() - _lastCheckMs < SAMPLE_INTERVAL_MS) return;
        _lastCheckMs = millis();

        int raw = _battery.getBatteryChargeLevel();
        _lastAnalogRaw = _battery.getLastAnalogReadValue();
        if (raw != _rawPercent) _rawPercent = raw;
        applySmoothing();
    }

    bool enabled()           const override { return _enabled; }
    int  percent()           const override { return BATTERY_SMOOTHING_ENABLED ? _smoothedPercent : _rawPercent; }
    int  percentRaw()        const override { return _rawPercent; }
    float voltage()          const override { return 0.0f; } // not available from Pangodream
    double lastCheckMillis() const override { return _lastCheckMs; }
    ShowBattery displayMode()const override { return _displayMode; }
    const char* name()       const override { return "Analog"; }

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

    /// Extra accessor for legacy code that needs the raw ADC value.
    int lastAnalogRaw() const { return _lastAnalogRaw; }

private:
    void initialSample() {
        _lastCheckMs = millis();
        _rawPercent = _battery.getBatteryChargeLevel();
        _lastAnalogRaw = _battery.getLastAnalogReadValue();
        _smoothedPercent = _rawPercent; // seed EMA
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

    Pangodream_18650_CL _battery;
    bool _enabled { false };
    ShowBattery _displayMode { NONE };
    int _rawPercent { 100 };
    int _smoothedPercent { 100 };
    int _lastAnalogRaw { 0 };
    double _lastCheckMs { -10000.0 };
};

#endif // USE_BATTERY_TEST
