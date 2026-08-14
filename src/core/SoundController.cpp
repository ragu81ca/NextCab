// SoundController.cpp - Event-driven diesel locomotive sound management
#include "SoundController.h"
#include "ThrottleManager.h"
#include "LocoManager.h"
#include <WiThrottleProtocol.h>
#include "../../WiTcontroller.h"  // getMultiThrottleChar

SoundController::SoundController() : throttleMgr_(nullptr), proto_(nullptr), locoMgr_(nullptr) {
    for (int i = 0; i < WIT_MAX_THROTTLES; i++) {
        for (int r = 0; r < ROLE_COUNT; r++) {
            rolePulseActive_[i][r] = false;
            rolePulseStart_[i][r] = 0;
            lastRoleTime_[i][r] = 0;
        }
        currentNotch_[i] = 1;     // Start at notch 1 (idle)
        targetNotch_[i] = 1;      // Start at notch 1 (idle)
        lastNotchTime_[i] = 0;
        idleFlushRemaining_[i] = 0;
        dynamicBraking_[i] = false;
    }
}

void SoundController::begin(ThrottleManager* throttleMgr, WiThrottleProtocol* proto, LocoManager* locoMgr) {
    throttleMgr_ = throttleMgr;
    proto_ = proto;
    locoMgr_ = locoMgr;
    Serial.println("[SoundController] Initialized - Diesel locomotive sound simulation");
}

void SoundController::update() {
    if (!config_.enabled || !throttleMgr_) return;
    
    unsigned long now = millis();
    
    // Handle non-blocking function pulses for all throttles and roles
    for (int throttle = 0; throttle < WIT_MAX_THROTTLES; throttle++) {
        for (int r = 0; r < ROLE_COUNT; r++) {
            if (rolePulseActive_[throttle][r]) {
                if (now - rolePulseStart_[throttle][r] >= config_.pulseduration) {
                    // Time to turn OFF the function
                    SoundRole role = static_cast<SoundRole>(r);
                    turnOffFunction(throttle, role);
                    
                    rolePulseActive_[throttle][r] = false;
                }
            }
        }
        
        // Update notch sounds for this throttle
        updateNotchSounds(throttle, now);
    }
}

void SoundController::onBrakeStateChange(int throttle, bool braking) {
    if (!config_.enabled) return;
    if (throttle < 0 || throttle >= WIT_MAX_THROTTLES) return;
    
    Serial.print("[SoundController] T");
    Serial.print(throttle);
    Serial.print(" Brake: ");
    Serial.println(braking ? "APPLIED" : "RELEASED");
    
    // Send brake function to each sound-enabled loco using its configured function
    if (locoMgr_ && locoMgr_->hasSoundThrottle(throttle) && proto_) {
        triggerBrakeSound(throttle, braking);
    }
}

void SoundController::onDynamicBrakeStateChange(int throttle, bool active) {
    if (!config_.enabled) return;
    if (throttle < 0 || throttle >= WIT_MAX_THROTTLES) return;
    
    Serial.print("[SoundController] T");
    Serial.print(throttle);
    Serial.print(" Dynamic Brake: ");
    Serial.println(active ? "ENGAGED" : "RELEASED");
    
    dynamicBraking_[throttle] = active;
    
    if (active) {
        // Engineer notches down to idle before applying dynamic brakes
        targetNotch_[throttle] = 1;
    }
    // When released, the next onPowerLevelChange or onSpeedChange will
    // set the correct notch — no need to recalculate here.
    
    triggerDynamicBrakeSound(throttle, active);
}

void SoundController::triggerBrakeSound(int throttle, bool state) {
    if (!locoMgr_ || !proto_) return;
    char tChar = getMultiThrottleChar(throttle);
    for (const auto& cfg : locoMgr_->soundConfigs(throttle)) {
        if (cfg.funcBrake < 0) continue;
        proto_->setFunction(tChar, cfg.address, cfg.funcBrake, state, true);
        debugPrint(throttle, cfg.funcBrake, state ? "ON" : "OFF", "brake");
    }
}

