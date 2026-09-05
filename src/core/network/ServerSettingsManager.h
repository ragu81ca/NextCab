// ServerSettingsManager.h — Settings that belong to a WiThrottle server rather
// than to the device or the locos.
//
// Settings are keyed by server name, loaded on connect and saved when changed.
// A value the user set always outranks one implied by the server type.
#pragma once
#include <Arduino.h>

class ConfigStore;
class ServerDataStore;
class WiThrottleConnectionManager;

class ServerSettingsManager {
public:
    /// Call once during setup() after the dependencies are constructed.
    void begin(ConfigStore *configStore, ServerDataStore *dataStore,
               WiThrottleConnectionManager *connMgr);

    // ── Server type (e.g. "DCC-EX", "JMRI") ─────────────────────────────
    const String &serverType() const { return serverType_; }
    /// Records the type and applies any defaults it implies.
    void setServerType(const String &type);

    // ── Lifecycle ───────────────────────────────────────────────────────
    /// Load stored settings for the connected server. Safe to call repeatedly.
    void restoreForCurrentServer();
    /// Forget per-server state so the next server starts clean.
    void reset();

    // ── Persistence ─────────────────────────────────────────────────────
    void savePrefixes(const String &turnoutPrefix, const String &routePrefix);

    /// Defaults for a DCC-EX command station. Also reachable directly for the
    /// AP-mode heuristic, which infers the server type from the SSID before the
    /// server has identified itself.
    void applyDccExDefaults();

private:
    void applyServerTypeDefaults();

    ConfigStore                 *configStore_ = nullptr;
    ServerDataStore             *dataStore_   = nullptr;
    WiThrottleConnectionManager *connMgr_     = nullptr;

    String serverType_;
    bool   restoredForCurrentServer_ = false;
};
