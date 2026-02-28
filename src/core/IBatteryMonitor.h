// IBatteryMonitor.h — Interface for battery monitoring implementations.
//
// Concrete implementations:
//   MAX17048BatteryMonitor — I2C fuel gauge (Adafruit Feather ESP32-S3)
//   AnalogBatteryMonitor   — ADC voltage divider (Pangodream_18650_CL)
//   NullBatteryMonitor     — no-op stub when no battery hardware is present
#pragma once

#include <Arduino.h>
#include "../../static.h" // ShowBattery enum, USE_BATTERY_SLEEP_AT_PERCENT

class IBatteryMonitor {
public:
    virtual ~IBatteryMonitor() = default;

    /// Attempt to initialise the battery hardware.
    /// Returns true if the hardware was found and is ready.
    virtual bool begin() = 0;

    /// Periodic update — call from loop().  Implementations should
    /// self-throttle (e.g. sample every 10 s).
    virtual void loop() = 0;

    /// Whether the monitor is active and producing data.
    virtual bool enabled() const = 0;

    /// Battery percentage for display (smoothed if the implementation supports it).
    virtual int percent() const = 0;

    /// Raw / unsmoothed percentage for critical decisions (e.g. sleep threshold).
    virtual int percentRaw() const = 0;

    /// Most recent cell voltage (0 if not available).
    virtual float voltage() const = 0;

    /// millis() timestamp of the last successful sample, or <= 0 if none yet.
    virtual double lastCheckMillis() const = 0;

    /// Current display mode (NONE / ICON_ONLY / ICON_AND_PERCENT).
    virtual ShowBattery displayMode() const = 0;

    /// Cycle through display modes.
    virtual void toggleDisplayMode() = 0;

    /// True when battery is critically low and the device should enter deep sleep.
    virtual bool shouldSleepForLowBattery() const = 0;

    /// Human-readable name for logging.
    virtual const char* name() const = 0;
};
