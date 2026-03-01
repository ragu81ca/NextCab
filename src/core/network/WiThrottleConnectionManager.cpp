#include "WiThrottleConnectionManager.h"
#include "../Renderer.h"
#include "../ThrottleManager.h"
#include "../input/InputManager.h"
#include "../input/WiThrottleServerSelectionHandler.h"
#include "../SystemState.h"
#include "../heartbeat/HeartbeatMonitor.h"
#include "../DisplayConfig.h"        // displayDriver (for sendBuffer)
#include "../../../WiTcontroller.h"  // oledText macros, activeLayout, MSG_*, menu_*, etc.
#include "../protocol/WiThrottleDelegate.h" // debug_print macros, WiThrottleDelegate

// External display helpers still living in the sketch (thin wrappers)
extern void setAppnameForOled();
extern void clearOledArray();
extern void setMenuTextForOled(int);
extern String getDots(int);

// External config vars still living in the sketch
extern bool hashShowsFunctionsInsteadOfKeyDefs;
extern int ssidSelectionSource;
extern String selectedSsid;

// ──────────────────────────────────────────────
//  begin()
// ──────────────────────────────────────────────

void WiThrottleConnectionManager::begin(
        WiFiClient              &client,
        WiThrottleProtocol      &protocol,
        WiThrottleDelegate      &delegate,
        Renderer                &renderer,
        ThrottleManager         &throttleManager,
        InputManager            &inputManager,
        SystemStateManager      &systemStateManager,
        HeartbeatMonitor        &heartbeatMonitor,
        WiThrottleServerSelectionHandler &witServerSelectionHandler)
{
    client_                    = &client;
    protocol_                  = &protocol;
    delegate_                  = &delegate;
    renderer_                  = &renderer;
    throttleManager_           = &throttleManager;
    inputManager_              = &inputManager;
    systemStateManager_        = &systemStateManager;
    heartbeatMonitor_          = &heartbeatMonitor;
    witServerSelectionHandler_ = &witServerSelectionHandler;
}

// ──────────────────────────────────────────────
//  loop()  (was witServiceLoop)
// ──────────────────────────────────────────────

void WiThrottleConnectionManager::loop() {
    SystemState state = systemStateManager_->getState();

    if (state == SystemState::WifiConnected || state == SystemState::ServerScanning) {
        browseService();
    }

    if (state == SystemState::ServerManualEntry) {
        enterManualServer();
    }

    if (state == SystemState::ServerConnecting) {
        connectServer();
    }

    // ServerSelection state just waits for user input via WiThrottleServerSelectionHandler
}

// ──────────────────────────────────────────────
//  browseService()  (was browseWitService)
// ──────────────────────────────────────────────

