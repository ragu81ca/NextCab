#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiThrottleProtocol.h>

// Forward declarations
class Renderer;
class ThrottleManager;
class InputManager;
class SystemStateManager;
class HeartbeatMonitor;
class WiThrottleServerSelectionHandler;
class WiThrottleDelegate;

#include "../../../static.h" // constants: MSG_*, menu_*, etc.
#include "../../../WiTcontroller.h" // maxFoundWitServers, DEFAULT_IP_AND_PORT, etc.
#include "../ui/TextInputScreen.h"

/**
 * WiThrottleConnectionManager
 *
 * Owns the lifecycle of WiThrottle server discovery, selection, manual entry,
 * TCP connection setup, and disconnection.  Extracted from WiTcontroller.ino
 * to reduce the monolith.
 *
 * All state that was previously scattered as globals in the sketch
 * (foundWitServers*, selectedWitServer*, witServerIpAndPort*, identity, etc.)
 * now lives here.
 */
class WiThrottleConnectionManager {
public:
    // ── Construction ──
    WiThrottleConnectionManager() = default;

    /**
     * Wire up references that the manager needs.
     * Call once from setup(), before any other method.
     */
    void begin(WiFiClient              &client,
               WiThrottleProtocol      &protocol,
               WiThrottleDelegate      &delegate,
               Renderer                &renderer,
               ThrottleManager         &throttleManager,
               InputManager            &inputManager,
               SystemStateManager      &systemStateManager,
               HeartbeatMonitor        &heartbeatMonitor,
               WiThrottleServerSelectionHandler &witServerSelectionHandler);

    // ── Main loop entry point (replaces witServiceLoop) ──
    void loop();

    // ── Actions callable from selection handlers / menus ──
    void browseService();
    void selectServer(int selection);
    void connectServer();
    void enterManualServer();
    void disconnect();

    // ── IP manual-entry helpers ──
    void entryAddChar(char key);
    void entryDeleteChar(char key);
    void buildEntry();

    // ── Identity (MAC-based) ──
    void computeIdentity();
    const String& hostname() const { return wifiHostname_; }
    const String& deviceId() const { return witDeviceId_; }

    // ── Callbacks the sketch must provide ──
    void setOnReleaseAllLocos(void (*cb)(int))   { onReleaseAllLocos_ = cb; }
    void setOnStartupCommands(void (*cb)())      { onStartupCommands_ = cb; }

    // ── Accessors (read-only for external code) ──
    IPAddress          selectedIP()       const { return selectedWitServerIP_; }
    int                selectedPort()     const { return selectedWitServerPort_; }
    const String&      selectedName()     const { return selectedWitServerName_; }
    int                foundCount()       const { return foundWitServersCount_; }
    const IPAddress*   foundIPs()         const { return foundWitServersIPs_; }
    const int*         foundPorts()       const { return foundWitServersPorts_; }
    const String*      foundNames()       const { return foundWitServersNames_; }

    // ── Mutable accessors (needed by WiThrottleServerSelectionHandler) ──
    String&     ipAndPortEntered()   { return witServerIpAndPortEntered_; }
    bool&       ipAndPortChanged()   { return witServerIpAndPortChanged_; }
    const String& ipAndPortConstructed() const { return witServerIpAndPortConstructed_; }

private:
    // ── External references (set by begin()) ──
    WiFiClient*                         client_                    = nullptr;
    WiThrottleProtocol*                 protocol_                  = nullptr;
    WiThrottleDelegate*                 delegate_                  = nullptr;
    Renderer*                           renderer_                  = nullptr;
    ThrottleManager*                    throttleManager_           = nullptr;
    InputManager*                       inputManager_              = nullptr;
    SystemStateManager*                 systemStateManager_        = nullptr;
    HeartbeatMonitor*                   heartbeatMonitor_          = nullptr;
    WiThrottleServerSelectionHandler*   witServerSelectionHandler_ = nullptr;

    // ── Callbacks ──
    void (*onReleaseAllLocos_)(int) = nullptr;
    void (*onStartupCommands_)()    = nullptr;

    // ── Discovered servers ──
    IPAddress foundWitServersIPs_[maxFoundWitServers];
    int       foundWitServersPorts_[maxFoundWitServers];
    String    foundWitServersNames_[maxFoundWitServers];
    int       foundWitServersCount_ = 0;
    int       noOfWitServices_      = 0;

    // ── Selected server ──
    IPAddress selectedWitServerIP_;
    int       selectedWitServerPort_ = 0;
    String    selectedWitServerName_;

    // ── Manual IP entry ──
    TextInputScreen manualEntryScreen_;
    String witServerIpAndPortConstructed_ = "";
    String witServerIpAndPortEntered_     = DEFAULT_IP_AND_PORT;
    bool   witServerIpAndPortChanged_     = true;

    // ── Protocol config (captured from sketch constants at begin()) ──
    int  outboundCmdsMininumDelay_   = OUTBOUND_COMMANDS_MINIMUM_DELAY;
    bool commandsNeedLeadingCrLf_    = SEND_LEADING_CR_LF_FOR_COMMANDS;

    // ── Identity ──
    String macLast4_;
    String wifiHostname_;
    String witDeviceId_;

    // ── Inactivity timer ──
    double startWaitForSelection_ = 0;
};
