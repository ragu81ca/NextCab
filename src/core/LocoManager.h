// LocoManager.h — Loco acquisition, release, consist management
// Owns the protocol-level loco operations that were free functions in WiTcontroller.ino
#pragma once
#include <Arduino.h>
#include <WiThrottleProtocol.h>

class ThrottleManager;
class ServerDataStore;
class Renderer;
class UIState;

class LocoManager {
public:
    LocoManager() = default;

    /// Call once during setup() after all dependencies are constructed.
    void begin(WiThrottleProtocol *proto, ThrottleManager *throttle,
               ServerDataStore *dataStore, Renderer *renderer, UIState *uiState);

    // ── Acquire / Release ───────────────────────────────────────────────
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

private:
    WiThrottleProtocol *proto_    = nullptr;
    ThrottleManager    *throttle_ = nullptr;
    ServerDataStore    *dataStore_ = nullptr;
    Renderer           *renderer_ = nullptr;
    UIState            *uiState_  = nullptr;

    bool   dropBeforeAcquire_ = true;   // overwritten by begin() from config
    String serverType_;
};
