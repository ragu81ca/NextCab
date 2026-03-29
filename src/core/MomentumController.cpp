// MomentumController.cpp - Implementation of throttle momentum/inertia simulation
#include "MomentumController.h"

#ifdef UNIT_TEST
  // Test stubs — lightweight replacements for heavy production headers.
  // Explicit relative paths bypass compiler file-relative resolution.
  #include "../../test/stubs/SoundController.h"
  #include "../../test/stubs/ThrottleManager.h"
  #include "../../test/stubs/storage/ConfigStore.h"
#else
  #include "ThrottleManager.h"
  #include "SoundController.h"
  #include "storage/ConfigStore.h"  // LocoType enum
#endif

#include <math.h>

MomentumController::MomentumController(ClockFn clock) 
    : throttleMgr_(nullptr), soundCtrl_(nullptr), clock_(clock ? clock : millis) {
    for (int i = 0; i < MOMENTUM_MAX_THROTTLES; i++) {
        momentumLevel_[i] = MomentumLevel::Off;
        targetSpeed_[i] = 0;
        actualSpeed_[i] = 0.0f;
        braking_[i] = false;
        dynamicBraking_[i] = false;
        consistSize_[i] = 1;
        locoType_[i] = LocoType::Diesel;
        lastUpdate_[i] = 0;
        pendingDirectionChange_[i] = false;
        pendingDirection_[i] = Forward;
        originalDirection_[i] = Forward;
    }
}

void MomentumController::begin(ThrottleManager* throttleMgr, SoundController* soundCtrl) {
    throttleMgr_ = throttleMgr;
    soundCtrl_ = soundCtrl;
}

void MomentumController::update() {
    // Always process momentum updates - when "off", rates are set to 999.0 for instant changes
    unsigned long now = clock_();
    
    // When momentum is off, use faster update rate for immediate response
    unsigned long updateInterval = isAnyActive() ? UPDATE_INTERVAL_MS : 50; // 50ms when off, 500ms when active
    
    for (int throttle = 0; throttle < MOMENTUM_MAX_THROTTLES; throttle++) {
        // Check if enough time has passed for this throttle
        unsigned long elapsed = now - lastUpdate_[throttle];
        if (elapsed < updateInterval) {
            continue;
        }
        
        int target = targetSpeed_[throttle];
        float actual = actualSpeed_[throttle];
        
        // Service braking: when the operator holds the encoder while throttle > 0,
        // continuously decelerate as though applying brakes, but leave the displayed
        // target speed untouched.  The deceleration rate and minimum effective speed
        // depend on the locomotive type (diesel/steam/electric).
        bool serviceActive = dynamicBraking_[throttle] && target > 0;
        if (serviceActive) {
            const BrakeProfile& profile = getBrakeProfile(throttle);
            if ((int)round(actual) <= profile.minSpeed) {
                // Below effective threshold — hold at current speed
                serviceActive = false;  // let normal logic handle this tick
            } else {
                // Override target to 0 so the momentum system decelerates;
                // the brake rate below will be used instead of decel rate.
                target = max(0, profile.minSpeed);
            }
        }
        
        // Sound leads movement: If sound controller is still notching for this throttle,
        // delay the actual speed change so engine sound transitions complete first.
        // Don't reset lastUpdate_ here — elapsed time accumulates so that when
        // notching finishes, the proportionally larger delta lets speed catch up
        // smoothly instead of surging.
        if (soundCtrl_ && soundCtrl_->isNotching(throttle)) {
            continue;
        }
        
        // Mark this throttle as processed (after notching check so delays accumulate)
        lastUpdate_[throttle] = now;
        
        // Already at target? Nothing to do
        if (abs(actual - target) < 0.5f) {
            actualSpeed_[throttle] = (float)target;
            
            // Safety check: if train has stopped and there's a pending direction change, notify ThrottleManager
            // This allows the queued direction change to be applied now that it's safe.
            if (target == 0 && actual == 0 && hasPendingDirectionChange(throttle)) {
                // Release brake since we've reached stop
                if (braking_[throttle]) {
                    setBraking(throttle, false);
                }
                
                #ifdef MOMENTUM_DEBUG
                Serial.print("[Momentum] T");
                Serial.print(throttle);
                Serial.println(" Stopped - ready for pending direction change");
                #endif
                
                // ThrottleManager will poll hasPendingDirectionChange() and apply the direction
                // Note: We don't apply it here because MomentumController shouldn't directly
                // call ThrottleManager's direction methods (would create circular dependency)
            }
            
            continue;
        }
        
        bool accelerating = (target > actual);
        
        // Get base rate based on momentum level and direction (per-throttle)
        float baseRate;
        if (serviceActive && !accelerating) {
            // Service braking: use loco-type-specific deceleration rate
            const BrakeProfile& profile = getBrakeProfile(throttle);
            baseRate = profile.decelRate;
        } else if (braking_[throttle] && !accelerating) {
            baseRate = getBrakeRate(throttle); // Faster decel when braking (throttle=0 hold)
        } else if (accelerating) {
            baseRate = getAccelRate(throttle);
        } else {
            baseRate = getDecelRate(throttle);
        }
        
        // Calculate delta using actual elapsed time for physically accurate ramping.
        // Cap at 1000ms to prevent huge jumps after long pauses (e.g., task preemption).
        float elapsedSec = min(elapsed, (unsigned long)1000) / 1000.0f;
        float delta = baseRate * elapsedSec;
        
        // Apply curve for natural feel (speed-dependent tractive effort / resistance)
        if (accelerating) {
            delta = applyAccelCurve(delta, actual);
            // Consist size bonus: each additional loco contributes ~15% more tractive effort.
            // This silently makes larger consists accelerate faster, just like real railroads.
            int consist = consistSize_[throttle];
            if (consist > 1) {
                float consistBonus = 1.0f + (consist - 1) * 0.15f;
                delta *= consistBonus;
            }
        } else {
            delta = applyDecelCurve(delta, actual);
        }
        
        // Apply delta
        float newSpeed;
        if (accelerating) {
            newSpeed = min(actual + delta, (float)target);
        } else {
            newSpeed = max(actual - delta, (float)target);
        }
        
        // Debug output when speed changes
        if ((int)round(newSpeed) != (int)round(actual)) {
            const char* levelNames[] = {"Off", "Low", "Med", "High"};
            Serial.print("Momentum[");
            Serial.print(levelNames[(int)momentumLevel_[throttle]]);
            Serial.print("] T");
            Serial.print(throttle);
            Serial.print(": ");
            Serial.print((int)round(actual));
            Serial.print(" -> ");
            Serial.print((int)round(newSpeed));
            Serial.print(" (target: ");
            Serial.print(target);
            Serial.print(", delta: ");
            Serial.print(delta, 1);
            Serial.println(")");
            
            // Notify SoundController of actual speed change so effort notch
            // can recalculate (prime mover settles as train reaches speed)
            if (soundCtrl_) {
                soundCtrl_->onActualSpeedUpdate(throttle, (int)round(newSpeed));
            }
        }
        
        actualSpeed_[throttle] = newSpeed;
    }
}

