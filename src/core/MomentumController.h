// MomentumController.h - Simulated momentum/inertia for throttle control
#pragma once
#include <Arduino.h>
#include <WiThrottleProtocol.h> // For Direction enum

// Forward declarations
class ThrottleManager;
class SoundController;
enum class LocoType : uint8_t;

// Momentum levels (Off, Low, Medium, High)
enum class MomentumLevel {
    Off = 0,
    Low = 1,
    Medium = 2,
    High = 3
};

/// Function pointer type for clock source — defaults to millis(),
/// injectable for unit testing.
using ClockFn = unsigned long(*)();

class MomentumController {
public:
    /// Construct with an optional clock source (defaults to millis()).
    explicit MomentumController(ClockFn clock = nullptr);
    
    // Initialize with references
    void begin(ThrottleManager* throttleMgr, SoundController* soundCtrl = nullptr);
    
    // Called from main loop - integrates actual speed when momentum is active
    void update();
    
    // Commanded speed (0-126) — the balancing speed the driver has asked for.
    // With momentum off this is the loco speed directly; with momentum on the
    // train works toward it under the physics model.
    void setTargetSpeed(int throttle, int speed);
    
    // Get actual speed for a throttle (what's sent to loco after momentum)
    int getActualSpeed(int throttle) const;
    
    // Get commanded speed for a throttle (what user set)
    int getTargetSpeed(int throttle) const;
    
    // Commanded speed expressed as a power percentage, for the simulator display
    int getPowerPercent(int throttle) const;
    
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
    
    // Brake control — accelerates deceleration when coasting (throttle at 0)
    void setBraking(int throttle, bool braking);
    bool isBraking(int throttle) const;
    
    // Dynamic braking — hold-to-slow while throttle > 0.
    // Continuously decelerates the actual speed while maintaining the displayed
    // target. On release, momentum carries speed back up to the set throttle.
    // Deceleration rate and minimum effective speed vary by locomotive type.
    void setDynamicBraking(int throttle, bool active);
    bool isDynamicBraking(int throttle) const;
    
    // Locomotive type — determines dynamic brake characteristics
    void setLocoType(int throttle, LocoType type);
    LocoType getLocoType(int throttle) const;
    
    // Consist size affects acceleration (more locos = more tractive effort)
    void setConsistSize(int throttle, int locoCount);
    int getConsistSize(int throttle) const;
    
    /// Davis-equation train resistance at a given speed step (normalised so
    /// resistance at step 126 == full tractive effort at balancing speed).
    static float resistance(float speedSteps);
    
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
    
	/// Per-loco-type brake profile — determines dynamic brake feel.
	struct BrakeProfile {
		float   decelRate;      // speed steps per second to subtract during service braking
		int     minSpeed;       // stop braking effect below this speed
	};
	
	// Brake profiles indexed by LocoType (Diesel=0, Steam=1, Electric=2)
	static constexpr BrakeProfile BRAKE_PROFILES[] = {
		{ 4.0f, 20 },   // Diesel:   moderate — rheostatic dynamic brake grids
		{ 3.0f, 20 },   // Steam:    gentle — air brakes only, no dynamic braking
		{ 5.0f, 10 },   // Electric: strong — regenerative braking, effective at low speeds
	};
	
	const BrakeProfile& getBrakeProfile(int throttle) const;

private:
    // Per-momentum-level rates: acceleration sets the train's mass,
    // braking is the rate the air brake sheds speed.
    float getAccelRate(int throttle) const;
    float getBrakeRate(int throttle) const;
    
	// ── Physics integration ─────────────────────────────────────────────
	// Newton's second law: dv/dt = (tractive effort - resistance - braking) / mass
	float tractiveEffort(int throttle, float speed) const;
	float trainMass(int throttle) const;
	void  updatePhysics(int throttle, float dtSeconds);
	
	// Use WIT_MAX_THROTTLES to avoid conflict with MAX_THROTTLES macro
	static constexpr int MOMENTUM_MAX_THROTTLES = 6;
	
	// Per-throttle momentum level
	MomentumLevel momentumLevel_[MOMENTUM_MAX_THROTTLES];
	
	// Per-throttle state
	int targetSpeed_[MOMENTUM_MAX_THROTTLES];      // Commanded balancing speed (0-126)
	float actualSpeed_[MOMENTUM_MAX_THROTTLES];    // Current actual speed (float for smooth integration)
	bool braking_[MOMENTUM_MAX_THROTTLES];         // Brake active flag (throttle=0, accelerate stop)
	bool dynamicBraking_[MOMENTUM_MAX_THROTTLES];   // Dynamic brake active flag (throttle>0, hold-to-slow)
	int consistSize_[MOMENTUM_MAX_THROTTLES];        // Number of locos in consist (for accel scaling)
	LocoType locoType_[MOMENTUM_MAX_THROTTLES];      // Locomotive type (for brake profile selection)
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
	
	// Clock source — injectable for testing
	ClockFn clock_;

    // Timing
    static constexpr unsigned long UPDATE_INTERVAL_MS = 100; // Update every 100ms (10Hz) - smooth ramp with elapsed-time delta
};
