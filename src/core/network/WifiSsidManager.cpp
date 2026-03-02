#include "WifiSsidManager.h"
#include "WiThrottleConnectionManager.h"
#include <WiFi.h>
#include <ESPmDNS.h>
#include "../../../static.h"          // macro constants
#include "../../../WiTcontroller.h"   // legacy glue (menuIsShowing etc.)
#include "../Renderer.h"
#include "../protocol/WiThrottleDelegate.h" // debug_print macros
#include "../SystemState.h"
#include "../ui/TitleScreen.h"
#include "../ui/WaitScreen.h"

// Remaining globals that are truly app-wide and written by many consumers
extern int ssidSelectionSource;
extern String turnoutPrefix;
extern String routePrefix;

void WifiSsidManager::begin(const String* configuredSsids,
               const String* configuredPasswords,
               int configuredCount,
               const String* turnoutPrefixes,
               const String* routePrefixes,
               SystemStateManager& stateManager,
               Renderer& renderer,
               StateCallback onStateChange,
               ListChangedCallback onListChanged) {
    ssids_    = configuredSsids;
    passwords_ = configuredPasswords;
    maxSsids  = configuredCount;
    turnoutPrefixes_ = turnoutPrefixes;
    routePrefixes_   = routePrefixes;
    stateManager_    = &stateManager;
    renderer_        = &renderer;
    onStateChangeCb  = onStateChange;
    onListChangedCb  = onListChanged;
    changeState(State::Disconnected);
}

void WifiSsidManager::changeState(State s) {
    if (currentState != s) {
        currentState = s;
        if (onStateChangeCb) onStateChangeCb(s);
    }
}

WifiSsidManager::FoundSsid WifiSsidManager::getFound(int i) const {
    FoundSsid fs{"",0,false};
    if (i>=0 && i<foundSsidsCount_) {
        fs.ssid = foundSsids_[i];
        fs.rssi = foundSsidRssis_[i];
        fs.open = foundSsidsOpen_[i];
    }
    return fs;
}

int WifiSsidManager::rssiToStrength(long rssi) {
    if (rssi >= -50) return 3;
    if (rssi >= -65) return 2;
    if (rssi >= -80) return 1;
    return 0;
}

void WifiSsidManager::loop() {
    SystemState state = stateManager_->getState();

    // These states have their own UI/input handling — don't interfere.
    if (state == SystemState::WifiPasswordEntry || state == SystemState::WifiSelection) {
        return;
    }

    // When told to connect, do it immediately — don't re-scan.
    if (state == SystemState::WifiConnecting) {
        connectSelectedInternal();
        return;
    }

    // Not connected — show selection UI or start scanning.
    if (!stateManager_->isConnectedToWifi()) {
        if (ssidSelectionSource == SSID_CONNECTION_SOURCE_LIST) {
            showConfiguredList();
        } else {
            browseSsids();
        }
    }
}

void WifiSsidManager::browseSsids() {
    debug_println("WifiSsidManager::browseSsids()");

    // Static "please wait" screen — no animation for a 2-3 second scan
    { TitleScreen ts;
      ts.setAppHeader(appName, appVersion);
      ts.addBody(MSG_BROWSING_FOR_SSIDS);
      renderer_->renderTitle(ts);
      renderer_->renderBattery(); }

    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);
    WiFi.setScanMethod(WIFI_ALL_CHANNEL_SCAN);
    WiFi.setSortMethod(WIFI_CONNECT_AP_BY_SIGNAL);

    int num = WiFi.scanNetworks(); // blocking, ~2-3 seconds
    debug_print("scanNetworks() returned: "); debug_println(num);
    processScanResults(num);
    WiFi.scanDelete();
}