void MomentumController::setTargetSpeed(int throttle, int speed) {
    if (throttle < 0 || throttle >= MOMENTUM_MAX_THROTTLES) return;
    
    // Clamp to valid range
    if (speed < 0) speed = 0;
    if (speed > 126) speed = 126;
    
    int oldTarget = targetSpeed_[throttle];
    targetSpeed_[throttle] = speed;
    
    // Emit sound event when target speed changes (user moves throttle)
    if (soundCtrl_ && speed != oldTarget) {
        soundCtrl_->onSpeedChange(throttle, oldTarget, speed);
    }
    
    // Debug output only if target actually changed
    if (speed != oldTarget && isActive(throttle)) {
        Serial.print("Target speed set T");
        Serial.print(throttle);
        Serial.print(": ");
        Serial.print(oldTarget);
        Serial.print(" -> ");
        Serial.print(speed);
        Serial.print(" (actual: ");
        Serial.print((int)round(actualSpeed_[throttle]));
        Serial.println(")");
    }
    
    // If momentum is off for this throttle, snap actual to target immediately
    if (!isActive(throttle)) {
        actualSpeed_[throttle] = (float)speed;
    }
}

int MomentumController::getActualSpeed(int throttle) const {
    if (throttle < 0 || throttle >= MOMENTUM_MAX_THROTTLES) return 0;
    return (int)round(actualSpeed_[throttle]);
}

int MomentumController::getTargetSpeed(int throttle) const {
    if (throttle < 0 || throttle >= MOMENTUM_MAX_THROTTLES) return 0;
    return targetSpeed_[throttle];
}

