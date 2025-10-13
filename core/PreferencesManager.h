// PreferencesManager.h
// Encapsulates non-volatile storage (Preferences) logic for WiTcontroller.
#pragma once

#include <Arduino.h>
#include <Preferences.h>
#include "WiThrottleProtocol.h"

// Forward declaration to avoid including large headers unnecessarily.

class PreferencesManager {
public:
    PreferencesManager();

    // Initialize storage, creating initial keys if required.
    void begin(bool forceClear = false);

    // Restore previously acquired locomotives (if feature enabled).
    void restoreLocos(WiThrottleProtocol &proto, char throttleIndexCharStart = '0');

    // Persist currently acquired locomotives for all active throttles.
    void saveLocos(WiThrottleProtocol &proto, int maxThrottles);

    // Clear (reinitialize) persistent storage region.
    void clear();

    bool isInitialized() const { return nvsInit; }
    bool locosRestored() const { return preferencesRead; }

private:
    void open(const char *ns, bool readOnly);
    void close();

    Preferences prefs;
    bool nvsInit;
    bool preferencesRead; // whether locos restored
};