void WifiSsidManager::processScanResults(int num) {
    debug_print("processScanResults: "); debug_println(num);

    foundSsidsCount_ = 0;
    if (num > 0) {
        for (int thisSsid = 0; thisSsid < num && foundSsidsCount_ < kMaxFound; ++thisSsid) {
            String ssid = WiFi.SSID(thisSsid);
            bool duplicate = false;
            for (int i = 0; i < foundSsidsCount_; i++) {
                if (foundSsids_[i] == ssid) { duplicate = true; break; }
            }
            if (!duplicate) {
                foundSsids_[foundSsidsCount_] = ssid;
                foundSsidRssis_[foundSsidsCount_] = WiFi.RSSI(thisSsid);
                foundSsidsOpen_[foundSsidsCount_] = (WiFi.encryptionType(thisSsid) == 7);
                foundSsidsCount_++;
            }
        }
        for (int i = 0; i < foundSsidsCount_; i++) { debug_println(foundSsids_[i]); }
    } else {
        debug_println("No SSIDs found in scan");
    }

    // forceState ensures onEnter() always fires on the WifiSelectionHandler,
    // even if the system state was already WifiSelection from a previous scan.
    // The handler renders the appropriate screen (list or "No SSIDs found").
    stateManager_->forceState(SystemState::WifiSelection);
}

void WifiSsidManager::showConfiguredList() {
    debug_println("WifiSsidManager::showConfiguredList()");
    if (maxSsids == 0) {
        // No configured networks — fall through to WiFi scan so the user can discover one
        debug_println("No configured SSIDs, switching to browse mode");
        ssidSelectionSource = SSID_CONNECTION_SOURCE_BROWSE;
        browseSsids();
        return;
    }
    debug_print(maxSsids); debug_println(MSG_SSIDS_LISTED);
    // Auto-connect if exactly 1 configured SSID
    if (maxSsids == 1) {
        selectedSsidStr = ssids_[0];
        selectedSsidPasswordStr = passwords_[0];
        stateManager_->setState(SystemState::WifiConnecting);
        turnoutPrefix = turnoutPrefixes_[0];
        routePrefix = routePrefixes_[0];
    } else {
        stateManager_->setState(SystemState::WifiSelection);
        // Rendering is handled by WifiSelectionHandler via ListSelectionScreen
    }
}

void WifiSsidManager::selectConfigured(int index) {
    if (index>=0 && index<maxSsids) {
        selectedSsidStr = ssids_[index];
        selectedSsidPasswordStr = passwords_[index];
        changeState(State::Selected);
        stateManager_->setState(SystemState::WifiConnecting);
    }
}

void WifiSsidManager::selectFound(int index) {
    if (index>=0 && index<foundSsidsCount_) {
        selectedSsidStr = foundSsids_[index];
        getSsidPasswordAndMetadataForFound();
        if (selectedSsidPasswordStr.length()==0) {
            changeState(State::PasswordEntry);
            stateManager_->setState(SystemState::WifiPasswordEntry);
        } else {
            changeState(State::Selected);
            stateManager_->setState(SystemState::WifiConnecting);
        }
    }
}

void WifiSsidManager::getSsidPasswordAndMetadataForFound() {
    bool found = false;
    for (int i=0;i<maxSsids;i++) {
        if (selectedSsidStr == ssids_[i]) {
            selectedSsidPasswordStr = passwords_[i];
            turnoutPrefix = turnoutPrefixes_[i];
            routePrefix = routePrefixes_[i];
            found = true; break;
        }
    }
    if (!found) {
        if (selectedSsidStr.startsWith("DCCEX_") && selectedSsidStr.length()==12) {
            selectedSsidPasswordStr = String("PASS_") + selectedSsidStr.substring(6);
            // Mirror legacy heuristic: Set default WiT server IP:Port when connecting to DCC-EX AP
            extern WiThrottleConnectionManager connectionManager;
            connectionManager.ipAndPortEntered() = "19216800400102560"; // 192.168.4.1:2560 compressed
            turnoutPrefix = DCC_EX_TURNOUT_PREFIX;
            routePrefix = DCC_EX_ROUTE_PREFIX;
            debug_println("getSsidPasswordAndMetadataForFound() Using guessed DCC-EX password & defaults");
        }
    }
}

