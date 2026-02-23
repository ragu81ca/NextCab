# WiTcontroller Refactor Architecture (Phase 1)

This document describes the emerging modular architecture introduced to improve extensibility, testability and maintainability while preserving existing behaviour.

## Goals

1. Reduce the 3.7K line monolithic `WiTcontroller.ino` into focused modules.
2. Introduce clear ownership boundaries (single responsibility per module).
3. Enable unit testing of logic isolated from hardware.
4. Provide stable facades so incremental refactors do not break sketches / user configs.
5. Prepare for future extension points (plugins, alternative inputs, displays, transports).

## High-Level Module Map

Planned (✔ = started):

| Layer | Module | Responsibility | Status |
|-------|--------|----------------|--------|
| Core  | `CoreController` | Application lifecycle & orchestration | Pending |
| Core  | `PreferencesManager` | NVS persistence of consists & settings | ✔ Implemented |
| Core  | `StartupCommands` | Parsing & executing startup macro sequence | Pending |
| Domain | `ThrottleManager` | Throttle state (speeds, directions, functions) & consist ops | ✔ Implemented (Phase 1) |
| Domain | `RosterManager` | Roster, turnouts, routes lists & sorting | Pending |
| Hardware | `WifiManager` | SSID discovery/selection/connection | Pending |
| Hardware | `ServerDiscovery` | mDNS WiThrottle service enumeration & manual entry | Pending |
| Hardware | `BatteryMonitor` | Periodic battery sampling, sleep threshold | ✔ Implemented |
| Hardware | `Input::KeypadHandler` | Keypad event translation | Pending |
| Hardware | `Input::EncoderHandler` | Rotary encoder + button semantics | Pending |
| Hardware | `Input::AdditionalButtons` | Optional external buttons | Pending |
| Protocol | `WiThrottleClient` | Thin wrapper around WiThrottleProtocol delegate & callbacks | Pending |
| UI | `Renderer` | All drawing logic (currently many `writeOled*` funcs) | Pending |
| UI | `MenuSystem` | Menu state machine & command parsing | Pending |
| Utils | `ActionDispatcher` | Mapping high-level actions (enum) -> operations | Pending |
| Utils | `EventBus` | Lightweight pub/sub for decoupling (optional future) | Planned |

## Extraction Strategy

1. Identify cohesive function clusters (e.g. battery test) and move into a class with a small public API.
2. Replace global variables with class members; provide transitional access via existing functions or thin wrappers to avoid large call-site churn.
3. Keep `.ino` as a façade: create objects, wire dependencies, delegate to them in `setup()` / `loop()`.
4. After each extraction: compile & (when feasible) add a unit test in `test/`.
5. Only once major subsystems are modular: introduce an event bus or interface abstractions (e.g. alternative display implementation).

## BatteryMonitor Example (Completed)

Old global state: `useBatteryTest`, `showBatteryTest`, `lastBatteryTestValue`, etc. + inline logic in `batteryTest_loop()` & `writeOledBattery()`.

Refactor:
* New files: `core/BatteryMonitor.h/.cpp`.
* Public API: `begin()`, `loop()`, `percent()`, `toggleDisplayMode()`, `shouldSleepForLowBattery()`.
* UI layer (`writeOledBattery`) now queries the monitor instead of raw globals.
* Sleep decision moved into monitor helper to centralize policy.

## Upcoming Refactors (Revised Order – Post Throttle & Preferences)

1. WiThrottleClient: wrap protocol delegate, surface high-level events (acquire/release/speed/direction/function changes).
2. MenuSystem + ActionDispatcher: centralize command interpretation & reduce conditional complexity in the root loop.
3. Renderer: unify drawing operations; introduce a small render model (struct of current view state) to minimize flicker & repeated calculations.
4. WifiManager + ServerDiscovery separation: distinct responsibilities for network vs service discovery; enable retry/backoff policies.
5. Input Handlers (Keypad/Encoder/Buttons): emit semantic Actions to dispatcher (decouple physical inputs from operations).
6. RosterManager: encapsulate roster/turnout/route collections, sorting and filtering.
7. CoreController: orchestrate setup/loop and mediate between managers (final pass to slim `.ino`).
8. Optional: EventBus introduction once at least three modules would benefit from decoupled notifications (likely after MenuSystem & Renderer extraction).

## ThrottleManager Summary (Phase 1 Completed)

Scope moved:
* Speed set / up / down / display conversion (percent, 0–28, raw).
* Direction change & toggle across single or multi-loco consists (preserving orientation logic).
* Throttle index cycling & selection, max throttle count changes, speed step multiplier cycling, emergency stops.

Still in root (future phases):
* Function (F0–F28 etc.) handling & labeling – candidate for a future FunctionManager or extension of ThrottleManager.
* Consist assembly & release orchestration (steal/release multi-loco operations) – may shift to a ConsistManager.

Planned refinements:
* Remove remaining thin wrappers once external references updated.
* Add unit tests for boundary conditions (speed clamp, direction inversion with mixed consist orientations, multiplier cycling sequence).

## Coding Conventions (New Modules)

* Header guards via `#pragma once`.
* Use `CamelCase` for class names, `camelCase` for methods.
* Prefer `const` correctness & `explicit` constructors (future cleanups).
* Limit Arduino dependencies to boundary layers (Hardware & UI) to ease future portability.
* Avoid new global variables; prefer dependency injection via constructors.

## Transitional Guidelines

During Phase 1 many legacy globals remain for compatibility. Each extraction should either:
* Remove a group of globals entirely, OR
* Mark remaining ones with a `// TODO modularize` comment.

## Testing Plan

* Use PlatformIO `test/` with Unity framework (bundled) for pure logic modules.
* Provide mocks/stubs for protocol & hardware where needed.

## Future Extension Points

* Event Bus (publish e.g. `SpeedChanged`, `FunctionToggled`, `BatteryLow`).
* Plugin interface allowing new input hardware or display types without editing core.
* Config loader (JSON in SPIFFS) to replace compile-time defines for runtime flexibility.
* CI workflow (GitHub Actions) building multiple board environments.

---
Feedback & contributions welcome. Each subsequent module extraction should update this document.
