// BatteryMonitor.h — Compile-time type alias that selects the concrete battery
// monitoring implementation based on build flags.
//
// Priority:
//   1. MAX17048BatteryMonitor  (if USE_MAX17048 is true)
//   2. AnalogBatteryMonitor    (if USE_BATTERY_TEST is true)
//   3. NullBatteryMonitor      (no-op fallback)
//
// All callers use `BatteryMonitor` as the type — the concrete class is
// selected at compile time with no facade, no heap allocation, no vtable.
#pragma once

#include "../../static.h"   // ensures config_buttons.h macros + fallback defaults are available
#include "NullBatteryMonitor.h"

#if USE_MAX17048
  #include "MAX17048BatteryMonitor.h"
  using BatteryMonitor = MAX17048BatteryMonitor;
#elif USE_BATTERY_TEST
  #include "AnalogBatteryMonitor.h"
  using BatteryMonitor = AnalogBatteryMonitor;
#else
  using BatteryMonitor = NullBatteryMonitor;
#endif