void MomentumController::emergencyStop(int throttle) {
    if (throttle < 0 || throttle >= MOMENTUM_MAX_THROTTLES) return;
    targetSpeed_[throttle] = 0;
    actualSpeed_[throttle] = 0.0f; // Bypass momentum
}

void MomentumController::emergencyStopAll() {
    for (int i = 0; i < MOMENTUM_MAX_THROTTLES; i++) {
        emergencyStop(i);
    }
}

void MomentumController::setMomentumLevel(int throttle, MomentumLevel level) {
    if (throttle < 0 || throttle >= MOMENTUM_MAX_THROTTLES) return;
    
    MomentumLevel oldLevel = momentumLevel_[throttle];
    momentumLevel_[throttle] = level;
    
    // Debug output
    const char* levelNames[] = {"Off", "Low", "Med", "High"};
    Serial.print("Momentum level T");
    Serial.print(throttle);
    Serial.print(" changed: ");
    Serial.print(levelNames[(int)oldLevel]);
    Serial.print(" -> ");
    Serial.println(levelNames[(int)level]);
    
    // If turning off, snap this throttle's actual speed to target
    if (level == MomentumLevel::Off && oldLevel != MomentumLevel::Off) {
        actualSpeed_[throttle] = (float)targetSpeed_[throttle];
        Serial.print("Momentum disabled T");
        Serial.print(throttle);
        Serial.println(" - speed snapped to target");
    }
}

void MomentumController::setMomentumLevelAll(MomentumLevel level) {
    for (int i = 0; i < MOMENTUM_MAX_THROTTLES; i++) {
        setMomentumLevel(i, level);
    }
}

MomentumLevel MomentumController::getMomentumLevel(int throttle) const {
    if (throttle < 0 || throttle >= MOMENTUM_MAX_THROTTLES) return MomentumLevel::Off;
    return momentumLevel_[throttle];
}

bool MomentumController::isActive(int throttle) const {
    if (throttle < 0 || throttle >= MOMENTUM_MAX_THROTTLES) return false;
    return momentumLevel_[throttle] != MomentumLevel::Off;
}

bool MomentumController::isAnyActive() const {
    for (int i = 0; i < MOMENTUM_MAX_THROTTLES; i++) {
        if (momentumLevel_[i] != MomentumLevel::Off) return true;
    }
    return false;
}

void MomentumController::cycleMomentumLevel(int throttle) {
    if (throttle < 0 || throttle >= MOMENTUM_MAX_THROTTLES) return;
    int current = (int)momentumLevel_[throttle];
    current = (current + 1) % 4; // 0->1->2->3->0
    setMomentumLevel(throttle, (MomentumLevel)current);
}

void MomentumController::setBraking(int throttle, bool braking) {
    if (throttle < 0 || throttle >= MOMENTUM_MAX_THROTTLES) return;
    
    bool wasbraking = braking_[throttle];
    braking_[throttle] = braking;
    
    // Emit brake state change event to sound controller
    if (braking != wasbraking && soundCtrl_) {
        soundCtrl_->onBrakeStateChange(throttle, braking);
    }
}

bool MomentumController::isBraking(int throttle) const {
    if (throttle < 0 || throttle >= MOMENTUM_MAX_THROTTLES) return false;
    return braking_[throttle];
}

void MomentumController::setDynamicBraking(int throttle, bool active) {
    if (throttle < 0 || throttle >= MOMENTUM_MAX_THROTTLES) return;
    
    bool wasActive = dynamicBraking_[throttle];
    
    if (active && !wasActive) {
        // When momentum is active, check actual (smoothed) speed.
        // When off, check the flag itself — caller has already stopped the loco.
        if (isActive(throttle)) {
            int actualSpd = (int)round(actualSpeed_[throttle]);
            const BrakeProfile& profile = getBrakeProfile(throttle);
            if (actualSpd < profile.minSpeed) {
                Serial.print("[Momentum] T");
                Serial.print(throttle);
                Serial.print(" Dynamic brake ignored - speed too low (");
                Serial.print(actualSpd);
                Serial.println(")");
                return;
            }
        }
        
        dynamicBraking_[throttle] = true;
        
        const char* typeNames[] = {"Diesel", "Steam", "Electric"};
        int typeIdx = (int)locoType_[throttle];
        Serial.print("[Momentum] T");
        Serial.print(throttle);
        Serial.print(" Dynamic brake ENGAGED (");
        Serial.print(typeNames[typeIdx]);
        Serial.print(") - speed: ");
        Serial.print((int)round(actualSpeed_[throttle]));
        Serial.print(", momentum: ");
        Serial.println(isActive(throttle) ? "on" : "off");
        
        // Notify sound controller
        if (soundCtrl_) {
            soundCtrl_->onDynamicBrakeStateChange(throttle, true);
        }
    } else if (!active && wasActive) {
        // Releasing dynamic brake - momentum carries speed back to throttle setting
        dynamicBraking_[throttle] = false;
        
        Serial.print("[Momentum] T");
        Serial.print(throttle);
        Serial.println(" Dynamic brake RELEASED - returning to target speed");
        
        // Notify sound controller
        if (soundCtrl_) {
            soundCtrl_->onDynamicBrakeStateChange(throttle, false);
        }
    }
}

