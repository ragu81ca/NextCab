# NextCab — TODO

Living document for planned features, in rough priority order.

---

## 0. PlatformIO Project Structure Cleanup

**Status:** In progress

Refactor to standard PlatformIO layout so firmware builds, tests, and includes are
cleanly separated without per-environment source filters.

- [~] Move main firmware sources to `src/` and shared headers to `include/`
  - Sources moved to `src/`; shared headers (`WiTcontroller.h`, `static.h`, `config_*.h`)
    still live at the repo root rather than `include/`
- [x] Remove root-level `src_dir = .` / `include_dir = .` assumptions in `platformio.ini`
  - `src_dir = src`; no `include_dir` override
- [x] Ensure `pio run` builds firmware only and `pio test` builds tests only
  - `env:native_test` uses `build_src_filter = -<*>` and `test_filter = test_native`
- [ ] Update include paths away from root-coupled relative chains where practical
  - e.g. `#include "../../../WiTcontroller.h"` in `src/core/protocol/WiThrottleDelegate.cpp`
- [ ] Add CI checks for at least one embedded env + native test env
  - No `.github/workflows/` present yet

---

## 1. RadioSelectScreen (UI primitive)

**Status:** ✔ COMPLETED  
**Depends on:** Nothing  
**Unblocks:** Loco config wizard, server prefix config, future settings screens

A screen type for choosing one option from a short list (3–8 items).
Renders a title, a list of labelled options with a selection indicator on the
currently highlighted item, and a footer with key instructions.

- [x] `RadioSelectScreen` data model (`src/core/ui/RadioSelectScreen.h`)
  - Title string, items array, selected index, confirm/cancel callbacks
  - Follows the data-only pattern (no pixel knowledge)
- [x] `Renderer::renderRadioSelect()` — draw method in Renderer
  - Selected item gets inverted row or bullet indicator
  - Digit keys select directly; `#` confirms; `*` cancels
  - Scroll indicators (chevrons) for overflow
- [x] Verify rendering on 128×64 OLED layout (implemented and in use in LocoConfigWizardHandler)
- [x] Active use in LocoConfigWizardHandler for loco type/config steps

---

## 2. Loco Configuration Wizard

**Status:** ✔ COMPLETED  
**Depends on:** RadioSelectScreen ✔ Done  
**Unblocks:** User-configurable per-loco sound functions, loco type selection

A multi-step wizard that runs through the same sequence of questions each time
a user configures a locomotive.  Previously saved values are pre-populated.

### Wizard steps

| Step | Screen Type | Question |
|------|-------------|----------|
| 1 | RadioSelect | **Loco type** — Diesel / Steam / Electric |
| 2 | TextInput | **Top speed** — MPH/KPH (converted to internal speed step cap) |
| 3 | TextInput | **Throttle up function** — F-number (Diesel/Electric only; skipped for Steam) |
| 4 | TextInput | **Throttle down function** — F-number (Diesel/Electric only; skipped for Steam) |
| 5 | TextInput | **Brake sound function** — F-number (all types) |
| 6 | TextInput | **Dynamic brake sound function** — F-number (all types) |

### Implementation status

- [x] `InputMode::LocoConfigWizard` — defined and routed
- [x] `LocoConfigWizardHandler` — fully implemented with all steps (PickLoco, LocoType, TopSpeed, function steps)
  - Loads `LocoConfig` from `ConfigStore` on entry, pre-populates each step
  - On confirm: calls `ConfigStore::saveLocoConfig()` and returns to Operation mode
- [x] Entry point — from Extras menu via `handleLocoConfig()` in MenuDefinitions
- [x] Wire `LocoType` selection back to `MomentumController::setLocoType()`
- [x] Live-update `LocoManager` sound config cache and `MomentumController` on save
- [x] Steam step skipping implemented (FuncThrottleUp/FuncThrottleDown skipped)
- [x] Empty function input supported (`-1` not configured) for all sound roles
- [x] Per-loco top speed cap persisted and applied to consist limiting

### Sound questions by loco type

| Field | Diesel | Steam | Electric |
|-------|--------|-------|----------|
| Throttle Up (notch up) | Ask | Skip | Ask |
| Throttle Down (notch down) | Ask | Skip | Ask |
| Brake sound | Ask | Ask | Ask |
| Dynamic brake sound | Ask | Ask | Ask |

Steam locomotives don't have throttle notch sounds.  The wizard skips those steps.

---