void WiThrottleConnectionManager::browseService() {
    debug_println("browseWitService()");

    double startTime = millis();
    double nowTime = startTime;

    const char* service = "withrottle";
    const char* proto   = "tcp";

    debug_printf("Browsing for service _%s._%s.local. on %s ... ", service, proto, selectedSsid.c_str());
    clearOledArray();
    oledText[0] = appName; oledText[activeLayout.secondColumnStartRow] = appVersion;
    oledText[1] = selectedSsid;
    oledText[2] = MSG_BROWSING_FOR_SERVICE;
    renderer_->renderBattery();
    renderer_->renderArray(false, false, true, true);

    startWaitForSelection_ = millis();

    noOfWitServices_ = 0;
    if ((selectedSsid.substring(0, 6) == "DCCEX_") && (selectedSsid.length() == 12)) {
        debug_println(MSG_BYPASS_WIT_SERVER_SEARCH);
        oledText[1] = MSG_BYPASS_WIT_SERVER_SEARCH;
        renderer_->renderBattery();
        renderer_->renderArray(false, false, true, true);
        delay(500);
    } else {
        int j = 0;
        while ((noOfWitServices_ == 0) && ((nowTime - startTime) <= 10000)) {
            noOfWitServices_ = MDNS.queryService(service, proto);
            oledText[3] = getDots(j);
            renderer_->renderBattery();
            renderer_->renderArray(false, false, true, true);
            j++;
            debug_print(".");
            nowTime = millis();
        }
        debug_println("");
    }

    foundWitServersCount_ = noOfWitServices_;
    if (noOfWitServices_ > 0) {
        for (int i = 0; (i < noOfWitServices_) && (i < maxFoundWitServers); ++i) {
            foundWitServersNames_[i] = MDNS.hostname(i);
            foundWitServersIPs_[i]   = ESPMDNS_IP_ATTRIBUTE_NAME;
            foundWitServersPorts_[i] = MDNS.port(i);
            if (MDNS.hasTxt(i, "jmri")) {
                String node = MDNS.txt(i, "node");
                node.toLowerCase();
                if (foundWitServersNames_[i].equals(node)) {
                    foundWitServersNames_[i] = "JMRI  (v" + MDNS.txt(i, "jmri") + ")";
                }
            }
        }
    }

    if ((selectedSsid.substring(0, 6) == "DCCEX_") && (selectedSsid.length() == 12)) {
        foundWitServersIPs_[foundWitServersCount_].fromString("192.168.4.1");
        foundWitServersPorts_[foundWitServersCount_] = 2560;
        foundWitServersNames_[foundWitServersCount_] = MSG_GUESSED_EX_CS_WIT_SERVER;
        foundWitServersCount_++;
    }

    if (foundWitServersCount_ == 0) {
        oledText[1] = MSG_NO_SERVICES_FOUND;
        renderer_->renderBattery();
        renderer_->renderArray(false, false, true, true);
        debug_println(oledText[1]);
        delay(1000);
        buildEntry();
        systemStateManager_->setState(SystemState::ServerManualEntry);
    } else {
        debug_print(noOfWitServices_); debug_println(MSG_SERVICES_FOUND);
        clearOledArray();
        oledText[3] = MSG_SERVICES_FOUND;

        for (int i = 0; i < foundWitServersCount_; ++i) {
            debug_print("  "); debug_print(i); debug_print(": '"); debug_print(foundWitServersNames_[i]);
            debug_print("' ("); debug_print(foundWitServersIPs_[i]); debug_print(":"); debug_print(foundWitServersPorts_[i]); debug_println(")");
            if (i < 5) {
                String truncatedIp = ".." + foundWitServersIPs_[i].toString().substring(foundWitServersIPs_[i].toString().lastIndexOf("."));
                oledText[i] = String(i + 1) + ": " + truncatedIp + ":" + String(foundWitServersPorts_[i]) + " " + foundWitServersNames_[i];
            }
        }

        if (foundWitServersCount_ > 0) {
            setMenuTextForOled(menu_select_wit_service);
        }
        renderer_->renderArray(false, false);

        // Auto-connect: if exactly 1 WiThrottle server found, connect automatically
        if (foundWitServersCount_ == 1) {
            debug_println("Auto-connecting to only WiThrottle server found");
            selectedWitServerIP_   = foundWitServersIPs_[0];
            selectedWitServerPort_ = foundWitServersPorts_[0];
            selectedWitServerName_ = foundWitServersNames_[0];
            systemStateManager_->setState(SystemState::ServerConnecting);
        } else {
            debug_println("WiT Selection required");
            systemStateManager_->setState(SystemState::ServerSelection);
            witServerSelectionHandler_->setSource(WiThrottleServerSource::Discovered);
        }
    }
}

// ──────────────────────────────────────────────
//  selectServer()  (was selectWitServer)
// ──────────────────────────────────────────────

void WiThrottleConnectionManager::selectServer(int selection) {
    debug_print("selectWitServer() "); debug_println(selection);

    if ((selection >= 0) && (selection < foundWitServersCount_)) {
        systemStateManager_->setState(SystemState::ServerConnecting);
        selectedWitServerIP_   = foundWitServersIPs_[selection];
        selectedWitServerPort_ = foundWitServersPorts_[selection];
        selectedWitServerName_ = foundWitServersNames_[selection];
    }
}

