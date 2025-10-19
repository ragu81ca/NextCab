#pragma once
#include <Arduino.h>
#include <functional>
#include "InputEvents.h"
#include "IInputDevice.h"
#include "../../../actions.h" // for FUNCTION_0..FUNCTION_31 range constants

#ifndef ADDITIONAL_BUTTONS_DEBUG
#define ADDITIONAL_BUTTONS_DEBUG 0
#endif
#ifndef AB_TOGGLE_FAST_MIN_MS
#define AB_TOGGLE_FAST_MIN_MS 5   // minimum held time to accept an ultra-short tap for toggle
#endif
#if ADDITIONAL_BUTTONS_DEBUG
#define AB_DBG_PRINT(x)   Serial.print(x)
#define AB_DBG_PRINTLN(x) Serial.println(x)
#define AB_DBG_PRINTF(...) Serial.printf(__VA_ARGS__)
#else
#define AB_DBG_PRINT(x)
#define AB_DBG_PRINTLN(x)
#define AB_DBG_PRINTF(...)
#endif

struct AdditionalButtonDef {
    uint8_t pin;              // GPIO pin
    uint8_t mode;             // INPUT or INPUT_PULLUP
    int actionCode;           // existing action code (maps to DomainAction or function)
    bool toggle;              // treat as toggle (dispatch press only; release ignored)
    bool oneShot;             // fire only on first press until released
};

class AdditionalButtonsInput : public IInputDevice {
public:
    using DispatchFn = std::function<void(const InputEvent&)>;
    explicit AdditionalButtonsInput(DispatchFn fn);
    ~AdditionalButtonsInput();
    AdditionalButtonsInput(const AdditionalButtonsInput&) = delete;
    AdditionalButtonsInput& operator=(const AdditionalButtonsInput&) = delete;

    void begin(const AdditionalButtonDef* defs, size_t count, unsigned long debounceMs=40);
    void poll() override; // renamed from loop
    void begin() override { /* requires external defs begin call; keep empty */ }
    const char* name() const override { return "AdditionalButtons"; }

private:
    DispatchFn dispatch;
    const AdditionalButtonDef* definitions { nullptr };
    size_t defCount { 0 };
    unsigned long debounceDelay { 40 };
    uint8_t* lastState { nullptr };            // instantaneous reading
    uint8_t* lastStableState { nullptr };      // debounced state
    unsigned long* lastChangeTime { nullptr }; // time of last instantaneous change

    // State machine data
    enum class BtnState : uint8_t { Idle=0, PressNoise, Active, ReleaseNoise, LatchedReleaseWait };
    struct BtnRuntime { BtnState state; bool lastRaw; unsigned long ts; bool latched; };
    BtnRuntime* runtime { nullptr };


    inline void emitFunction(int buttonIndex, bool on, unsigned long now) {
        InputEvent ev; ev.timestamp = now; ev.type = InputEventType::AdditionalButton; ev.ivalue = buttonIndex; ev.cvalue = on ? 'P' : 'R';
        AB_DBG_PRINTF("[SM][fn] bIdx=%d state=%c t=%lu\n", buttonIndex, on?'P':'R', now);
        dispatch(ev);
    }
    inline void emitActionImmediate(int actionCode, unsigned long now) {
        InputEvent ev; ev.timestamp = now; ev.type = InputEventType::Action; ev.ivalue = actionCode; ev.cvalue = 0;
        AB_DBG_PRINTF("[SM][act] code=%d t=%lu\n", actionCode, now);
        dispatch(ev);
    }
    inline int idxToButtonIndex(size_t i) { return (int)i; }

    bool isPressed(size_t idx, uint8_t raw) const; // implemented in .cpp

};
