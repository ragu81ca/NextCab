// MomentumController.cpp - Implementation of throttle momentum/inertia simulation
#include "MomentumController.h"
#include <math.h>

MomentumController::MomentumController() 
    : momentumLevel_(MomentumLevel::Off) {
    for (int i = 0; i < MOMENTUM_MAX_THROTTLES; i++) {
        targetSpeed_[i] = 0;
        actualSpeed_[i] = 0.0f;
        braking_[i] = false;
        lastUpdate_[i] = 0;
    }
}

void MomentumController::begin() {
    // Initialize if needed
}

void MomentumController::update() {
    if (!isActive()) return; // Skip if momentum is off
    
    unsigned long now = millis();
    
    for (int throttle = 0; throttle < MOMENTUM_MAX_THROTTLES; throttle++) {
        // Check if enough time has passed for this throttle
        if (now - lastUpdate_[throttle] < UPDATE_INTERVAL_MS) {
            continue;
        }
        
        lastUpdate_[throttle] = now;
        
        int target = targetSpeed_[throttle];
        float actual = actualSpeed_[throttle];
        
        // Already at target? Nothing to do
        if (abs(actual - target) < 0.5f) {
            actualSpeed_[throttle] = (float)target;
            continue;
        }
        
        // Calculate distance to target
        float distance = abs(target - actual);
        bool accelerating = (target > actual);
        
        // Get base rate based on momentum level and direction
        float baseRate;
        if (braking_[throttle] && !accelerating) {
            baseRate = getBrakeRate(); // Faster decel when braking
        } else if (accelerating) {
            baseRate = getAccelRate();
        } else {
            baseRate = getDecelRate();
        }
        
        // Calculate delta for this update
        float delta = baseRate * (UPDATE_INTERVAL_MS / 1000.0f); // Scale by time
        
        // Apply curve for natural feel
        if (accelerating) {
            delta = applyAccelCurve(delta, distance);
        } else {
            delta = applyDecelCurve(delta, distance);
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
            Serial.print(levelNames[(int)momentumLevel_]);
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
    
    // Debug output only if target actually changed
    if (speed != oldTarget && isActive()) {
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
    
    // If momentum is off, snap actual to target immediately
    if (!isActive()) {
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

void MomentumController::setMomentumLevel(MomentumLevel level) {
    MomentumLevel oldLevel = momentumLevel_;
    momentumLevel_ = level;
    
    // Debug output
    const char* levelNames[] = {"Off", "Low", "Med", "High"};
    Serial.print("Momentum level changed: ");
    Serial.print(levelNames[(int)oldLevel]);
    Serial.print(" -> ");
    Serial.println(levelNames[(int)level]);
    
    // If turning off, snap all actual speeds to targets
    if (level == MomentumLevel::Off && oldLevel != MomentumLevel::Off) {
        for (int i = 0; i < MOMENTUM_MAX_THROTTLES; i++) {
            actualSpeed_[i] = (float)targetSpeed_[i];
        }
        Serial.println("Momentum disabled - all speeds snapped to target");
    }
}

void MomentumController::cycleMomentumLevel() {
    int current = (int)momentumLevel_;
    current = (current + 1) % 4; // 0->1->2->3->0
    setMomentumLevel((MomentumLevel)current);
}

void MomentumController::setBraking(int throttle, bool braking) {
    if (throttle < 0 || throttle >= MOMENTUM_MAX_THROTTLES) return;
    braking_[throttle] = braking;
}

bool MomentumController::isBraking(int throttle) const {
    if (throttle < 0 || throttle >= MOMENTUM_MAX_THROTTLES) return false;
    return braking_[throttle];
}

// Acceleration rates (speed units per second)
// Doubled ramp times: Low=~4s, Medium=~8s, High=~16s
float MomentumController::getAccelRate() const {
    switch (momentumLevel_) {
        case MomentumLevel::Low:    return 30.0f;  // ~4 seconds 0-126
        case MomentumLevel::Medium: return 15.0f;  // ~8 seconds 0-126
        case MomentumLevel::High:   return 7.5f;   // ~16 seconds 0-126
        default:                    return 999.0f; // Instant (off)
    }
}

// Deceleration rates (speed units per second) - SLOWER than accel for realistic coasting
float MomentumController::getDecelRate() const {
    switch (momentumLevel_) {
        case MomentumLevel::Low:    return 15.0f;  // ~8 seconds (2x slower than accel)
        case MomentumLevel::Medium: return 10.0f;  // ~12 seconds (1.5x slower than accel)
        case MomentumLevel::High:   return 5.0f;   // ~25 seconds (3x+ slower than accel!)
        default:                    return 999.0f; // Instant (off)
    }
}

// Brake rates (speed units per second) - for future Stage 4/5 implementation
// These will be used when user holds down decel button or similar
float MomentumController::getBrakeRate() const {
    switch (momentumLevel_) {
        case MomentumLevel::Low:    return 50.0f;  // ~2.5 seconds (faster than normal decel)
        case MomentumLevel::Medium: return 40.0f;  // ~3.2 seconds
        case MomentumLevel::High:   return 30.0f;  // ~4.2 seconds
        default:                    return 999.0f; // Instant (off)
    }
}

// Acceleration curve - linear for predictable feel with 500ms updates
float MomentumController::applyAccelCurve(float delta, float distance) const {
    // Linear ramping - consistent speed increase
    return delta;
}

// Deceleration curve - linear for predictable feel with 500ms updates  
float MomentumController::applyDecelCurve(float delta, float distance) const {
    // Linear ramping - consistent speed decrease
    return delta;
}
