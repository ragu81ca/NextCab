// SoundController.cpp - Event-driven diesel locomotive sound management
#include "SoundController.h"
#include "ThrottleManager.h"
#include "LocoManager.h"
#include <WiThrottleProtocol.h>
#include "../../WiTcontroller.h"  // getMultiThrottleChar

SoundController::SoundController() : throttleMgr_(nullptr), proto_(nullptr), locoMgr_(nullptr) {
    for (int i = 0; i < WIT_MAX_THROTTLES; i++) {
        for (int f = 0; f < 32; f++) {
            functionPulseActive_[i][f] = false;
            functionPulseStart_[i][f] = 0;
            lastFunctionTime_[i][f] = 0;
        }
        currentNotch_[i] = 1;     // Start at notch 1 (idle)
        targetNotch_[i] = 1;      // Start at notch 1 (idle)
        lastNotchTime_[i] = 0;
        targetSpeed_[i] = 0;
        actualSpeed_[i] = 0;
        idleFlushRemaining_[i] = 0;
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
    
    // Handle non-blocking function pulses for all throttles and functions
    for (int throttle = 0; throttle < WIT_MAX_THROTTLES; throttle++) {
        for (int funcNum = 0; funcNum < 32; funcNum++) {
            if (functionPulseActive_[throttle][funcNum]) {
                if (now - functionPulseStart_[throttle][funcNum] >= config_.pulseduration) {
                    // Time to turn OFF the function
                    turnOffFunction(throttle, funcNum);
                    debugPrint(throttle, funcNum, "OFF", "diesel sound pulse complete");
                    
                    functionPulseActive_[throttle][funcNum] = false;
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
    
    // Send brake function to each sound-enabled loco using its configured function,
    // or fall back to global default on lead loco
    if (locoMgr_ && locoMgr_->hasSoundThrottle(throttle) && proto_) {
        char tChar = getMultiThrottleChar(throttle);
        for (const auto& cfg : locoMgr_->soundConfigs(throttle)) {
            int func = (cfg.funcBrake >= 0) ? cfg.funcBrake : config_.brakeFunction;
            if (func > 0) {
                proto_->setFunction(tChar, cfg.address, func, braking, true);
                debugPrint(throttle, func, braking ? "ON" : "OFF", "per-loco brake");
            }
        }
    } else if (config_.brakeFunction != 0) {
        // Fallback: legacy single-loco behavior
        triggerFunctionImmediate(throttle, config_.brakeFunction, braking);
    }
}

void SoundController::onSpeedChange(int throttle, int oldSpeed, int newSpeed) {
    if (!config_.enabled) return;
    if (throttle < 0 || throttle >= WIT_MAX_THROTTLES) return;
    
    targetSpeed_[throttle] = newSpeed;
    
    // Enable idle flush when stopping - sends extra F7 pulses for decoder reliability
    if (newSpeed == 0 && oldSpeed > 0) {
        idleFlushRemaining_[throttle] = IDLE_FLUSH_COUNT;
    }
    
    Serial.print("[SoundController] T");
    Serial.print(throttle);
    Serial.print(" Target speed change ");
    Serial.print(oldSpeed);
    Serial.print(" -> ");
    Serial.print(newSpeed);
    Serial.println();
    
    // Recalculate effort-based notch (accounts for overshoot during acceleration)
    recalculateTargetNotch(throttle);
}

void SoundController::onActualSpeedUpdate(int throttle, int actualSpeed) {
    if (!config_.enabled) return;
    if (throttle < 0 || throttle >= WIT_MAX_THROTTLES) return;
    
    actualSpeed_[throttle] = actualSpeed;
    
    // Recalculate effort notch - as actual speed catches up to target,
    // the prime mover overshoot settles back to cruising notch
    recalculateTargetNotch(throttle);
}

void SoundController::recalculateTargetNotch(int throttle) {
    int newTargetNotch = calculateEffortNotch(targetSpeed_[throttle], actualSpeed_[throttle]);
    if (newTargetNotch != targetNotch_[throttle]) {
        Serial.print("[SoundController] T");
        Serial.print(throttle);
        Serial.print(" Effort notch: ");
        Serial.print(targetNotch_[throttle]);
        Serial.print(" -> ");
        Serial.print(newTargetNotch);
        Serial.print(" (target spd=");
        Serial.print(targetSpeed_[throttle]);
        Serial.print(", actual spd=");
        Serial.print(actualSpeed_[throttle]);
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

void SoundController::triggerFunction(int throttle, uint8_t funcNum, const char* reason) {
    if (funcNum == 0 || !canTriggerFunction(throttle, funcNum)) return;
    
    unsigned long now = millis();
    
    debugPrint(throttle, funcNum, "TRIGGER", reason);
    
    // Dispatch to all sound-enabled locos using per-loco function numbers,
    // or fall back to lead-loco with global function number
    if (locoMgr_ && locoMgr_->hasSoundThrottle(throttle) && proto_) {
        char tChar = getMultiThrottleChar(throttle);
        for (const auto& cfg : locoMgr_->soundConfigs(throttle)) {
            // Determine which per-loco function to fire
            int locoFunc = funcNum;  // default to the global function number
            if (funcNum == config_.throttleUpFunction && cfg.funcThrottleUp >= 0) {
                locoFunc = cfg.funcThrottleUp;
            } else if (funcNum == config_.throttleDownFunction && cfg.funcThrottleDown >= 0) {
                locoFunc = cfg.funcThrottleDown;
            } else if (funcNum == config_.brakeFunction && cfg.funcBrake >= 0) {
                locoFunc = cfg.funcBrake;
            }
            proto_->setFunction(tChar, cfg.address, locoFunc, true, true);
            debugPrint(throttle, locoFunc, "ON", "per-loco sound");
        }
    } else {
        // Fallback: legacy single-loco behavior
        throttleMgr_->setFunction(throttle, funcNum, true, true);
        debugPrint(throttle, funcNum, "ON", "diesel sound simulation");
    }
    
    // Track pulse state for automatic OFF timing
    functionPulseActive_[throttle][funcNum] = true;
    functionPulseStart_[throttle][funcNum] = now;
    
    lastFunctionTime_[throttle][funcNum] = now;
}

void SoundController::triggerFunctionImmediate(int throttle, uint8_t funcNum, bool state) {
    if (funcNum == 0 || !throttleMgr_) return;
    
    // Fallback: send to lead loco via ThrottleManager
    throttleMgr_->setFunction(throttle, funcNum, state, true);
    debugPrint(throttle, funcNum, state ? "ON" : "OFF", "immediate diesel sound");
}

void SoundController::turnOffFunction(int throttle, uint8_t funcNum) {
    // Mirror the same per-loco dispatch used in triggerFunction, but with state=false
    if (locoMgr_ && locoMgr_->hasSoundThrottle(throttle) && proto_) {
        char tChar = getMultiThrottleChar(throttle);
        for (const auto& cfg : locoMgr_->soundConfigs(throttle)) {
            int locoFunc = funcNum;
            if (funcNum == config_.throttleUpFunction && cfg.funcThrottleUp >= 0) {
                locoFunc = cfg.funcThrottleUp;
            } else if (funcNum == config_.throttleDownFunction && cfg.funcThrottleDown >= 0) {
                locoFunc = cfg.funcThrottleDown;
            } else if (funcNum == config_.brakeFunction && cfg.funcBrake >= 0) {
                locoFunc = cfg.funcBrake;
            }
            proto_->setFunction(tChar, cfg.address, locoFunc, false, true);
        }
    } else {
        throttleMgr_->setFunction(throttle, funcNum, false, true);
    }
}

bool SoundController::canTriggerFunction(int throttle, uint8_t funcNum) {
    if (throttle < 0 || throttle >= WIT_MAX_THROTTLES) return false;
    if (funcNum >= 32) return false; // Support F0-F31
    if (!throttleMgr_) return false;
    
    unsigned long now = millis();
    
    // Don't start a new pulse if one is already active for this specific function
    if (functionPulseActive_[throttle][funcNum]) {
        debugPrint(throttle, funcNum, "SKIP", "pulse already active");
        return false;
    }
    
    // Respect cooldown period for this specific function
    if (now - lastFunctionTime_[throttle][funcNum] < config_.cooldownPeriod) {
        debugPrint(throttle, funcNum, "SKIP", "cooldown active");
        return false;
    }
    
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

// Effort-based notch: prime mover overshoots during acceleration, then settles
// Real diesel-electrics need MORE power to accelerate than to maintain speed.
// The engineer notches up high, the prime mover revs, the train sluggishly
// starts moving, and then the engineer notches back as speed is reached.
// This creates the realistic "rev up before moving" effect.
int SoundController::calculateEffortNotch(int targetSpeed, int actualSpeed) const {
    if (targetSpeed == 0) return 1;  // Idle when stopped
    
    int cruisingNotch = calculateNotchFromSpeed(targetSpeed);
    int speedDeficit = targetSpeed - actualSpeed;
    
    if (speedDeficit <= 0) {
        // At or above target speed - cruising power only
        return cruisingNotch;
    }
    
    // Accelerating: engine needs extra power to overcome inertia
    // Larger speed deficit = more throttle overshoot
    int extraNotches;
    if (speedDeficit > 64) extraNotches = 3;       // Heavy acceleration demand
    else if (speedDeficit > 32) extraNotches = 2;  // Moderate demand  
    else if (speedDeficit > 12) extraNotches = 1;  // Light demand
    else extraNotches = 0;                          // Almost at target
    
    return min(8, cruisingNotch + extraNotches);
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
            triggerFunction(throttle, config_.throttleUpFunction, "notch up");
            
        } else if (currentNotch_[throttle] > targetNotch_[throttle]) {
            // Notching down
            currentNotch_[throttle]--;
            lastNotchTime_[throttle] = now;
            triggerFunction(throttle, config_.throttleDownFunction, "notch down");
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
        
        triggerFunction(throttle, config_.throttleDownFunction, "idle flush");
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