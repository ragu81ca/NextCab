#pragma once
#include "IModeHandler.h"
#include "../ThrottleManager.h"
#include "../Renderer.h"
#include "../BatteryMonitor.h"
#include "InputEvents.h"
#include "../../../actions.h"
#include <WiThrottleProtocol.h>

// Handles non-locomotive programmable actions: power, sleep, throttle selection, battery toggle, etc.
class SystemActionHandler : public IModeHandler {
public:
    SystemActionHandler(ThrottleManager &throttle, Renderer &renderer,
                        BatteryMonitor &battery, WiThrottleProtocol &proto)
        : throttle_(throttle), renderer_(renderer), battery_(battery), proto_(proto) {}
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
    Renderer &renderer_;
    BatteryMonitor &battery_;
    WiThrottleProtocol &proto_;

    void powerOnOff(TrackPower p) {
        extern TrackPower trackPower;
        proto_.setTrackPower(p);
        trackPower = p;
        renderer_.renderSpeed();
    }
    void powerToggle() {
        extern TrackPower trackPower;
        powerOnOff(trackPower == PowerOn ? PowerOff : PowerOn);
    }
    void batteryShowToggle() {
        battery_.toggleDisplayMode();
        renderer_.renderSpeed();
    }
    // Deep sleep remains in .ino (hardware-specific GPIO/wakeup config)
    void deepSleepStart();
};