## 3. Server Prefix Configuration (per WiThrottle server)

**Status:** Not started  
**Depends on:** TextInputScreen (already exists)  
**Unblocks:** User-configurable route/turnout prefixes without recompiling

### Context

Different WiThrottle servers use different prefix schemes for turnout and route
system names.  DCC-EX auto-detects (`DCC_EX_TURNOUT_PREFIX` / `DCC_EX_ROUTE_PREFIX`),
but JMRI users must currently hard-code them.

`ServerConfig` already has `turnoutPrefix` and `routePrefix` fields.
`ConfigStore::saveServer()` already persists them.
`ServerDataStore` already applies them at runtime.

### Implementation plan

- [ ] New menu entry (Extras → Server Settings, or *9 submenu)
- [ ] Two-step TextInput flow:
  1. "Turnout prefix:" → pre-populated with current `serverDataStore.turnoutPrefix()`
  2. "Route prefix:" → pre-populated with current `serverDataStore.routePrefix()`
- [ ] On confirm: save to `ConfigStore` and update `ServerDataStore` live
- [ ] Only available when connected (context-sensitive menu visibility)

---

## 4. Device Settings Configuration Wizard

**Status:** In progress — core runtime preferences implemented; display and battery settings remain  
**Depends on:** RadioSelectScreen ✔, TextInputScreen (already exists)  
**Unblocks:** Runtime configuration of display/speed/hardware settings without recompiling

Move build-time compile settings to a runtime user-configurable device settings screen.
Frees up menu space by consolidating scattered toggles into one cohesive flow.

### Settings to migrate

| Setting | Screen Type | Current State | Purpose |
|---------|-------------|--------------|---------|
| Speed unit (MPH/KPH) | RadioSelect | `SPEED_SCALE_METRIC_UNITS` macro | Simulator subdisplay unit label and conversion direction |
| Maximum scale speed | TextInput | `SPEED_SCALE_AT_MAX_STEP` macro | DCC step 126 maps to this user-entered integer; we compute conversion factor |
| Heartbeat enabled | Toggle/Confirmation | Menu item | Connection watchdog on/off |
| Number of throttles | TextInput or spinner | Menu item `+ / -` buttons | 1–6 throttles |
| Drop before acquire | Toggle/Confirmation | Menu item | Auto-release existing consist before acquiring new locos |
| Remember locos | Toggle/Confirmation | Menu item | Restore acquired locos on next connection |
| Battery % display | Toggle/Confirmation | Menu item (if battery monitor enabled) | Show percentage vs. icon only |

### Wizard flow

1. **Speed unit** → RadioSelect: `MPH` / `KPH`  
   - Pre-populated from persistent `DeviceConfig`
2. **Maximum scale speed** → TextInput: integer (e.g., "100" for 100 MPH)  
   - Label shows unit from step 1: "Max speed MPH:" or "Max speed KPH:"
   - On confirm: compute factor = `enteredValue / 126.0`, store both value and derived factor
3. **Heartbeat** → RadioSelect: `On` / `Off`  
   - Pre-populated from `HeartbeatMonitor::enabled()`
4. **Number of throttles** → TextInput: integer 1–6  
   - Pre-populated from `MAX_THROTTLES`
5. **Drop before acquire** → RadioSelect: `Yes` / `No`  
   - Pre-populated from `LocoManager::dropBeforeAcquire()`
6. **Remember locos** → RadioSelect: `Yes` / `No`  
   - Pre-populated from `RESTORE_ACQUIRED_LOCOS` setting
7. **Battery % display** → RadioSelect: `Percentage` / `Icon only` (conditional on battery monitor enabled)  
   - Pre-populated from config

### Storage & initialization

- [~] New `DeviceConfig` struct in `ConfigStore.h` (parallel to `LocoConfig`, `ServerConfig`)
  - Struct exists with `speedStep`, `speedStepMultiplier`, `speedDisplayMode`,
    `encoderCwIsIncrease`, `heartbeatEnabled`, `restoreAcquiredLocos`,
    `numberOfThrottles`, `dropBeforeAcquire`
  - Still missing the fields below:
  - `speedMetricUnits: bool`
  - `maxScaleSpeed: int` (MPH/KPH value user entered)
  - `speedConversionFactor: float` (computed and stored for fast runtime access)
  - `batteryShowPercentage: bool`
  - [x] JSON load/save methods with fallback defaults (`loadDeviceConfig()` / `saveDeviceConfig()`)
