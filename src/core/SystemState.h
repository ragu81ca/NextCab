#pragma once
#include <Arduino.h>
#include "input/InputManager.h"

// Unified system state enum representing the complete application state machine
// Replaces separate ssidConnectionState and witConnectionState variables
enum class SystemState : uint8_t {
    Boot,                    // Initial startup state
    WifiScanning,           // Scanning for available WiFi networks
    WifiSelection,          // User selecting WiFi network from list
    WifiPasswordEntry,      // User entering WiFi password
    WifiConnecting,         // Attempting WiFi connection
    WifiConnected,          // WiFi connected, ready to scan for WiThrottle servers
    ServerScanning,         // Browsing for WiThrottle servers via mDNS
    ServerSelection,        // User selecting WiThrottle server from discovered list
    ServerManualEntry,      // User manually entering server IP:port
    ServerConnecting,       // Attempting WiThrottle server connection
    Operating               // Connected and operational (can control locos, functions, etc.)
};

// SystemStateManager: Manages system state transitions and coordinates with InputManager
// Provides centralized state management to replace fragmented connection state tracking
class SystemStateManager {
public:
    SystemStateManager(InputManager &inputMgr) : inputManager_(inputMgr), state_(SystemState::Boot) {}
    
    // Get current system state
    SystemState getState() const { return state_; }
    
    // Set system state and update InputManager mode accordingly
    void setState(SystemState newState) {
        if (state_ == newState) return; // No change
        
        state_ = newState;
        
        // Map system state to appropriate InputMode
        InputMode targetMode = mapStateToInputMode(newState);
        inputManager_.setMode(targetMode);
    }
    
    // Force state change with InputManager mode update (always calls onEnter/onExit)
    void forceState(SystemState newState) {
        state_ = newState;
        InputMode targetMode = mapStateToInputMode(newState);
        inputManager_.forceMode(targetMode);
    }
    
    // Convenience checks for common state categories
    bool isConnectedToWifi() const {
        return state_ == SystemState::WifiConnected ||
               state_ == SystemState::ServerScanning ||
               state_ == SystemState::ServerSelection ||
               state_ == SystemState::ServerManualEntry ||
               state_ == SystemState::ServerConnecting ||
               state_ == SystemState::Operating;
    }
    
    bool isConnectedToServer() const {
        return state_ == SystemState::Operating;
    }
    
    bool isInPreConnectionSequence() const {
        // States where inactivity timeout should apply
        return state_ != SystemState::Operating;
    }
    
    bool isOperating() const {
        return state_ == SystemState::Operating;
    }
    
private:
    InputManager &inputManager_;
    SystemState state_;
    
    // Map system state to corresponding InputMode for input handling
    InputMode mapStateToInputMode(SystemState state) const {
        switch (state) {
            case SystemState::WifiSelection:
                return InputMode::WifiSelection;
            case SystemState::WifiPasswordEntry:
                return InputMode::PasswordEntry;
            case SystemState::ServerSelection:
            case SystemState::ServerManualEntry:
                return InputMode::WiThrottleServerSelection;
            case SystemState::Operating:
                return InputMode::Operation;
            default:
                // Boot, scanning, connecting states don't need specific input modes
                // Keep whatever mode is active (or default to Operation)
                return InputMode::Operation;
        }
    }
};

// Global system state manager instance (defined in WiTcontroller.ino)
extern SystemStateManager systemStateManager;
