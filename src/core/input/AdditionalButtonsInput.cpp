#include "AdditionalButtonsInput.h"

AdditionalButtonsInput::AdditionalButtonsInput(DispatchFn fn) : dispatch(fn) {}

AdditionalButtonsInput::~AdditionalButtonsInput() {
    delete[] lastState; delete[] lastStableState; delete[] lastChangeTime; delete[] runtime;
}

void AdditionalButtonsInput::begin(const AdditionalButtonDef* defs, size_t count, unsigned long debounceMs) {
    definitions = defs; defCount = count; debounceDelay = debounceMs;
    lastState = new uint8_t[count];
    lastStableState = new uint8_t[count];
    lastChangeTime = new unsigned long[count];
    runtime = new BtnRuntime[count];
    for (size_t i=0;i<count;i++) {
        pinMode(defs[i].pin, defs[i].mode);
        uint8_t read = digitalRead(defs[i].pin);
        lastState[i] = read; lastStableState[i] = read; lastChangeTime[i] = 0;
        runtime[i].state = BtnState::Idle; runtime[i].lastRaw = read; runtime[i].ts = 0; runtime[i].latched=false;
    }
}

void AdditionalButtonsInput::poll() {
    unsigned long now = millis();
    for (size_t i=0;i<defCount;i++) {
        const auto &def = definitions[i];
        if (def.actionCode == FUNCTION_NULL) continue;
        bool isFn = (def.actionCode >= FUNCTION_0 && def.actionCode <= FUNCTION_31);
        bool isEmerg = (def.actionCode == E_STOP || def.actionCode == E_STOP_CURRENT_LOCO);
        uint8_t raw = digitalRead(def.pin);
        BtnRuntime &rt = runtime[i];
        bool logical = isPressed(i, raw);
        if (raw != rt.lastRaw) {
            bool wasPressed = isPressed(i, rt.lastRaw);
            rt.lastRaw = raw; rt.ts = now; AB_DBG_PRINTF("[SM][edge] i=%u raw=%u t=%lu\n", (unsigned)i,(unsigned)raw,now);
            if (isEmerg) {
                if (logical) { emitActionImmediate(def.actionCode, now); rt.state = BtnState::PressNoise; continue; }
                else if (wasPressed) { rt.state = BtnState::Idle; continue; }
            }
        }
        if (isEmerg) continue; // emergency handled solely on edges
        switch (rt.state) {
            case BtnState::Idle:
                if (logical) { rt.state = BtnState::PressNoise; rt.ts = now; }
                break;
            case BtnState::PressNoise:
                if (!logical) {
                    unsigned long held = now - rt.ts;
                    if (isFn && def.toggle && held >= AB_TOGGLE_FAST_MIN_MS) {
                        rt.latched = !rt.latched; emitFunction(idxToButtonIndex(i), rt.latched, now);
                        AB_DBG_PRINTF("[SM][fast-toggle] i=%u held=%lu latched=%u\n", (unsigned)i, held, (unsigned)rt.latched);
                    }
                    rt.state = BtnState::Idle; break;
                }
                if ((now - rt.ts) >= debounceDelay) {
                    if (isFn && def.toggle) { rt.latched = !rt.latched; emitFunction(idxToButtonIndex(i), rt.latched, now); rt.state = BtnState::LatchedReleaseWait; }
                    else if (isFn && !def.toggle) { emitFunction(idxToButtonIndex(i), true, now); rt.state = BtnState::Active; }
                    else { emitActionImmediate(def.actionCode, now); rt.state = BtnState::Active; }
                }
                break;
            case BtnState::Active:
                if (!logical) { rt.state = BtnState::ReleaseNoise; rt.ts = now; }
                break;
            case BtnState::ReleaseNoise:
                if (logical) { rt.state = BtnState::Active; break; }
                if ((now - rt.ts) >= debounceDelay) {
                    if (isFn && !def.toggle) emitFunction(idxToButtonIndex(i), false, now);
                    rt.state = BtnState::Idle;
                }
                break;
            case BtnState::LatchedReleaseWait:
                if (!logical) { rt.state = BtnState::ReleaseNoise; rt.ts = now; }
                else if (logical && (now - rt.ts) > 1500UL) { /* hold no-op */ }
                if (rt.state == BtnState::ReleaseNoise && (now - rt.ts) >= debounceDelay && !logical) { rt.state = BtnState::Idle; }
                break;
        }
    }
}

bool AdditionalButtonsInput::isPressed(size_t idx, uint8_t raw) const {
    return (definitions[idx].mode == INPUT_PULLUP) ? (raw == LOW) : (raw == HIGH);
}