- [x] Load the device config at startup and apply runtime values
- [x] Create a new `InputMode::DeviceSettings` handler
- [x] Wire `"Device Cfg"` into Extras
- [x] Apply heartbeat, throttle count, acquire mode, and remembered-loco settings live
- [ ] Update dependent systems:
  - [x] `HeartbeatMonitor::setEnabled()` 
  - [x] `ThrottleManager::setMaxThrottles()`
  - [x] `LocoManager::setDropBeforeAcquire()`
  - [x] `LocoManager::setRestoreAcquiredLocos()`
  - Battery display preference (if monitor exists)
  - speed conversion factor and unit flag

### Benefits

- Eliminates 5 separate menu items; consolidates to 1 "Device Settings" entry
- Users can adjust speed unit / conversion factor mid-session without recompiling
- Easier to test: no need for multiple firmware builds
- Matches user mental model: "settings" are grouped, not scattered

---

## 5. Renderer / Screen Decoupling

**Status:** Not started  
**Depends on:** Nothing (can be done incrementally)  
**Unblocks:** Adding new screen types without modifying Renderer

Every new screen type currently requires adding a bespoke `renderXxx()` method
to `Renderer.cpp`.  This violates Open/Closed — the Renderer must know about
every screen type and grows with each new one.

The render methods all share the same chrome boilerplate:

1. Clear buffer
2. Draw top bar separator + battery
3. **Fill the content area** ← only part that varies
4. Draw bottom bar separator + footer
5. Send buffer

### Options

**A) Content callback on IScreen.**  Extract the chrome into a
`renderChrome()` helper.  Add a `renderContent(DisplayDriver&, DisplayLayout&,
FontSet&, int contentTop, int contentBottom)` method to `IScreen`.  The
Renderer draws chrome, then calls the screen's content method.  New screen
types never touch Renderer.  Trade-off: screens gain pixel knowledge.

**B) Composable layout primitives.**  Screens describe content as a sequence of
layout blocks (`CenteredTextBlock`, `SelectableListBlock`, `InputFieldBlock`,
etc.).  Renderer knows how to draw each primitive; screens just compose them.
New screen types only touch Renderer if they need a genuinely new primitive.
Cleaner but more upfront work.

### First step (either option)

- [ ] Extract shared chrome (clear, top bar, battery, bottom bar, footer, sendBuffer) into a helper
- [ ] `renderRadioSelect` and `renderTextInput` already bypass `renderArrayInternal` — convert them to use the chrome helper as proof of concept

### Not urgent

Only 6 screen types exist today.  Address before the count doubles.

---

## 6. Footer Bar as Discrete Menu Items

**Status:** Not started  
**Depends on:** Nothing (can be done incrementally)  
**Unblocks:** Consistent, data-driven footer rendering across all screen types

Currently, footer text is a hard-coded string (e.g. `"# OK  * Cancel"`) baked
into each screen model.  This has several problems:

- Layout is guesswork — spacing depends on font width and screen size
- No way to grey out / hide a button contextually (e.g. hide "Cancel" on a
  mandatory wizard step)
- Inconsistent formatting across screens
- Harder to support soft-button hardware (3 buttons below the display)

### Goal

Replace the single `footerText` string with a small array of discrete footer
items, each with a label, key binding, and enabled/disabled state.  The
Renderer spaces them evenly across the footer row and can dim disabled items.

### Affected screens

- `RadioSelectScreen` — `"# OK  * Cancel"`
- `TextInputScreen` — `"# Submit  * Delete"`
- `ListSelectionScreen` — `footerTemplate` with `%p` / `%t` substitution
- `TitleScreen` / `WaitScreen` — various hard-coded footer strings

### Implementation sketch

```cpp
struct FooterItem {
    String label;      // e.g. "OK", "Cancel", "Pg 1/3"
    char   key;        // bound key ('*', '#', 0 for info-only)
    bool   enabled;    // false = draw dimmed / skip
};
```

The Renderer measures each label, distributes them with even spacing, and
draws bound keys in a distinct style (e.g. inverted box around the key char).

---

## 7. Future / Backlog

- [ ] Per-loco function presets (e.g. "Digitrax SD70" template that pre-fills function numbers)
- [ ] Config export/import (JSON download via serial or BLE)
- [ ] TFT-specific RadioSelectScreen rendering with colour highlights
- [ ] Sound profiles beyond diesel (steam chuff/cutoff, electric motor whine)