// ──────────────────────────────────────────────
//  connectServer()  (was connectWitServer)
// ──────────────────────────────────────────────

void WiThrottleConnectionManager::connectServer() {
    // Pass the delegate instance to wiThrottleProtocol
    protocol_->setDelegate(delegate_);
#if WITHROTTLE_PROTOCOL_DEBUG == 0
    protocol_->setLogStream(&Serial);
    protocol_->setLogLevel(DEBUG_LEVEL);
#endif

    debug_println("Connecting to the server...");
    clearOledArray();
    setAppnameForOled();
    oledText[1] = "        " + selectedWitServerIP_.toString() + " : " + String(selectedWitServerPort_);
    oledText[2] = "        " + selectedWitServerName_;
    oledText[3] + MSG_CONNECTING;
    renderer_->renderBattery();
    renderer_->renderArray(false, false, true, true);

    startWaitForSelection_ = millis();

    if (!client_->connect(selectedWitServerIP_, selectedWitServerPort_)) {
        debug_println(MSG_CONNECTION_FAILED);
        oledText[3] = MSG_CONNECTION_FAILED;
        renderer_->renderArray(false, false, true, true);
        delay(5000);

        systemStateManager_->setState(SystemState::WifiConnected);
        ssidSelectionSource = SSID_CONNECTION_SOURCE_LIST;
        witServerIpAndPortChanged_ = true;

    } else {
        debug_print("Connected to server: ");
        debug_println(selectedWitServerIP_); debug_println(selectedWitServerPort_);

        protocol_->connect(client_, outboundCmdsMininumDelay_);
        debug_println("WiThrottle connected");

        protocol_->setDeviceName(appName);
        protocol_->setDeviceID(witDeviceId_);
        debug_print("WiThrottle Device Name: "); debug_println(appName);
        debug_print("WiThrottle Device ID: ");   debug_println(witDeviceId_);
        protocol_->setCommandsNeedLeadingCrLf(commandsNeedLeadingCrLf_);
        if (HEARTBEAT_ENABLED) {
            protocol_->requireHeartbeat(true);
        }

        systemStateManager_->setState(SystemState::Operating);

        if (HEARTBEAT_ENABLED) {
            heartbeatMonitor_->begin(MAX_HEARTBEAT_PERIOD / 1000, true);
            debug_printf("Heartbeat monitoring started with initial timeout %lus (waiting for server config)\n",
                         (MAX_HEARTBEAT_PERIOD / 1000) * HeartbeatMonitor::TIMEOUT_MULTIPLIER);
        }

        oledText[3] = MSG_CONNECTED;
        if (!hashShowsFunctionsInsteadOfKeyDefs) {
            setMenuTextForOled(menu_menu);
        } else {
            setMenuTextForOled(menu_menu_hash_is_functions);
        }
        renderer_->renderArray(false, false, true, true);
        renderer_->renderBattery();
        displayDriver.sendBuffer();

        if (onStartupCommands_) onStartupCommands_();
    }
}

// ──────────────────────────────────────────────
//  enterManualServer()  (was enterWitServer)
// ──────────────────────────────────────────────

void WiThrottleConnectionManager::enterManualServer() {
    inputManager_->setMode(InputMode::WiThrottleServerSelection);
    witServerSelectionHandler_->setSource(WiThrottleServerSource::ManualEntry);
    if (witServerIpAndPortChanged_) {
        debug_println("enterWitServer()");
        clearOledArray();
        setAppnameForOled();
        oledText[1] = MSG_NO_SERVICES_FOUND_ENTRY_REQUIRED;
        oledText[3] = witServerIpAndPortConstructed_;
        setMenuTextForOled(menu_select_wit_entry);
        renderer_->renderArray(false, false, true, true);
        witServerIpAndPortChanged_ = false;
    }
}

// ──────────────────────────────────────────────
//  disconnect()  (was disconnectWitServer)
// ──────────────────────────────────────────────

