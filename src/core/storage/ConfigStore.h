// ConfigStore.h — LittleFS + JSON persistent storage layer.
//
// Provides read/write access to runtime configuration stored as JSON
// files on the ESP32's flash filesystem.  Falls back gracefully when
// LittleFS is unavailable or files are missing/corrupt.
//
// Current file layout:
//   /config/wifi.json     – known SSID+password pairs
//   /config/servers.json  – WiThrottle server settings (stub)
#pragma once

#include <Arduino.h>
#include <vector>

/// WiFi network credentials (SSID + password).
struct WifiCredential {
    String ssid;
    String password;
};

/// Per-server configuration (host, port, turnout/route prefixes, last-used locos).
struct ServerConfig {
    String host;
    int port = 0;
    String turnoutPrefix;
    String routePrefix;
    // Per-throttle loco addresses in WiThrottle format ("L1234" or "S3").
    // throttleLocos[i] = list of locos acquired on throttle i.
    std::vector<std::vector<String>> throttleLocos;
};

class ConfigStore {
public:
    /// Mount LittleFS (format on first boot).  Safe to call more than once.
    void begin();

    /// Erase all stored configuration and remount.
    bool factoryReset();

    // ── WiFi credentials ───────────────────────────────────────────────
    std::vector<WifiCredential> loadWifiNetworks();
    void saveWifiNetwork(const String& ssid, const String& password);
    void removeWifiNetwork(const String& ssid);
    String findPasswordForSsid(const String& ssid);

    // ── Server configs (stub — full implementation later) ──────────────
    std::vector<ServerConfig> loadServers();
    void saveServer(const ServerConfig& server);
    ServerConfig findServerConfig(const String& host, int port);

    bool isMounted() const { return mounted_; }

private:
    bool mounted_ = false;

    String readFile(const String& path);
    bool   writeFile(const String& path, const String& content);
    bool   ensureDir(const String& path);
};
