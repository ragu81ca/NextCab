#pragma once
#include <Arduino.h>
#include <functional>
#include <vector>

// Forward declarations
class SystemStateManager;
class Renderer;
class ConfigStore;

class WifiSsidManager {
public:
    static constexpr int kMaxFound = 60;  // must be a multiple of ssidItemsPerPage

    enum class State { Disconnected, Scanning, SelectionRequired, PasswordEntry, Selected, Connecting, Connected };

    struct FoundSsid {
        String ssid; long rssi; bool open;
    };

    using StateCallback = std::function<void(State)>;
    using ListChangedCallback = std::function<void()>;

    void begin(SystemStateManager& stateManager,
               Renderer& renderer,
               ConfigStore& configStore,
               StateCallback onStateChange = nullptr,
               ListChangedCallback onListChanged = nullptr);

    void loop();

    // Selection
    void selectConfigured(int index);
    void selectFound(int index);

    // Password entry
    void startPasswordEntry();

    void attemptConnect();

    // Found-SSID accessors (data owned by this class)
    int foundCount() const { return foundSsidsCount_; }
    const String& foundSsid(int i) const { return foundSsids_[i]; }
    long foundRssi(int i) const { return foundSsidRssis_[i]; }
    bool foundIsOpen(int i) const { return foundSsidsOpen_[i]; }
    FoundSsid getFound(int i) const;

    // State accessors
    State state() const { return currentState; }
    const String& currentSsid() const { return selectedSsidStr; }
    const String& currentPassword() const { return selectedSsidPasswordStr; }
    void setPassword(const String &pw) { selectedSsidPasswordStr = pw; }

    // Stored credential count
    int configuredCount() const { return storedCount_; }
    const String& configuredSsid(int i) const { return storedSsids_[i]; }

    // Network scanning
    void browseSsids();
    void showConfiguredList();

    /// Compute WiFi signal strength (0-3) from RSSI
    static int rssiToStrength(long rssi);

private:
    void connectSelectedInternal();
    void getSsidPasswordAndMetadataForFound();
    void processScanResults(int num);
    void changeState(State s);
    void loadStoredCredentials();

    // Injected dependencies (set by begin())
    SystemStateManager* stateManager_ = nullptr;
    Renderer* renderer_ = nullptr;
    ConfigStore* configStore_ = nullptr;

    // Stored credential list (loaded from ConfigStore)
    std::vector<String> storedSsids_;
    std::vector<String> storedPasswords_;
    int storedCount_ = 0;

    // State
    String selectedSsidStr;
    String selectedSsidPasswordStr;
    State currentState = State::Disconnected;

    // Found-SSID storage (owned by this class)
    String foundSsids_[kMaxFound];
    long   foundSsidRssis_[kMaxFound];
    bool   foundSsidsOpen_[kMaxFound];
    int    foundSsidsCount_ = 0;

    StateCallback onStateChangeCb;
    ListChangedCallback onListChangedCb;
};