void WifiSsidManager::startPasswordEntry() {
    changeState(State::PasswordEntry);
    stateManager_->setState(SystemState::WifiPasswordEntry);
}

void WifiSsidManager::attemptConnect() {
    if (selectedSsidStr.length()==0) return;
    changeState(State::Connecting);
    // Actual connect logic stays in sketch for Phase 1 to avoid duplication.
}

void WifiSsidManager::connectSelectedInternal() {
    // Migrate legacy connectSsid() body here (UI side-effects retained for now)
    extern bool commandsNeedLeadingCrLf;
    extern WiThrottleConnectionManager connectionManager;

    debug_println("WifiSsidManager::connectSelectedInternal() start");
    WaitScreen ws;
    ws.setAppHeader(appName, appVersion);
    ws.addBody(selectedSsidStr);
    ws.addBody("connecting...");
    renderer_->renderWait(ws);
    renderer_->renderBattery();

    double startTime = millis();
    double nowTime = startTime;
    const char *cSsid = selectedSsidStr.c_str();
    const char *cPassword = selectedSsidPasswordStr.c_str();
    if (selectedSsidStr.length()==0) return;

    debug_print("Trying Network "); debug_println(cSsid);
    ws.body[1] = MSG_TRYING_TO_CONNECT;
    if (ws.bodyCount < 2) ws.bodyCount = 2;
    renderer_->renderWait(ws);
    renderer_->renderBattery();
    debug_print("hostname "); debug_println(WiFi.getHostname());
    WiFi.begin(cSsid, cPassword);
    int tempTimer = millis();
    debug_print("Trying Network ... Checking status "); debug_print(cSsid); debug_print(" :"); debug_print(cPassword); debug_println(":");
    while ((WiFi.status() != WL_CONNECTED) && ((nowTime-startTime) <= SSID_CONNECTION_TIMEOUT)) {
        if (millis() > tempTimer + 125) {
            ws.advance();
            renderer_->renderWait(ws);
            renderer_->renderBattery();
            debug_print("."); tempTimer = millis();
        }
        nowTime = millis();
    }
    debug_println("");
    if (WiFi.status() == WL_CONNECTED) {
        WiFi.setSleep(false);
        debug_print("Connected. IP address: "); debug_println(WiFi.localIP());
        debug_println("WiFi modem sleep disabled for stable TCP");
        ws.body[1] = MSG_CONNECTED;
        ws.body[2] = MSG_ADDRESS_LABEL + String(WiFi.localIP());
        if (ws.bodyCount < 3) ws.bodyCount = 3;
        renderer_->renderTitle(ws);
        renderer_->renderBattery();
        stateManager_->setState(SystemState::WifiConnected);
        if (!MDNS.begin(connectionManager.hostname().c_str())) {
            debug_println("Error setting up MDNS responder!");
            ws.body[1] = MSG_BOUNJOUR_SETUP_FAILED;
            ws.bodyCount = 2;
            renderer_->renderTitle(ws);
            renderer_->renderBattery();
            delay(2000);
            stateManager_->setState(SystemState::Boot);
        } else {
            debug_print("MDNS responder started: "); debug_println(connectionManager.hostname());
        }
    } else {
        debug_println(MSG_CONNECTION_FAILED);
        ws.body[1] = MSG_CONNECTION_FAILED;
        ws.bodyCount = 2;
        renderer_->renderTitle(ws);
        renderer_->renderBattery();
        delay(2000);
        WiFi.disconnect();
        // Clear credentials so we don't retry with the same bad password
        selectedSsidStr = "";
        selectedSsidPasswordStr = "";
        // Go back to browse mode so the user can pick a different network
        ssidSelectionSource = SSID_CONNECTION_SOURCE_BROWSE;
        stateManager_->setState(SystemState::Boot);
    }
}
