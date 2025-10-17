#pragma once
#include "IModeHandler.h"
#include "../ThrottleManager.h"
#include "InputEvents.h"
#include "../../../actions.h"

// Handles non-locomotive programmable actions: power, sleep, throttle selection, battery toggle, etc.
class SystemActionHandler : public IModeHandler {
public:
    SystemActionHandler(ThrottleManager &throttle) : throttle_(throttle) {}
    bool handle(const InputEvent &ev) override {
        if (ev.type != InputEventType::Action) return false;
        switch (ev.ivalue) {
            case POWER_TOGGLE: { powerToggle(); return true; }
            case POWER_ON: { powerOnOff(PowerOn); return true; }
            case POWER_OFF: { powerOnOff(PowerOff); return true; }
            case SLEEP: { deepSleepStart(); return true; }
            case NEXT_THROTTLE: { throttle_.nextThrottle(); return true; }
            case MAX_THROTTLE_INCREASE: { throttle_.changeNumberOfThrottles(true); return true; }
            case MAX_THROTTLE_DECREASE: { throttle_.changeNumberOfThrottles(false); return true; }
            case SHOW_HIDE_BATTERY: { batteryShowToggle(); return true; }
            case THROTTLE_1: { throttle_.selectThrottle(0); return true; }
            case THROTTLE_2: { throttle_.selectThrottle(1); return true; }
            case THROTTLE_3: { throttle_.selectThrottle(2); return true; }
            case THROTTLE_4: { throttle_.selectThrottle(3); return true; }
            case THROTTLE_5: { throttle_.selectThrottle(4); return true; }
            case THROTTLE_6: { throttle_.selectThrottle(5); return true; }
            default:
                return false; // not handled
        }
    }
    void onEnter() override {}
    void onExit() override {}
private:
    ThrottleManager &throttle_;
    // External system functions (declared in WiTcontroller.h)
    void powerOnOff(TrackPower p);
    void powerToggle();
    void batteryShowToggle();
    void deepSleepStart();
};