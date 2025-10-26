// MomentumController.h - Simulated momentum/inertia for throttle control
#pragma once
#include <Arduino.h>

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
    
    // Initialize with WiThrottle protocol reference
    void begin();
    
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
    
    // Momentum level control
    void setMomentumLevel(MomentumLevel level);
    MomentumLevel getMomentumLevel() const { return momentumLevel_; }
    void cycleMomentumLevel(); // Off -> Low -> Medium -> High -> Off
    
    // Check if momentum is currently active
    bool isActive() const { return momentumLevel_ != MomentumLevel::Off; }
    
    // Brake control (for future stage 5)
    void setBraking(int throttle, bool braking);
    bool isBraking(int throttle) const;
    
private:
    // Calculate rate of change based on momentum level
    float getAccelRate() const;
    float getDecelRate() const;
    float getBrakeRate() const;
    
	// Logarithmic curve for natural feel
	float applyAccelCurve(float delta, float distance) const;
	float applyDecelCurve(float delta, float distance) const;
	
	// Use WIT_MAX_THROTTLES to avoid conflict with MAX_THROTTLES macro
	static constexpr int MOMENTUM_MAX_THROTTLES = 6;
	
	MomentumLevel momentumLevel_;
	
	// Per-throttle state
	int targetSpeed_[MOMENTUM_MAX_THROTTLES];      // What user set (0-126)
	float actualSpeed_[MOMENTUM_MAX_THROTTLES];    // Current actual speed (float for smooth ramping)
	bool braking_[MOMENTUM_MAX_THROTTLES];         // Brake active flag
	unsigned long lastUpdate_[MOMENTUM_MAX_THROTTLES]; // Last update time    // Timing
    static constexpr unsigned long UPDATE_INTERVAL_MS = 500; // Update every 500ms (2Hz) - reduces network traffic
};