void SoundController::triggerDynamicBrakeSound(int throttle, bool state) {
    if (!locoMgr_ || !proto_) return;
    char tChar = getMultiThrottleChar(throttle);
    for (const auto& cfg : locoMgr_->soundConfigs(throttle)) {
        if (cfg.funcDynamicBrake < 0) continue;
        proto_->setFunction(tChar, cfg.address, cfg.funcDynamicBrake, state, true);
        debugPrint(throttle, cfg.funcDynamicBrake, state ? "ON" : "OFF", "dynamic brake");
    }
}

void SoundController::onSpeedChange(int throttle, int oldSpeed, int newSpeed) {
    if (!config_.enabled) return;
    if (throttle < 0 || throttle >= WIT_MAX_THROTTLES) return;
    
    // Enable idle flush when stopping - sends extra F7 pulses for decoder reliability
    if (newSpeed == 0 && oldSpeed > 0) {
        idleFlushRemaining_[throttle] = IDLE_FLUSH_COUNT;
    }
    
    // Direct speed→notch mapping (non-power-mode path)
    int newTargetNotch = dynamicBraking_[throttle] ? 1 : calculateNotchFromSpeed(newSpeed);
    if (newTargetNotch != targetNotch_[throttle]) {
        Serial.print("[SoundController] T");
        Serial.print(throttle);
        Serial.print(" Speed notch: ");
        Serial.print(targetNotch_[throttle]);
        Serial.print(" -> ");
        Serial.print(newTargetNotch);
        Serial.print(" (speed=");
        Serial.print(newSpeed);
        Serial.println(")");
        targetNotch_[throttle] = newTargetNotch;
    }
}

void SoundController::onDirectionChange(int throttle) {
    if (!config_.enabled) return;
    
    // This could trigger horn/bell sounds or other direction change sounds
    // For now, just log the event
    Serial.print("[SoundController] T");
    Serial.print(throttle);
    Serial.println(" Direction changed");
}

void SoundController::triggerFunction(int throttle, SoundRole role, const char* reason) {
    if (!canTriggerRole(throttle, role)) return;
    if (!locoMgr_ || !locoMgr_->hasSoundThrottle(throttle) || !proto_) return;
    
    unsigned long now = millis();
    int ri = static_cast<int>(role);
    
    char tChar = getMultiThrottleChar(throttle);
    for (const auto& cfg : locoMgr_->soundConfigs(throttle)) {
        int locoFunc = (role == SoundRole::ThrottleUp) ? cfg.funcThrottleUp : cfg.funcThrottleDown;
        if (locoFunc < 0) continue;
        proto_->setFunction(tChar, cfg.address, locoFunc, true, true);
        debugPrint(throttle, locoFunc, "ON", reason);
    }
    
    // Track pulse state for automatic OFF timing
    rolePulseActive_[throttle][ri] = true;
    rolePulseStart_[throttle][ri] = now;
    lastRoleTime_[throttle][ri] = now;
}

void SoundController::turnOffFunction(int throttle, SoundRole role) {
    if (!locoMgr_ || !locoMgr_->hasSoundThrottle(throttle) || !proto_) return;
    
    char tChar = getMultiThrottleChar(throttle);
    for (const auto& cfg : locoMgr_->soundConfigs(throttle)) {
        int locoFunc = (role == SoundRole::ThrottleUp) ? cfg.funcThrottleUp : cfg.funcThrottleDown;
        if (locoFunc < 0) continue;
        proto_->setFunction(tChar, cfg.address, locoFunc, false, true);
    }
}

bool SoundController::canTriggerRole(int throttle, SoundRole role) {
    if (throttle < 0 || throttle >= WIT_MAX_THROTTLES) return false;
    if (!throttleMgr_) return false;
    
    int ri = static_cast<int>(role);
    unsigned long now = millis();
    
    if (rolePulseActive_[throttle][ri]) return false;
    if (now - lastRoleTime_[throttle][ri] < config_.cooldownPeriod) return false;
    
    return true;
}

void SoundController::debugPrint(int throttle, uint8_t funcNum, const char* action, const char* reason) {
    Serial.print("[SoundController] T");
    Serial.print(throttle);
    Serial.print(" F");
    Serial.print(funcNum);
    Serial.print(" ");
    Serial.print(action);
    if (reason && strlen(reason) > 0) {
        Serial.print(" (");
        Serial.print(reason);
        Serial.print(")");
    }
    Serial.println();
}

