// DeviceSettingsManager.cpp — Runtime device preference ownership.
#include "DeviceSettingsManager.h"
#include "heartbeat/HeartbeatMonitor.h"
#include "LocoManager.h"
#include "Renderer.h"
#include "ThrottleManager.h"
#include <WiThrottleProtocol.h>

void DeviceSettingsManager::begin(ConfigStore *configStore, HeartbeatMonitor *heartbeat,
                                  WiThrottleProtocol *protocol, ThrottleManager *throttle,
                                  LocoManager *locoManager, Renderer *renderer) {
    configStore_ = configStore;
    heartbeat_   = heartbeat;
    protocol_    = protocol;
    throttle_    = throttle;
    locoManager_ = locoManager;
    renderer_    = renderer;

    cfg_ = configStore_ ? configStore_->loadDeviceConfig() : DeviceConfig();
    clamp();
    apply();
}

void DeviceSettingsManager::clamp() {
    if (cfg_.numberOfThrottles < 1) cfg_.numberOfThrottles = 1;
    if (cfg_.numberOfThrottles > WIT_MAX_THROTTLES) cfg_.numberOfThrottles = WIT_MAX_THROTTLES;
}

void DeviceSettingsManager::apply() {
    if (heartbeat_) heartbeat_->setEnabled(cfg_.heartbeatEnabled);
    if (protocol_) protocol_->requireHeartbeat(cfg_.heartbeatEnabled);
    if (throttle_) throttle_->setMaxThrottles(cfg_.numberOfThrottles);
    if (locoManager_) {
        locoManager_->setDropBeforeAcquire(cfg_.dropBeforeAcquire);
        locoManager_->setRestoreAcquiredLocos(cfg_.restoreAcquiredLocos);
    }
}

void DeviceSettingsManager::save() {
    clamp();
    if (configStore_) configStore_->saveDeviceConfig(cfg_);
}

void DeviceSettingsManager::setHeartbeatEnabled(bool enabled) {
    cfg_.heartbeatEnabled = enabled;
    apply();
    save();
    if (renderer_) renderer_->renderHeartbeatCheck();
}

void DeviceSettingsManager::toggleHeartbeatEnabled() {
    setHeartbeatEnabled(!cfg_.heartbeatEnabled);
}

void DeviceSettingsManager::setNumberOfThrottles(uint8_t count) {
    cfg_.numberOfThrottles = count;
    clamp();
    apply();
    save();
    if (renderer_) renderer_->renderSpeed();
}

void DeviceSettingsManager::increaseNumberOfThrottles() {
    setNumberOfThrottles(cfg_.numberOfThrottles + 1);
}

void DeviceSettingsManager::decreaseNumberOfThrottles() {
    setNumberOfThrottles(cfg_.numberOfThrottles > 1 ? cfg_.numberOfThrottles - 1 : 1);
}

void DeviceSettingsManager::setDropBeforeAcquire(bool enabled) {
    cfg_.dropBeforeAcquire = enabled;
    apply();
    save();
}

void DeviceSettingsManager::toggleDropBeforeAcquire() {
    setDropBeforeAcquire(!cfg_.dropBeforeAcquire);
}

void DeviceSettingsManager::setRestoreAcquiredLocos(bool enabled) {
    cfg_.restoreAcquiredLocos = enabled;
    apply();
    save();
}
