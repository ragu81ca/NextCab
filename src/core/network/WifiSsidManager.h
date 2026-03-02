#pragma once
#include <Arduino.h>
#include <functional>

// Forward declare for constants expected in existing code (Phase 1 keeps macros elsewhere)
// We reuse existing CONNECTION_STATE_* macros for compatibility.

class WifiSsidManager {
public:
    enum class State { Disconnected, Scanning, SelectionRequired, PasswordEntry, Selected, Connecting, Connected };

    struct FoundSsid {
        String ssid; long rssi; bool open;
    };

    using StateCallback = std::function<void(State)>;
    using ListChangedCallback = std::function<void()>;

    void begin(const String* configuredSsids,
               const String* configuredPasswords,
               int configuredCount,
               StateCallback onStateChange = nullptr,
               ListChangedCallback onListChanged = nullptr);

    void loop();

    // Selection
    void selectConfigured(int index);
    void selectFound(int index);

    // Password entry
    void startPasswordEntry();

    void attemptConnect();

    // Accessors
    int foundCount() const { return _foundSsidsCountExtern(); }
    FoundSsid getFound(int i) const;
    State state() const { return currentState; }
    const String& currentSsid() const { return selectedSsidStr; }
    const String& currentPassword() const { return selectedSsidPasswordStr; }
    void setPassword(const String &pw) { selectedSsidPasswordStr = pw; }
    int configuredCount() const { return maxSsids; }
    const String& configuredSsid(int i) const { return ssids[i]; }
    // Turnout/route prefixes intentionally not managed here (remain global/config-driven).

    // Network scanning
    void browseSsids(); // Trigger SSID scan and display results
    void showConfiguredList(); // Display configured network list

private:
    void connectSelectedInternal();
    void getSsidPasswordAndMetadataForFound();

    void changeState(State s);

    // Config references
    const String* ssids = nullptr;
    const String* passwords = nullptr;
    int maxSsids = 0;

    // State copied from sketch
    String selectedSsidStr;
    String selectedSsidPasswordStr;

    // Found SSID storage now relies on legacy global arrays (foundSsids, foundSsidRssis, foundSsidsOpen, foundSsidsCount)

    State currentState = State::Disconnected;
    int selectionSource = 0; // mirror ssidSelectionSource

    double startWaitForSelection = 0;

    StateCallback onStateChangeCb;
    ListChangedCallback onListChangedCb;

    // Scan result processing
    void processScanResults(int num);

    // Access to legacy global foundSsidsCount
    static int _foundSsidsCountExtern();
};
