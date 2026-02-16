// MomentumController.h - Simulated momentum/inertia for throttle control
#pragma once
#include <Arduino.h>
#include <WiThrottleProtocol.h> // For Direction enum

// Forward declarations
class ThrottleManager;
class SoundController;

// Momentum levels (Off, Low, Medium, High)
enum class MomentumLevel {
    Off = 0,
    Low = 1,
    Medium = 2,
    High = 3
};

class MomentumController {
public:
    MomentumController();
    
    // Initialize with references
    void begin(ThrottleManager* throttleMgr, SoundController* soundCtrl = nullptr);
    
    // Called from main loop - updates actual speed towards target
    void update();
    
    // Set target speed for a throttle (what user wants)
    void setTargetSpeed(int throttle, int speed);
    
    // Get actual speed for a throttle (what's sent to loco after momentum)
    int getActualSpeed(int throttle) const;
    
    // Get target speed for a throttle (what user set)
    int getTargetSpeed(int throttle) const;
    
    // Emergency stop - bypasses momentum
    void emergencyStop(int throttle);
    void emergencyStopAll();
    
    // Momentum level control (per-throttle)
    void setMomentumLevel(int throttle, MomentumLevel level);
    void setMomentumLevelAll(MomentumLevel level); // convenience: set all throttles
    MomentumLevel getMomentumLevel(int throttle) const;
    void cycleMomentumLevel(int throttle); // Off -> Low -> Medium -> High -> Off
    
    // Check if momentum is active for a specific throttle
    bool isActive(int throttle) const;
    // Check if ANY throttle has momentum active (for global decisions like update interval)
    bool isAnyActive() const;
    
    // Brake control (for future stage 5)
    void setBraking(int throttle, bool braking);
    bool isBraking(int throttle) const;
    
    // Direction change safety: request direction change while train is moving.
    // If already pending, this toggles it back (cancels the change).
    // Returns true if direction change was queued (train is moving), false if applied immediately (stopped).
    bool requestDirectionChange(int throttle, Direction targetDirection);
    
    // Check if there's a pending direction change for a throttle
    bool hasPendingDirectionChange(int throttle) const;
    
    // Get the pending target direction (for display feedback)
    Direction getPendingDirection(int throttle) const;
    
    // Get the original direction (direction train is actually still moving)
    Direction getOriginalDirection(int throttle) const;
    
    // Clear pending direction change (called by ThrottleManager after applying)
    void clearPendingDirectionChange(int throttle);
    
private:
    // Calculate rate of change based on momentum level for a specific throttle
    float getAccelRate(int throttle) const;
    float getDecelRate(int throttle) const;
    float getBrakeRate(int throttle) const;
    
	// Logarithmic curve for natural feel
	float applyAccelCurve(float delta, float distance) const;
	float applyDecelCurve(float delta, float distance) const;
	
	// Use WIT_MAX_THROTTLES to avoid conflict with MAX_THROTTLES macro
	static constexpr int MOMENTUM_MAX_THROTTLES = 6;
	
	// Per-throttle momentum level
	MomentumLevel momentumLevel_[MOMENTUM_MAX_THROTTLES];
	
	// Per-throttle state
	int targetSpeed_[MOMENTUM_MAX_THROTTLES];      // What user set (0-126)
	float actualSpeed_[MOMENTUM_MAX_THROTTLES];    // Current actual speed (float for smooth ramping)
	bool braking_[MOMENTUM_MAX_THROTTLES];         // Brake active flag
	unsigned long lastUpdate_[MOMENTUM_MAX_THROTTLES]; // Last update time
	
	// Direction change safety: prevents direction change while train is moving.
	// When direction is toggled while train has momentum, we automatically brake to stop.
	// If direction is toggled again before stopping, it cancels the pending change.
	bool pendingDirectionChange_[MOMENTUM_MAX_THROTTLES]; // Direction change requested while moving?
	Direction pendingDirection_[MOMENTUM_MAX_THROTTLES];   // Target direction for pending change
	Direction originalDirection_[MOMENTUM_MAX_THROTTLES];  // Direction train is actually moving (for cancel)
	
	// Event helpers
	void triggerBrakeSound(int throttle, bool enable);
	
	// References to other controllers
	ThrottleManager* throttleMgr_;
	SoundController* soundCtrl_;
	
    // Timing
    static constexpr unsigned long UPDATE_INTERVAL_MS = 500; // Update every 500ms (2Hz) - reduces network traffic
};
