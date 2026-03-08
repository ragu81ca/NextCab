// ConfigStore.h — LittleFS + JSON persistent storage layer.
//
// Provides read/write access to runtime configuration stored as JSON
// files on the ESP32's flash filesystem.  Falls back gracefully when
// LittleFS is unavailable or files are missing/corrupt.
//
// File layout (three tiers):
//   /config/device.json      – device-wide runtime settings
//   /config/wifi.json        – device-wide WiFi credentials
//   /config/servers.json     – per-WiThrottle-server settings
//   /config/locos/<addr>.json – per-loco settings (one file per DCC address)
#pragma once

#include <Arduino.h>
#include <vector>

// ── Tier 1: Device-level settings ──────────────────────────────────────
// Compile-time #defines provide the defaults; a stored device.json
// overrides them at runtime.  Missing fields fall through to defaults.
struct DeviceConfig {
    int   speedStep              = 1;      // SPEED_STEP
    int   speedStepMultiplier    = 3;      // SPEED_STEP_MULTIPLIER
    bool  displaySpeedAsPercent  = true;   // DISPLAY_SPEED_AS_PERCENT
    bool  encoderCwIsIncrease    = true;   // ENCODER_ROTATION_CLOCKWISE_IS_INCREASE_SPEED
    bool  heartbeatEnabled       = true;   // HEARTBEAT_ENABLED
    bool  restoreAcquiredLocos   = true;   // RESTORE_ACQUIRED_LOCOS
};

/// WiFi network credentials (SSID + password).  (Tier 1 — device-level)
struct WifiCredential {
    String ssid;
    String password;
};

// ── Tier 2: Per-server settings ────────────────────────────────────────
// Keyed by server name (mDNS hostname / display name), which is stable
// even when the server's DHCP address changes.  host+port are cached
// as last-known connection info.
struct ServerConfig {
    String name;            // primary key — mDNS hostname or display name
    String host;            // last-known IP (informational, not used as key)
    int port = 0;           // last-known port
    String turnoutPrefix;
    String routePrefix;
    // Per-throttle loco addresses in WiThrottle format ("L1234" or "S3").
    // throttleLocos[i] = list of locos acquired on throttle i.
    std::vector<std::vector<String>> throttleLocos;
};

// ── Tier 3: Per-loco settings (keyed by DCC address) ───────────────────
// Address-global — a decoder's sound functions are the same on any server.
struct LocoConfig {
    String address;               // WiThrottle format, e.g. "L1234", "S3"
    bool   soundThrottle  = false;
    int    funcThrottleUp   = -1; // -1 = not configured
    int    funcThrottleDown = -1;
    int    funcBrake        = -1;
};

class ConfigStore {
public:
    /// Mount LittleFS (format on first boot).  Safe to call more than once.
    void begin();

    /// Erase all stored configuration and remount.
    bool factoryReset();

    // ── Device config (Tier 1) ─────────────────────────────────────────
    DeviceConfig loadDeviceConfig();
    void saveDeviceConfig(const DeviceConfig& cfg);

    // ── WiFi credentials (Tier 1) ──────────────────────────────────────
    std::vector<WifiCredential> loadWifiNetworks();
    void saveWifiNetwork(const String& ssid, const String& password);
    void removeWifiNetwork(const String& ssid);
    String findPasswordForSsid(const String& ssid);

    // ── Server configs (Tier 2) ────────────────────────────────────────
    std::vector<ServerConfig> loadServers();
    void saveServer(const ServerConfig& server);
    ServerConfig findServerConfig(const String& name);

    // ── Loco configs (Tier 3) ──────────────────────────────────────────
    LocoConfig loadLocoConfig(const String& address);
    void saveLocoConfig(const LocoConfig& cfg);
    std::vector<LocoConfig> loadAllLocoConfigs();

    bool isMounted() const { return mounted_; }

private:
    bool mounted_ = false;

    String readFile(const String& path);
    bool   writeFile(const String& path, const String& content);
    bool   ensureDir(const String& path);
};
