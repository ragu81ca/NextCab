// SoundController.cpp - Event-driven diesel locomotive sound management
#include "SoundController.h"
#include "ThrottleManager.h"

SoundController::SoundController() : throttleMgr_(nullptr) {
    for (int i = 0; i < SOUND_MAX_THROTTLES; i++) {
        for (int f = 0; f < 32; f++) {
            functionPulseActive_[i][f] = false;
            functionPulseStart_[i][f] = 0;
            lastFunctionTime_[i][f] = 0;
        }
        currentNotch_[i] = 1;     // Start at notch 1 (idle)
        targetNotch_[i] = 1;      // Start at notch 1 (idle)
        lastNotchTime_[i] = 0;
    }
}

void SoundController::begin(ThrottleManager* throttleMgr) {
    throttleMgr_ = throttleMgr;
    Serial.println("[SoundController] Initialized - Diesel locomotive sound simulation");
}

void SoundController::update() {
    if (!config_.enabled || !throttleMgr_) return;
    
    unsigned long now = millis();
    
    // Handle non-blocking function pulses for all throttles and functions
    for (int throttle = 0; throttle < SOUND_MAX_THROTTLES; throttle++) {
        for (int funcNum = 0; funcNum < 32; funcNum++) {
            if (functionPulseActive_[throttle][funcNum]) {
                if (now - functionPulseStart_[throttle][funcNum] >= config_.pulseduration) {
                    // Time to turn OFF the function - always use direct control
                    // We must bypass roster settings to ensure precise timing for sound simulation
                    throttleMgr_->setFunction(throttle, funcNum, false, true); // Force OFF
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
    
    Serial.print("[SoundController] T");
    Serial.print(throttle);
    Serial.print(" Brake: ");
    Serial.println(braking ? "APPLIED" : "RELEASED");
    
    if (config_.brakeFunction != 0) {
        // Brake function is typically latching (ON when braking, OFF when not)
        triggerFunctionImmediate(throttle, config_.brakeFunction, braking);
    }
}

void SoundController::onSpeedChange(int throttle, int oldSpeed, int newSpeed) {
    if (!config_.enabled) return;
    
    // Update target notch based on new speed
    if (throttle >= 0 && throttle < SOUND_MAX_THROTTLES) {
        int newTargetNotch = calculateNotchFromSpeed(newSpeed);
        if (targetNotch_[throttle] != newTargetNotch) {
            targetNotch_[throttle] = newTargetNotch;
            
            Serial.print("[SoundController] T");
            Serial.print(throttle);
            Serial.print(" Speed change ");
            Serial.print(oldSpeed);
            Serial.print(" -> ");
            Serial.print(newSpeed);
            Serial.print(" (target notch now ");
            Serial.print(newTargetNotch);
            Serial.println(")");
        }
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
    
    // Always use direct function control for diesel sound simulation
    // We must bypass roster latching settings to ensure precise ON/OFF timing
    throttleMgr_->setFunction(throttle, funcNum, true, true); // Force ON
    debugPrint(throttle, funcNum, "ON", "diesel sound simulation");
    
    // Track pulse state for automatic OFF timing
    functionPulseActive_[throttle][funcNum] = true;
    functionPulseStart_[throttle][funcNum] = now;
    
    lastFunctionTime_[throttle][funcNum] = now;
}

void SoundController::triggerFunctionImmediate(int throttle, uint8_t funcNum, bool state) {
    if (funcNum == 0 || !throttleMgr_) return;
    
    // Always use direct function control - bypass roster settings for sound simulation
    throttleMgr_->setFunction(throttle, funcNum, state, true); // Force state
    debugPrint(throttle, funcNum, state ? "ON" : "OFF", "immediate diesel sound");
}

bool SoundController::canTriggerFunction(int throttle, uint8_t funcNum) {
    if (throttle < 0 || throttle >= SOUND_MAX_THROTTLES) return false;
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
// Note: Steam locomotives would use completely different sound patterns (chuff rates, etc.)
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
    if (throttle < 0 || throttle >= SOUND_MAX_THROTTLES) return;
    
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
}

// ============================================================================
// Notching State Query (used by MomentumController for "sound leads movement")
// ============================================================================

bool SoundController::isNotching(int throttle) const {
    if (!config_.enabled) return false;
    if (throttle < 0 || throttle >= SOUND_MAX_THROTTLES) return false;
    
    // We're notching if current notch differs from target
    return currentNotch_[throttle] != targetNotch_[throttle];
}

bool SoundController::isAnyNotching() const {
    if (!config_.enabled) return false;
    
    for (int i = 0; i < SOUND_MAX_THROTTLES; i++) {
        if (currentNotch_[i] != targetNotch_[i]) {
            return true;
        }
    }
    return false;
}