bool MomentumController::isDynamicBraking(int throttle) const {
    if (throttle < 0 || throttle >= MOMENTUM_MAX_THROTTLES) return false;
    return dynamicBraking_[throttle];
}

void MomentumController::setLocoType(int throttle, LocoType type) {
    if (throttle < 0 || throttle >= MOMENTUM_MAX_THROTTLES) return;
    locoType_[throttle] = type;
}

LocoType MomentumController::getLocoType(int throttle) const {
    if (throttle < 0 || throttle >= MOMENTUM_MAX_THROTTLES) return LocoType::Diesel;
    return locoType_[throttle];
}

const MomentumController::BrakeProfile& MomentumController::getBrakeProfile(int throttle) const {
    if (throttle < 0 || throttle >= MOMENTUM_MAX_THROTTLES) return BRAKE_PROFILES[0];
    int idx = (int)locoType_[throttle];
    if (idx < 0 || idx > 2) idx = 0;
    return BRAKE_PROFILES[idx];
}

void MomentumController::setConsistSize(int throttle, int locoCount) {
    if (throttle < 0 || throttle >= MOMENTUM_MAX_THROTTLES) return;
    if (locoCount < 1) locoCount = 1;
    consistSize_[throttle] = locoCount;
    
    Serial.print("[Momentum] T");
    Serial.print(throttle);
    Serial.print(" Consist size set to ");
    Serial.println(locoCount);
}

int MomentumController::getConsistSize(int throttle) const {
    if (throttle < 0 || throttle >= MOMENTUM_MAX_THROTTLES) return 1;
    return consistSize_[throttle];
}

// Acceleration rates (speed units per second)
// Realistic momentum based on prototype physics:
// Low = light passenger (12s), Medium = mixed freight (25s), High = heavy freight (50s)
float MomentumController::getAccelRate(int throttle) const {
    if (throttle < 0 || throttle >= MOMENTUM_MAX_THROTTLES) return 999.0f;
    switch (momentumLevel_[throttle]) {
        case MomentumLevel::Low:    return 10.5f;  // ~12 seconds 0-126
        case MomentumLevel::Medium: return 5.0f;   // ~25 seconds 0-126
        case MomentumLevel::High:   return 2.5f;   // ~50 seconds 0-126
        default:                    return 999.0f; // Instant (off)
    }
}

// Deceleration rates (speed units per second) - MUCH slower for realistic coasting
// Trains coast 1.5-2x longer than they accelerate
float MomentumController::getDecelRate(int throttle) const {
    if (throttle < 0 || throttle >= MOMENTUM_MAX_THROTTLES) return 999.0f;
    switch (momentumLevel_[throttle]) {
        case MomentumLevel::Low:    return 7.0f;   // ~18 seconds (1.5x accel)
        case MomentumLevel::Medium: return 3.2f;   // ~40 seconds (1.6x accel)
        case MomentumLevel::High:   return 1.4f;   // ~90 seconds (1.8x accel!)
        default:                    return 999.0f; // Instant (off)
    }
}

// Brake rates (speed units per second) - ACTIVE braking when user holds encoder
// Much faster than coasting, but still realistic train braking
float MomentumController::getBrakeRate(int throttle) const {
    if (throttle < 0 || throttle >= MOMENTUM_MAX_THROTTLES) return 999.0f;
    switch (momentumLevel_[throttle]) {
        case MomentumLevel::Low:    return 25.0f;  // ~5 seconds (emergency stop feel)
        case MomentumLevel::Medium: return 15.0f;  // ~8 seconds (moderate braking)
        case MomentumLevel::High:   return 10.0f;  // ~13 seconds (heavy train needs time!)
        default:                    return 999.0f; // Instant (off)
    }
}

