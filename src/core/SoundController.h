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
    unsigned long pulseduration = 300;   // ms for momentary pulses (notch sounds)
    unsigned long cooldownPeriod = 700;  // ms between function commands
    
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
    void onDirectionChange(int throttle);
    
    // Configuration
    void setConfig(const SoundConfig& config) { config_ = config; }
    const SoundConfig& getConfig() const { return config_; }
    
    // Enable/disable sound system
    void setEnabled(bool enabled) { config_.enabled = enabled; }
    bool isEnabled() const { return config_.enabled; }
    
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
    static constexpr unsigned long NOTCH_TRANSITION_MS = 1000;
    
    // Reference to throttle manager for function calls
    ThrottleManager* throttleMgr_;
    
    // Internal methods
    void triggerFunction(int throttle, uint8_t funcNum, const char* reason);
    void triggerFunctionImmediate(int throttle, uint8_t funcNum, bool state);
    bool canTriggerFunction(int throttle, uint8_t funcNum);
    
    // Internal notch simulation for sound effects only
    int calculateNotchFromSpeed(int speed) const;
    void updateNotchSounds(int throttle, unsigned long now);
    
    // Debug helpers
    void debugPrint(int throttle, uint8_t funcNum, const char* action, const char* reason = "");
};