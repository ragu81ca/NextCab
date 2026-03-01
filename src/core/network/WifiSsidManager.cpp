#include "WifiSsidManager.h"
#include "WiThrottleConnectionManager.h"
#include <WiFi.h>
#include <ESPmDNS.h>
#include "../../../static.h" // macro constants
#include "../../../WiTcontroller.h" // size-related macros (maxFoundSsids etc.)
#include "../Renderer.h"
#include "../UIState.h"
#include "../protocol/WiThrottleDelegate.h" // debug_print macros
#include "../input/InputManager.h"
#include "../SystemState.h"
#include "../ui/TitleScreen.h"
#include "../ui/WaitScreen.h"

// External state still hosted in sketch
extern InputManager inputManager;
extern SystemStateManager systemStateManager;
extern int ssidSelectionSource;
extern String turnoutPrefix;
extern String routePrefix;
extern String foundSsids[];
extern long foundSsidRssis[];
extern UIState uiState;
extern bool foundSsidsOpen[];
extern int foundSsidsCount;
// Timeout & sources
// SSID_CONNECTION_TIMEOUT is a macro constant in static.h
// Configured arrays
extern String ssids[];
extern String passwords[];
// Fallback prefix arrays if not linked from config_network.h (will be overridden when begin() passes real arrays)
extern String turnoutPrefixes[]; // declared in config_network.h normally
extern String routePrefixes[];   // declared in config_network.h normally

// UI side effects (temporary coupling)
extern Renderer renderer;

int WifiSsidManager::_foundSsidsCountExtern() {
    extern int foundSsidsCount;
    return foundSsidsCount;
}

void WifiSsidManager::begin(const String* configuredSsids,
               const String* configuredPasswords,
               int configuredCount,
               StateCallback onStateChange,
               ListChangedCallback onListChanged) {
    ssids = configuredSsids;
    passwords = configuredPasswords;
    maxSsids = configuredCount;
    onStateChangeCb = onStateChange;
    onListChangedCb = onListChanged;
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
    if (i>=0 && i<foundSsidsCount) {
        fs.ssid = foundSsids[i];
        fs.rssi = foundSsidRssis[i];
        fs.open = foundSsidsOpen[i];
    }
    return fs;
}

void WifiSsidManager::loop() {
    // Replicates legacy ssidsLoop decision tree.
    if (!systemStateManager.isConnectedToWifi()) {
        if (ssidSelectionSource == SSID_CONNECTION_SOURCE_LIST) {
            showConfiguredList();
        } else {
            browseSsids();
        }
    }
    // Password entry UI is automatically handled by InputManager when mode is set to PasswordEntry
    if (systemStateManager.getState() == SystemState::WifiConnecting) {
        connectSelectedInternal();
    }
}