// Acceleration curve - diesel-electric tractive effort decreases at higher speeds
// At low speed the motors have maximum torque; at high speed, back-EMF reduces
// available current. This makes acceleration brisk at first, then tapering off.
// Consist size bonus: each additional loco contributes ~15% more tractive effort,
// simulating the real-world benefit of multiple units in a consist.
float MomentumController::applyAccelCurve(float delta, float actualSpeed) const {
    float speedFraction = actualSpeed / 126.0f;
    // Quadratic taper: 100% effort at speed 0, ~60% at speed 126
    float factor = 1.0f - 0.4f * speedFraction * speedFraction;
    return delta * factor;
}

// Deceleration curve - rolling resistance + aerodynamic drag increase with speed
// Trains coast much further at low speeds than high speeds.
// At high speed: drag slows them faster; at low speed: they roll a long time.
float MomentumController::applyDecelCurve(float delta, float actualSpeed) const {
    float speedFraction = actualSpeed / 126.0f;
    // Linear increase: 70% at low speed (long coast), 130% at high speed (more drag)
    float factor = 0.7f + 0.6f * speedFraction;
    return delta * factor;
}

void MomentumController::triggerBrakeSound(int throttle, bool enable) {
    // Legacy method - now handled by SoundController via events
    // Kept for compatibility but functionality moved to setBraking event emission
}

// ============================================================================
// Direction Change Safety
// ============================================================================

// Request a direction change. If train is moving, queues the change and starts braking.
// If already queued, toggles back (cancels the pending change).
// Returns true if direction change was queued (train is moving), false if no action needed (stopped).
bool MomentumController::requestDirectionChange(int throttle, Direction targetDirection) {
    if (throttle < 0 || throttle >= MOMENTUM_MAX_THROTTLES) return false;
    
    // If momentum is off for this throttle, don't queue - let caller handle immediately
    if (!isActive(throttle)) return false;
    
    // Check if train is actually moving
    int actualSpeed = (int)round(actualSpeed_[throttle]);
    
    if (actualSpeed > 0) {
        // Train is moving
        
        // If already pending, toggle it back (cancel)
        if (pendingDirectionChange_[throttle]) {
            pendingDirectionChange_[throttle] = false;
            
            #ifdef MOMENTUM_DEBUG
            Serial.print("[Momentum] T");
            Serial.print(throttle);
            Serial.println(" Direction change CANCELLED");
            #endif
            
            return false; // Cancelled, no longer pending
        }
        
        // Queue the direction change — train coasts to a stop naturally.
        // User can manually brake (encoder hold) to stop faster.
        pendingDirectionChange_[throttle] = true;
        pendingDirection_[throttle] = targetDirection;
        originalDirection_[throttle] = targetDirection == Forward ? Reverse : Forward;
        
        #ifdef MOMENTUM_DEBUG
        Serial.print("[Momentum] T");
        Serial.print(throttle);
        Serial.print(" Direction change queued to ");
        Serial.print(targetDirection == Forward ? "Forward" : "Reverse");
        Serial.print(" - coasting to stop (speed=");
        Serial.print(actualSpeed);
        Serial.println(")");
        #endif
        
        return true; // Direction change queued
    }
    
    // Train is stopped - no need to queue
    return false; // Caller should apply direction change immediately
}

bool MomentumController::hasPendingDirectionChange(int throttle) const {
    if (throttle < 0 || throttle >= MOMENTUM_MAX_THROTTLES) return false;
    return pendingDirectionChange_[throttle];
}

Direction MomentumController::getPendingDirection(int throttle) const {
    if (throttle < 0 || throttle >= MOMENTUM_MAX_THROTTLES) return Forward;
    return pendingDirection_[throttle];
}

Direction MomentumController::getOriginalDirection(int throttle) const {
    if (throttle < 0 || throttle >= MOMENTUM_MAX_THROTTLES) return Forward;
    return originalDirection_[throttle];
}

void MomentumController::clearPendingDirectionChange(int throttle) {
    if (throttle < 0 || throttle >= MOMENTUM_MAX_THROTTLES) return;
    pendingDirectionChange_[throttle] = false;
}

// Out-of-class definition required for constexpr array (C++14/17 ODR)
constexpr MomentumController::BrakeProfile MomentumController::BRAKE_PROFILES[];
