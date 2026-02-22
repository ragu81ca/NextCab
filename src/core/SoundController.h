// SoundController.h - Event-driven diesel locomotive sound management
#pragma once
#include <Arduino.h>

// Forward declarations
class ThrottleManager;

// Sound events that can trigger sound functions
enum class SoundEvent {
    ThrottleNotchUp,
    ThrottleNotchDown, 
    BrakeApplied,
    BrakeReleased,
    SpeedChanged,
    DirectionChanged
};

// Sound function configuration for diesel locomotive sound simulation
struct SoundConfig {
    bool enabled = true;
    uint8_t throttleUpFunction = 6;      // F6 - diesel notch up sound
    uint8_t throttleDownFunction = 7;    // F7 - diesel notch down sound
    uint8_t brakeFunction = 9;           // F9 - brake sound
    
    // Pulse timing: ON duration for momentary function press
    // 300ms gives Digitrax and other decoders reliable time to register
    // (150ms was too short — DCC packets may only repeat 1-2 times)
    unsigned long pulseduration = 300;
    
    // Cooldown: minimum time between triggering same function again
    // This prevents re-triggering while a pulse is conceptually "active"
    // Should be >= pulseduration + DCC round-trip margin (~100ms)
    unsigned long cooldownPeriod = 100;
    
    // Note: We always bypass roster latching settings for sound simulation
    // Sound functions must be precisely timed ON/OFF regardless of roster config
};

class SoundController {
public:
    SoundController();
    
    // Initialize with reference to throttle manager
    void begin(ThrottleManager* throttleMgr);
    
    // Called from main loop to handle non-blocking function pulses
    void update();
    
    // Event handlers - called by other controllers when state changes
    void onBrakeStateChange(int throttle, bool braking);
    void onSpeedChange(int throttle, int oldSpeed, int newSpeed);
    void onActualSpeedUpdate(int throttle, int actualSpeed);
    void onDirectionChange(int throttle);
    
    // Configuration
    void setConfig(const SoundConfig& config) { config_ = config; }
    const SoundConfig& getConfig() const { return config_; }
    
    // Enable/disable sound system
    void setEnabled(bool enabled) { config_.enabled = enabled; }
    bool isEnabled() const { return config_.enabled; }
    
    // Check if sound is currently transitioning notches for a throttle
    // Used by MomentumController to delay actual speed changes until sound leads
    bool isNotching(int throttle) const;
    
    // Check if any throttle is notching
    bool isAnyNotching() const;
    
private:
    static constexpr int SOUND_MAX_THROTTLES = 6;
    
    // Configuration
    SoundConfig config_;
    
    // Per-throttle, per-function state for non-blocking function pulses
    bool functionPulseActive_[SOUND_MAX_THROTTLES][32];  // Support F0-F31
    unsigned long functionPulseStart_[SOUND_MAX_THROTTLES][32];
    unsigned long lastFunctionTime_[SOUND_MAX_THROTTLES][32];
    
    // Internal notch simulation (1-8, diesel locomotive sound effects only)
    // Steam locomotives would require completely different sound patterns
    int currentNotch_[SOUND_MAX_THROTTLES];
    int targetNotch_[SOUND_MAX_THROTTLES];
    unsigned long lastNotchTime_[SOUND_MAX_THROTTLES];
    
    // Speed tracking for effort-based notch calculation (prime mover overshoot)
    int targetSpeed_[SOUND_MAX_THROTTLES];
    int actualSpeed_[SOUND_MAX_THROTTLES];
    
    // Idle recovery: extra throttle-down pulses to ensure decoder reaches idle
    // Digitrax decoders can miss individual DCC function packets, so we send
    // redundant F7 pulses after the normal notch-down sequence completes
    int idleFlushRemaining_[SOUND_MAX_THROTTLES];
    static constexpr int IDLE_FLUSH_COUNT = 3;
    
    // Time between notch transitions (ms)
    // Formula: pulseDuration + OFF-to-ON gap for decoder to register
    // 300ms pulse + 400ms gap = 700ms total - reliable for Digitrax + WiFi latency
    // Going from idle to notch 8 takes 7 transitions × 700ms = 4.9 seconds
    static constexpr unsigned long NOTCH_TRANSITION_MS = 700;
    
    // Reference to throttle manager for function calls
    ThrottleManager* throttleMgr_;
    
    // Internal methods
    void triggerFunction(int throttle, uint8_t funcNum, const char* reason);
    void triggerFunctionImmediate(int throttle, uint8_t funcNum, bool state);
    bool canTriggerFunction(int throttle, uint8_t funcNum);
    
    // Internal notch simulation for sound effects only
    int calculateNotchFromSpeed(int speed) const;
    int calculateEffortNotch(int targetSpeed, int actualSpeed) const;
    void recalculateTargetNotch(int throttle);
    void updateNotchSounds(int throttle, unsigned long now);
    
    // Debug helpers
    void debugPrint(int throttle, uint8_t funcNum, const char* action, const char* reason = "");
};