void WifiSsidManager::browseSsids() {
    debug_println("WifiSsidManager::browseSsids()");
    double startTime = millis();
    double nowTime = startTime;

    { TitleScreen ts;
      ts.setAppHeader(appName, appVersion);
      ts.addBody(MSG_BROWSING_FOR_SSIDS);
      renderer.renderTitle(ts);
      renderer.renderBattery(); }

    // Ensure in station mode and not connected before scanning
    WiFi.mode(WIFI_STA);
    WiFi.disconnect(); // prevent filtering only current network
    delay(100);
    WiFi.setScanMethod(WIFI_ALL_CHANNEL_SCAN);
    WiFi.setSortMethod(WIFI_CONNECT_AP_BY_SIGNAL);
    int num = WiFi.scanNetworks();
    debug_print("scanNetworks() returned: "); debug_println(num);
    while ( (num == -1) && ((nowTime-startTime) <= 10000) ) {
        delay(250);
        debug_print(".");
        nowTime = millis();
    }

    foundSsidsCount = 0;
    if (num > 0) {
    for (int thisSsid=0; thisSsid<num && foundSsidsCount<maxFoundSsids; ++thisSsid) {
            String ssid = WiFi.SSID(thisSsid);
            bool duplicate = false;
            for (int i=0;i<foundSsidsCount;i++) { if (foundSsids[i]==ssid) { duplicate=true; break; } }
            if (!duplicate) {
                foundSsids[foundSsidsCount] = ssid;
                foundSsidRssis[foundSsidsCount] = WiFi.RSSI(thisSsid);
                foundSsidsOpen[foundSsidsCount] = (WiFi.encryptionType(thisSsid) == 7);
                foundSsidsCount++;
            }
        }
        for (int i=0;i<foundSsidsCount;i++) { debug_println(foundSsids[i]); }
        // Use unified rendering path: delegate to renderer::renderFoundSsids
        uiState.page = 0; // reset to first page
        renderer.renderFoundSsids("");
        systemStateManager.setState(SystemState::WifiSelection);
        
        // Auto-connect: if exactly ONE scanned SSID matches a configured SSID, connect automatically
        int matchCount = 0;
        int matchIndex = -1;
        for (int found = 0; found < foundSsidsCount; found++) {
            for (int cfg = 0; cfg < maxSsids; cfg++) {
                if (foundSsids[found] == ssids[cfg]) {
                    matchCount++;
                    matchIndex = found;
                    break; // Don't double-count
                }
            }
        }
        if (matchCount == 1 && matchIndex >= 0) {
            debug_println("Auto-connecting to only matching known SSID");
            systemStateManager.setState(SystemState::WifiConnecting);
            selectedSsidStr = foundSsids[matchIndex];
            getSsidPasswordAndMetadataForFound();
        }
    } else {
        { TitleScreen ts;
          ts.setAppHeader(appName, appVersion);
          ts.addBody(MSG_NO_SSIDS_FOUND);
          renderer.renderTitle(ts); }
        debug_println("No SSIDs found in scan");
    }
}

void WifiSsidManager::showConfiguredList() {
    debug_println("WifiSsidManager::showConfiguredList()");
    if (maxSsids == 0) {
        { TitleScreen ts;
          ts.setAppHeader(appName, appVersion);
          ts.addBody(MSG_NO_SSIDS_FOUND);
          renderer.renderTitle(ts);
          renderer.renderBattery(); }
        debug_println(MSG_NO_SSIDS_FOUND);
        return;
    }
    debug_print(maxSsids); debug_println(MSG_SSIDS_LISTED);
    // Auto-connect if exactly 1 configured SSID
    if (maxSsids == 1) {
        selectedSsidStr = ssids[0];
        selectedSsidPasswordStr = passwords[0];
        systemStateManager.setState(SystemState::WifiConnecting);
        turnoutPrefix = turnoutPrefixes[0];
        routePrefix = routePrefixes[0];
    } else {
        systemStateManager.setState(SystemState::WifiSelection);
        // Rendering is handled by WifiSelectionHandler via ListSelectionScreen
    }
}

void WifiSsidManager::selectConfigured(int index) {
    if (index>=0 && index<maxSsids) {
    selectedSsidStr = ssids[index];
    selectedSsidPasswordStr = passwords[index];
        changeState(State::Selected);
        systemStateManager.setState(SystemState::WifiConnecting);
    }
}

void WifiSsidManager::selectFound(int index) {
    if (index>=0 && index<foundSsidsCount) {
    selectedSsidStr = foundSsids[index];
        getSsidPasswordAndMetadataForFound();
    if (selectedSsidPasswordStr.length()==0) {
            changeState(State::PasswordEntry);
            systemStateManager.setState(SystemState::WifiPasswordEntry);
        } else {
            changeState(State::Selected);
            systemStateManager.setState(SystemState::WifiConnecting);
        }
    }
}

void WifiSsidManager::getSsidPasswordAndMetadataForFound() {
    bool found = false;
    for (int i=0;i<maxSsids;i++) {
        if (selectedSsidStr == ssids[i]) {
            selectedSsidPasswordStr = passwords[i];
            turnoutPrefix = turnoutPrefixes[i];
            routePrefix = routePrefixes[i];
            found = true; break;
        }
    }
    if (!found) {
        if (selectedSsidStr.startsWith("DCCEX_") && selectedSsidStr.length()==12) {
            selectedSsidPasswordStr = String("PASS_") + selectedSsidStr.substring(6);
            // Mirror legacy heuristic: Set default WiT server IP:Port when connecting to DCC-EX AP
            extern WiThrottleConnectionManager connectionManager;
            connectionManager.ipAndPortEntered() = "19216800400102560"; // 192.168.4.1:2560 compressed
            turnoutPrefix = DCC_EX_TURNOUT_PREFIX; // macro constant
            routePrefix = DCC_EX_ROUTE_PREFIX;     // macro constant
            debug_println("getSsidPasswordAndMetadataForFound() Using guessed DCC-EX password & defaults");
        }
    }
}

