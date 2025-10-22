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

    // Password entry (Phase 1: thin wrappers around legacy globals that still live here)
    void startPasswordEntry();
    void appendPasswordChar(char c);
    void backspacePassword();

    void attemptConnect();

    // Accessors
    int foundCount() const { return _foundSsidsCountExtern(); }
    FoundSsid getFound(int i) const;
    State state() const { return currentState; }
    const String& currentSsid() const { return selectedSsidStr; }
    const String& currentPassword() const { return selectedSsidPasswordStr; }
    // Turnout/route prefixes intentionally not managed here (remain global/config-driven).

    // Network scanning
    void browseSsids(); // Trigger SSID scan and display results

    // Legacy compatibility helpers
    int rawConnectionStateMacro() const; // map enum to CONNECTION_STATE_* macro ints

private:
    void showConfiguredList();
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

    // Password entry
    String passwordEntered;
    bool passwordChanged = true;

    StateCallback onStateChangeCb;
    ListChangedCallback onListChangedCb;

    // Access to legacy global foundSsidsCount
    static int _foundSsidsCountExtern();
};