void WiThrottleConnectionManager::disconnect() {
    debug_println("disconnectWitServer()");

    if (HEARTBEAT_ENABLED) {
        heartbeatMonitor_->setEnabled(false);
        debug_println("Heartbeat monitoring stopped");
    }

    if (onReleaseAllLocos_) {
        for (int i = 0; i < throttleManager_->getMaxThrottles(); i++) {
            onReleaseAllLocos_(i);
        }
    }

    protocol_->disconnect();
    debug_println("Disconnected from wiThrottle server\n");
    clearOledArray();
    oledText[0] = MSG_DISCONNECTED;
    renderer_->renderArray(false, false, true, true);
    systemStateManager_->setState(SystemState::WifiConnected);
    witServerIpAndPortChanged_ = true;
}

// ──────────────────────────────────────────────
//  IP entry helpers
// ──────────────────────────────────────────────

void WiThrottleConnectionManager::entryAddChar(char key) {
    if (witServerIpAndPortEntered_.length() < 17) {
        witServerIpAndPortEntered_ = witServerIpAndPortEntered_ + key;
        debug_print("wit entered: ");
        debug_println(witServerIpAndPortEntered_);
        buildEntry();
        witServerIpAndPortChanged_ = true;
    }
}

void WiThrottleConnectionManager::entryDeleteChar(char key) {
    if (witServerIpAndPortEntered_.length() > 0) {
        witServerIpAndPortEntered_ = witServerIpAndPortEntered_.substring(0, witServerIpAndPortEntered_.length() - 1);
        debug_print("wit deleted: ");
        debug_println(witServerIpAndPortEntered_);
        buildEntry();
        witServerIpAndPortChanged_ = true;
    }
}

void WiThrottleConnectionManager::buildEntry() {
    debug_println("buildWitEntry()");
    witServerIpAndPortConstructed_ = "";
    for (unsigned int i = 0; i < witServerIpAndPortEntered_.length(); i++) {
        if ((i == 3) || (i == 6) || (i == 9)) {
            witServerIpAndPortConstructed_ = witServerIpAndPortConstructed_ + ".";
        } else if (i == 12) {
            witServerIpAndPortConstructed_ = witServerIpAndPortConstructed_ + ":";
        }
        witServerIpAndPortConstructed_ = witServerIpAndPortConstructed_ + witServerIpAndPortEntered_.substring(i, i + 1);
    }
    debug_print("wit Constructed: ");
    debug_println(witServerIpAndPortConstructed_);
    if (witServerIpAndPortEntered_.length() < 17) {
        witServerIpAndPortConstructed_ = witServerIpAndPortConstructed_ + witServerIpAndPortEntryMask.substring(witServerIpAndPortConstructed_.length());
    }
    debug_print("wit Constructed: ");
    debug_println(witServerIpAndPortConstructed_);

    if (witServerIpAndPortEntered_.length() == 17) {
        selectedWitServerIP_.fromString(witServerIpAndPortConstructed_.substring(0, 15));
        selectedWitServerPort_ = witServerIpAndPortConstructed_.substring(16).toInt();
    }
}

// ──────────────────────────────────────────────
//  computeIdentity()  (was computeIdentityFromMac)
// ──────────────────────────────────────────────

void WiThrottleConnectionManager::computeIdentity() {
    String macStr = WiFi.macAddress(); // Expected format "AA:BB:CC:DD:EE:FF"
    if (macStr.length() == 17) {
        int lastSep = macStr.lastIndexOf(':');
        String byte5 = macStr.substring(lastSep - 2, lastSep);
        String byte6 = macStr.substring(lastSep + 1);
        macLast4_ = byte5 + byte6;
        macLast4_.toUpperCase();
        debug_print("computeIdentityFromMac(): MAC via WiFi.macAddress() -> ");
        debug_println(macLast4_);
    } else {
        macLast4_ = "0000";
        debug_print("computeIdentityFromMac(): macAddress invalid ('");
        debug_print(macStr); debug_println("'), using 0000");
    }
    wifiHostname_ = String(appName) + "-" + macLast4_;
    witDeviceId_  = macLast4_;
}