void WifiSsidManager::startPasswordEntry() {
    changeState(State::PasswordEntry);
    systemStateManager.setState(SystemState::WifiPasswordEntry);
}

void WifiSsidManager::appendPasswordChar(char c) {
    if (c=='#') return; // commit handled elsewhere
    if (passwordEntered.length()<32) { passwordEntered += c; passwordChanged = true; }
}

void WifiSsidManager::backspacePassword() {
    if (passwordEntered.length()>0) { passwordEntered.remove(passwordEntered.length()-1); passwordChanged = true; }
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
    renderer.renderWait(ws);
    renderer.renderBattery();

    double startTime = millis();
    double nowTime = startTime;
    const char *cSsid = selectedSsidStr.c_str();
    const char *cPassword = selectedSsidPasswordStr.c_str();
    if (selectedSsidStr.length()==0) return;

    debug_print("Trying Network "); debug_println(cSsid);
    for (int i=0; i<3; ++i) {
        ws.body[1] = String(MSG_TRYING_TO_CONNECT) + " (" + String(i + 1) + " of 3)";
        if (ws.bodyCount < 2) ws.bodyCount = 2;
        renderer.renderWait(ws);
        renderer.renderBattery();
        nowTime = startTime;
        debug_print("hostname "); debug_println(WiFi.getHostname());
        WiFi.begin(cSsid, cPassword);
        int tempTimer = millis();
        debug_print("Trying Network ... Checking status "); debug_print(cSsid); debug_print(" :"); debug_print(cPassword); debug_println(":");
        while ((WiFi.status() != WL_CONNECTED) && ((nowTime-startTime) <= SSID_CONNECTION_TIMEOUT)) {
            if (millis() > tempTimer + 125) {
                ws.advance();
                renderer.renderWait(ws);
                renderer.renderBattery();
                debug_print("."); tempTimer = millis();
            }
            nowTime = millis();
        }
        if (WiFi.status() == WL_CONNECTED) {
            if (!commandsNeedLeadingCrLf) { debug_println("Leading CRLF will not be sent for commands"); }
            break; // success
        } else {
            debug_println("");
        }
    }
    debug_println("");
    if (WiFi.status() == WL_CONNECTED) {
        // Disable WiFi modem sleep to prevent radio power-down from dropping TCP connections.
        // Without this, the ESP32 periodically turns off the radio to save power,
        // which causes random TCP disconnects with persistent connections like WiThrottle.
        WiFi.setSleep(false);
        debug_print("Connected. IP address: "); debug_println(WiFi.localIP());
        debug_println("WiFi modem sleep disabled for stable TCP");
        ws.body[1] = MSG_CONNECTED;
        ws.body[2] = MSG_ADDRESS_LABEL + String(WiFi.localIP());
        if (ws.bodyCount < 3) ws.bodyCount = 3;
        renderer.renderTitle(ws);
        renderer.renderBattery();
        systemStateManager.setState(SystemState::WifiConnected);
        // Mode will be set by systemStateManager
        if (!MDNS.begin(connectionManager.hostname().c_str())) {
            debug_println("Error setting up MDNS responder!");
            ws.body[1] = MSG_BOUNJOUR_SETUP_FAILED;
            ws.bodyCount = 2;
            renderer.renderTitle(ws);
            renderer.renderBattery();
            delay(2000);
            systemStateManager.setState(SystemState::Boot);
        } else {
            debug_print("MDNS responder started: "); debug_println(connectionManager.hostname());
        }
    } else {
        debug_println(MSG_CONNECTION_FAILED);
        ws.body[1] = MSG_CONNECTION_FAILED;
        ws.bodyCount = 2;
        renderer.renderTitle(ws);
        renderer.renderBattery();
        delay(2000);
        WiFi.disconnect();
        systemStateManager.setState(SystemState::Boot);
        ssidSelectionSource = SSID_CONNECTION_SOURCE_LIST;
    }
}
