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
    unsigned long now = clock_();
    
    for (int throttle = 0; throttle < MOMENTUM_MAX_THROTTLES; throttle++) {
        // Momentum off: commanded speed is the loco speed, nothing to integrate.
        if (!isActive(throttle)) {
            lastUpdate_[throttle] = now;
            continue;
        }
        
        unsigned long elapsed = now - lastUpdate_[throttle];
        if (elapsed < UPDATE_INTERVAL_MS) {
            continue;
        }
        lastUpdate_[throttle] = now;
        
        // Cap at 1000ms so a long pause can't integrate a huge jump.
        updatePhysics(throttle, min(elapsed, (unsigned long)1000) / 1000.0f);
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
        Serial.print("Commanded speed T");
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

int MomentumController::getPowerPercent(int throttle) const {
    if (throttle < 0 || throttle >= MOMENTUM_MAX_THROTTLES) return 0;
    return (int)round(targetSpeed_[throttle] * 100.0f / 126.0f);
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

// ============================================================================
// Power model — simulator mode
// ============================================================================
//
// Longitudinal train dynamics, the standard textbook model:
//
//     m · dv/dt  =  TE(power, v)  −  R(v)  −  B(brake)
//
//   R(v)  Davis equation — A (journal/bearing) + B·v (rolling) + C·v² (aero),
//         normalised so R(126) == 1.0, i.e. full effort balances at full speed.
//   TE    Tractive effort, limited by wheel adhesion at low speed and regulated
//         toward the balancing speed the driver has commanded (governor droop).
//   m     Train mass — this is what the Low/Medium/High momentum setting selects.
//
// Everything the operator feels (slow start, gradual roll-down when power is
// reduced, long coast with power off) falls out of the integration; there are
// no ramp rates or thresholds in the power path.

static constexpr float DAVIS_A        = 0.06f;      // constant journal/bearing resistance
static constexpr float DAVIS_B        = 0.0027f;    // rolling/flange, proportional to v
static constexpr float DAVIS_C        = 3.78e-5f;   // aerodynamic, proportional to v²
static constexpr float STICTION       = 0.05f;      // extra breakaway resistance at a standstill
static constexpr float STICTION_SPEED = 4.0f;       // stiction has decayed to zero by this speed
static constexpr float ADHESION_LIMIT = 3.0f;       // max tractive effort before wheel slip
static constexpr float GOVERNOR_GAIN  = 0.05f;      // effort added per speed step below balancing speed
static constexpr float ENGINE_BRAKE_FRACTION = 0.5f; // share of the governor error that retards above balancing speed
static constexpr float ENGINE_BRAKE_LIMIT    = 0.5f; // cap on that retarding force
static constexpr float CONSIST_EFFORT_BONUS = 0.15f; // extra adhesion per additional loco

float MomentumController::resistance(float speedSteps) {
    if (speedSteps < 0.0f) speedSteps = 0.0f;
    float r = DAVIS_A + DAVIS_B * speedSteps + DAVIS_C * speedSteps * speedSteps;
    if (speedSteps < STICTION_SPEED) {
        r += STICTION * (1.0f - speedSteps / STICTION_SPEED);
    }
    return r;
}

// Tractive effort available at the current speed for the commanded balancing speed.
// Held at that speed by governor droop, capped by wheel adhesion — which is what
// stops low speeds from surging when the throttle is opened.
float MomentumController::tractiveEffort(int throttle, float speed) const {
    int balancingSpeed = targetSpeed_[throttle];
    if (balancingSpeed <= 0) return 0.0f;   // power off — coast on drag alone
    
    float effort = resistance(balancingSpeed) + GOVERNOR_GAIN * (balancingSpeed - speed);
    if (effort < 0.0f) {
        // Above the balancing speed the engine retards instead of driving
        // (compression braking / regeneration), so power reductions bite.
        return max(effort * ENGINE_BRAKE_FRACTION, -ENGINE_BRAKE_LIMIT);
    }
    
    float adhesion = ADHESION_LIMIT;
    int consist = consistSize_[throttle];
    if (consist > 1) adhesion *= 1.0f + (consist - 1) * CONSIST_EFFORT_BONUS;
    
    return min(effort, adhesion);
}

// Mass is what the momentum level actually selects.  Derived from the level's
// acceleration rate so full power from a standstill still gives that rate.
float MomentumController::trainMass(int throttle) const {
    float netEffort = ADHESION_LIMIT - resistance(0.0f);
    return netEffort / getAccelRate(throttle);
}

void MomentumController::updatePhysics(int throttle, float dtSeconds) {
    float speed = actualSpeed_[throttle];
    float mass  = trainMass(throttle);
    
    const BrakeProfile& profile = getBrakeProfile(throttle);
    bool serviceBraking = dynamicBraking_[throttle] && speed > profile.minSpeed;
    bool trainBraking   = braking_[throttle];
    
    // Brakes cut traction; rates are expressed as decelerations, so scale by mass.
    float effort = (serviceBraking || trainBraking) ? 0.0f : tractiveEffort(throttle, speed);
    float force  = effort - resistance(speed);
    if (serviceBraking) force -= profile.decelRate * mass;
    if (trainBraking)   force -= getBrakeRate(throttle) * mass;
    
    float newSpeed = speed + (force / mass) * dtSeconds;
    if (newSpeed > 126.0f) newSpeed = 126.0f;
    if (newSpeed < 0.0f)   newSpeed = 0.0f;
    // Without traction, a train barely creeping comes to rest rather than crawling forever.
    if (effort <= 0.0f && newSpeed < 0.5f) newSpeed = 0.0f;
    
    if ((int)round(newSpeed) != (int)round(speed)) {
        Serial.print("Physics T");
        Serial.print(throttle);
        Serial.print(": ");
        Serial.print((int)round(speed));
        Serial.print(" -> ");
        Serial.print((int)round(newSpeed));
        Serial.print(" (commanded: ");
        Serial.print(targetSpeed_[throttle]);
        Serial.print(", TE: ");
        Serial.print(effort, 2);
        Serial.print(", R: ");
        Serial.print(resistance(speed), 2);
        Serial.println(")");
    }
    
    actualSpeed_[throttle] = newSpeed;
}

// Acceleration rates (speed units per second) at full power from a standstill.
// These set the train's mass: Low = light passenger (12s to full speed),
// Medium = mixed freight (25s), High = heavy freight (50s).
// Only reached when momentum is active — inactive throttles never integrate.
float MomentumController::getAccelRate(int throttle) const {
    switch (momentumLevel_[throttle]) {
        case MomentumLevel::Low:    return 10.5f;  // ~12 seconds 0-126
        case MomentumLevel::High:   return 2.5f;   // ~50 seconds 0-126
        default:                    return 5.0f;   // Medium: ~25 seconds 0-126
    }
}

// Brake rates (speed units per second) - ACTIVE braking when user holds encoder
// Much faster than coasting, but still realistic train braking
float MomentumController::getBrakeRate(int throttle) const {
    switch (momentumLevel_[throttle]) {
        case MomentumLevel::Low:    return 25.0f;  // ~5 seconds (emergency stop feel)
        case MomentumLevel::High:   return 10.0f;  // ~13 seconds (heavy train needs time!)
        default:                    return 15.0f;  // Medium: ~8 seconds (moderate braking)
    }
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
        
        // Shut off power so the train can actually come to a stand.
        targetSpeed_[throttle] = 0;
        
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
