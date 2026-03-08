// LocoManager.h — Loco acquisition, release, consist management, persistence
// Owns the protocol-level loco operations and loco save/restore.
// Single choke-point for all acquire/release — fires onLocoChanged callbacks.
#pragma once
#include <Arduino.h>
#include <WiThrottleProtocol.h>
#include "storage/ConfigStore.h"   // LocoConfig struct
#include "../../static.h"
#include <vector>
#include <functional>

class ThrottleManager;
class ServerDataStore;
class Renderer;
class UIState;
class WiThrottleConnectionManager;

/// What kind of change occurred to a throttle's loco roster.
enum class LocoChangeType {
    Acquired,       ///< A loco was added (roster select, manual entry, steal, restore)
    Released,       ///< A loco was removed
};

/// Event fired whenever a throttle's loco roster changes.
struct LocoChangeEvent {
    int            throttleIndex;
    LocoChangeType type;
    String         address;   ///< The specific loco address
};

/// Callback signature for loco change observers.
using LocoChangeCallback = std::function<void(const LocoChangeEvent&)>;

class LocoManager {
public:
    LocoManager() = default;

    /// Call once during setup() after all dependencies are constructed.
    void begin(WiThrottleProtocol *proto, ThrottleManager *throttle,
               ServerDataStore *dataStore, Renderer *renderer, UIState *uiState,
               ConfigStore *configStore, WiThrottleConnectionManager *connMgr);

    // ── Acquire ─────────────────────────────────────────────────────────
    /// Add a loco to a throttle.  Handles drop-before-acquire, requests
    /// direction/speed from server, resets function states, and fires
    /// loco-changed callbacks.
    void acquireLoco(int throttleIndex, const String &loco);

    // ── Release ─────────────────────────────────────────────────────────
    void releaseAllLocos(int throttleIndex);
    void releaseOneLoco(int throttleIndex, const String &loco);
    void releaseOneLocoByIndex(int throttleIndex, int index);

    // ── Steal ───────────────────────────────────────────────────────────
    void stealLoco(int throttleIndex, const String &address);
    void stealLoco(char multiThrottle, const String &address);

    // ── Consist facing ──────────────────────────────────────────────────
    void toggleLocoFacing(int throttleIndex, const String &loco);

    // ── Display helpers ─────────────────────────────────────────────────
    String getDisplayLocoString(int throttleIndex, int index);
    String getLocoWithLength(const String &loco);

    // ── Drop-before-acquire toggle ──────────────────────────────────────
    bool  dropBeforeAcquire() const { return dropBeforeAcquire_; }
    void  setDropBeforeAcquire(bool v) { dropBeforeAcquire_ = v; }
    void  toggleDropBeforeAcquire();

    // ── Server type (DCC-EX, etc.) ──────────────────────────────────────
    const String &serverType() const { return serverType_; }
    void setServerType(const String &t) { serverType_ = t; }

    // ── Loco persistence (save/restore acquired locos per server) ───────
    void restoreLocos();
    void saveLocos();
    void resetRestoreGuard();

    // ── Loco-changed observer ───────────────────────────────────────────
    /// Register a callback to be invoked whenever a throttle's loco
    /// roster changes (acquire, release, steal, restore).
    void onLocoChanged(LocoChangeCallback cb);

    // ── Sound throttle config cache ─────────────────────────────────────
    /// Get the cached sound-enabled loco configs for a throttle.
    const std::vector<LocoConfig>& soundConfigs(int throttleIndex) const;

    /// Convenience: true if any loco on this throttle has sound throttle enabled.
    bool hasSoundThrottle(int throttleIndex) const;

private:
    WiThrottleProtocol          *proto_    = nullptr;
    ThrottleManager             *throttle_ = nullptr;
    ServerDataStore             *dataStore_ = nullptr;
    Renderer                    *renderer_ = nullptr;
    UIState                     *uiState_  = nullptr;
    ConfigStore                 *configStore_ = nullptr;
    WiThrottleConnectionManager *connMgr_  = nullptr;

    bool   dropBeforeAcquire_ = true;
    String serverType_;
    bool   locosRestoredForCurrentServer_ = false;

    // ── Callback infrastructure ─────────────────────────────────────────
    std::vector<LocoChangeCallback> locoChangeCallbacks_;
    void notifyLocoChanged(const LocoChangeEvent &event);

    // ── Sound config cache (self-registered as a loco-changed listener) ─
    void handleLocoChanged(const LocoChangeEvent &event);
    std::vector<LocoConfig> soundConfigs_[WIT_MAX_THROTTLES];
    static const std::vector<LocoConfig> emptyConfigs_;
};
