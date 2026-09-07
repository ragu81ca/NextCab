// DeviceSettingsManager.h — Runtime settings for this handheld.
#pragma once

#include <Arduino.h>
#include "storage/ConfigStore.h"

class HeartbeatMonitor;
class LocoManager;
class Renderer;
class ThrottleManager;
class WiThrottleProtocol;

class DeviceSettingsManager {
public:
    void begin(ConfigStore *configStore, HeartbeatMonitor *heartbeat,
               WiThrottleProtocol *protocol, ThrottleManager *throttle,
               LocoManager *locoManager, Renderer *renderer);

    const DeviceConfig &config() const { return cfg_; }

    bool heartbeatEnabled() const { return cfg_.heartbeatEnabled; }
    void setHeartbeatEnabled(bool enabled);
    void toggleHeartbeatEnabled();

    uint8_t numberOfThrottles() const { return cfg_.numberOfThrottles; }
    void setNumberOfThrottles(uint8_t count);
    void increaseNumberOfThrottles();
    void decreaseNumberOfThrottles();

    bool dropBeforeAcquire() const { return cfg_.dropBeforeAcquire; }
    void setDropBeforeAcquire(bool enabled);
    void toggleDropBeforeAcquire();

    bool restoreAcquiredLocos() const { return cfg_.restoreAcquiredLocos; }
    void setRestoreAcquiredLocos(bool enabled);

    void save();

private:
    void apply();
    void clamp();

    ConfigStore        *configStore_ = nullptr;
    HeartbeatMonitor   *heartbeat_   = nullptr;
    WiThrottleProtocol *protocol_    = nullptr;
    ThrottleManager    *throttle_    = nullptr;
    LocoManager        *locoManager_ = nullptr;
    Renderer           *renderer_    = nullptr;

    DeviceConfig cfg_;
};
