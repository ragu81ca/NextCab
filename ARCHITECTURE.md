# WiTcontroller Architecture Overview

This document captures the evolving internal structure after recent refactors (heartbeat simplification and unified additional button input). It is aimed at contributors; end‑user setup remains in `README.md`.

---
## High-Level Flow

Hardware Inputs  ->  Producers  ->  InputManager  ->  Mode Handler (Operation / Password)  ->  Fallback (SystemActionHandler)  ->  Domain (ThrottleManager, WiThrottleProtocol)

### Producers
- Keypad: Translates key scan states into press/release events, mapped to either Function actions (F0..F31) or generic Action codes.
- Rotary Encoder & Throttle Pot: Abstracted by `ThrottleInputManager` into SpeedDelta / SpeedAbsolute events.
- Additional Buttons: Unified in `AdditionalButtonsInput` with debouncing and immediate edge handling for critical/emergency or responsive function actions.

### Input Events (`InputEvents.h`)
- `SpeedDelta`, `SpeedAbsolute`, `EncoderClick`, `EncoderLongPress`
- `DirectionToggle`
- `Action` (non-function discrete actions: speed up/down, direction, emergency stops, track power toggle, etc.)
- `AdditionalButton` (function button press/release originating from optional hardware buttons; preserves legacy latching semantics)

### InputManager
1. For the current mode, calls the active handler's `handle()`; if consumed, stops.
2. Always ensures emergency actions (E_STOP, E_STOP_CURRENT_LOCO) are executed even outside Operation mode.
3. Processes `AdditionalButton` events directly: function codes invoke `doDirectFunction`, non-function actions converted to `Action` press-only events. Legacy latch handler removed.
4. Unhandled `Action` events flow to `SystemActionHandler`.

### Mode Handlers
- `OperationModeHandler`: Loco-centric speed/direction/emergency and speed-step multiplier changes.
- `PasswordEntryModeHandler`: Captures keypad digits and commit/cancel events until password entry finishes.
- `SystemActionHandler`: Non-loco global/system actions (track power toggle, UI mode toggles, etc.).

---
## Additional Buttons State Machine

### Motivation
Legacy debounce + immediate press path used a `pulseEmitted` guard and downstream latch logic, producing complexity and occasional duplicate semantics. A unified per-button state machine ensures deterministic single semantic events.

### States
- Idle
- PressNoise (pre-debounce press candidate)
- Active (momentary function/action held)
- ReleaseNoise (pre-debounce release candidate)
- LatchedReleaseWait (for toggle functions; waits only for release to return to Idle)

### Emission Rules
- Toggle function: single Press event per physical press (fast tap supported via minimum hold threshold) — no Release event.
- Momentary function: Press event after debounce (or immediate if optimized), Release event after stable release.
- Emergency (E_STOP / current loco): Action event immediately on raw press edge (one-shot), no release.
- Non-function actions: single Action event on press only.

### Advantages
- Eliminates double-send (press + post-debounce press) race.
- Centralizes latching; legacy `doDirectAdditionalButtonCommand` removed.
- Simplifies InputManager mapping logic (direct function invocation for functions; action dispatch for others).

### Fast Toggle Tap
Ultra-short taps (< debounce window) still produce a toggle via a minimal hold threshold (`AB_TOGGLE_FAST_MIN_MS`).

---
## Heartbeat Simplification
- Heartbeat monitor holds: enabled flag, period, last send timestamp.
- Server exclusively decides timeout; client no longer tracks last server response for disconnect logic.
- Cleaner separation: transport reliability & timeout logic delegated to server side.

---
## Action Code Taxonomy (Excerpt)
- Functions: `FUNCTION_0` .. `FUNCTION_31` (handled via latching logic when coming from additional buttons / keypad)
- Speed: `SPEED_UP`, `SPEED_DOWN`, `SPEED_UP_FAST`, `SPEED_DOWN_FAST`, `SPEED_STOP`
- Direction: `DIRECTION_TOGGLE`, `DIRECTION_FORWARD`, `DIRECTION_REVERSE`
- Emergency: `E_STOP`, `E_STOP_CURRENT_LOCO`
- System / Misc: e.g. `POWER_TOGGLE`, `SPEED_MULTIPLIER`, higher 500+ range for special toggles

---
## Extending Inputs
1. Implement a producer that periodically polls or ISR-captures state and emits `InputEvent` objects.
2. For new button-like hardware producing function codes, emit `AdditionalButton` events to leverage existing latching.
3. For discrete non-function actions, emit `Action` press-only events.
4. Preserve immediate emission only for cases with compelling latency or pulse-width concerns.

---
## Debounce Guidelines
- Default: 40 ms window used in state machine for stable transitions.
- Emergency actions bypass debounce (edge-driven immediate).
- Fast toggle tap path evaluates hold duration (< debounce) to accept valid toggles.
- Adjust to 50–60 ms for noisy hardware; can reduce to ~25–30 ms if clean switches—verify no false edges.

---
## Future Improvements (Backlog Candidates)
- Optional test harness (simulate pin edges + millis()).
- Per-button configuration for debounce & fast-tap threshold.
- Telemetry hook to log latency from edge to dispatch.

---
## Quick Reference: Event Lifecycle (Function Button)
1. Physical press detected (edge): immediate `AdditionalButton` press event dispatched; `pulseEmitted=true`.
2. Any bouncing during debounce ignored.
3. Stable release after debounce: `AdditionalButton` release event (if not toggle/oneShot) -> latch logic toggles off or finalizes state; `pulseEmitted=false`.

---
## Contributing
When modifying input behavior:
- Ensure single semantic event per physical intent (avoid double press artifacts).
- Keep emergency pathways constant-time and independent of UI mode.
- Update this document when altering flow guarantees.

---
*Last updated: 2025-10-18*