// ============================================================================
// Internal Notch Simulation (for diesel locomotive sound effects only)
// ============================================================================

// Convert speed (0-126) to notch (1-8) for diesel locomotive sound simulation
// Diesel locomotives have 8 throttle notches representing engine power settings
// Notch 1 = idle (0-15), Notches 2-8 = increasing power
// This is the "cruising" notch - the steady-state power to maintain a given speed
int SoundController::calculateNotchFromSpeed(int speed) const {
    if (speed >= 112) return 8;  // Notch 8: 112-126
    if (speed >= 96) return 7;   // Notch 7: 96-111
    if (speed >= 80) return 6;   // Notch 6: 80-95
    if (speed >= 64) return 5;   // Notch 5: 64-79
    if (speed >= 48) return 4;   // Notch 4: 48-63
    if (speed >= 32) return 3;   // Notch 3: 32-47
    if (speed >= 16) return 2;   // Notch 2: 16-31
    return 1;                    // Notch 1: 0-15 (idle)
}

void SoundController::updateNotchSounds(int throttle, unsigned long now) {
    if (throttle < 0 || throttle >= WIT_MAX_THROTTLES) return;
    
    // Diesel locomotive notch simulation: move gradually between engine power settings
    // Check if we need to change notch position (with rate limiting)
    if (currentNotch_[throttle] != targetNotch_[throttle] && 
        (now - lastNotchTime_[throttle] >= NOTCH_TRANSITION_MS)) {
        
        // Debug: Show notch transition attempt
        Serial.print("[SoundController] T");
        Serial.print(throttle);
        Serial.print(" Notch transition: ");
        Serial.print(currentNotch_[throttle]);
        Serial.print(" -> ");
        Serial.print(targetNotch_[throttle]);
        
        // Store old notch for logging
        int oldNotch = currentNotch_[throttle];
        
        // Move one notch at a time towards target
        if (currentNotch_[throttle] < targetNotch_[throttle]) {
            // Notching up
            currentNotch_[throttle]++;
            lastNotchTime_[throttle] = now;
            triggerFunction(throttle, SoundRole::ThrottleUp, "notch up");
            
        } else if (currentNotch_[throttle] > targetNotch_[throttle]) {
            // Notching down
            currentNotch_[throttle]--;
            lastNotchTime_[throttle] = now;
            triggerFunction(throttle, SoundRole::ThrottleDown, "notch down");
        }
        
        Serial.print(" (");
        Serial.print(oldNotch);
        Serial.print(" -> ");
        Serial.print(currentNotch_[throttle]);
        Serial.println(")");
    }
    
    // Idle recovery: send extra throttle-down pulses after normal sequence completes
    // This ensures Digitrax and other decoders that may have missed a packet
    // still reach idle. Extra F7 pulses at idle are harmless (decoder ignores them).
    if (currentNotch_[throttle] == 1 && targetNotch_[throttle] == 1 &&
        idleFlushRemaining_[throttle] > 0 &&
        (now - lastNotchTime_[throttle] >= NOTCH_TRANSITION_MS)) {
        
        triggerFunction(throttle, SoundRole::ThrottleDown, "idle flush");
        idleFlushRemaining_[throttle]--;
        lastNotchTime_[throttle] = now;
        
        Serial.print("[SoundController] T");
        Serial.print(throttle);
        Serial.print(" Idle flush pulse (");
        Serial.print(idleFlushRemaining_[throttle]);
        Serial.println(" remaining)");
    }
}

// ============================================================================
// Notching State Query (used by MomentumController for "sound leads movement")
// ============================================================================

bool SoundController::isNotching(int throttle) const {
    if (!config_.enabled) return false;
    if (throttle < 0 || throttle >= WIT_MAX_THROTTLES) return false;
    
    // We're notching if current notch differs from target
    return currentNotch_[throttle] != targetNotch_[throttle];
}

bool SoundController::isAnyNotching() const {
    if (!config_.enabled) return false;
    
    for (int i = 0; i < WIT_MAX_THROTTLES; i++) {
        if (currentNotch_[i] != targetNotch_[i]) {
            return true;
        }
    }
    return false;
}