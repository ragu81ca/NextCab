#include "WiThrottleDelegate.h"
#include "../ThrottleManager.h" // full definition for method calls
#include "../ServerDataStore.h"
#include "../LocoManager.h"
#include "../../../WiTcontroller.h" // extern throttleManager instance declaration
#include "../heartbeat/HeartbeatMonitor.h"
extern HeartbeatMonitor heartbeatMonitor;
extern ServerDataStore serverDataStore;
extern LocoManager locoManager;

// Helper to note server activity for heartbeat monitoring
static inline void noteServerActivity() {
    heartbeatMonitor.noteActivity(millis() / 1000, false);
}

void WiThrottleDelegate::heartbeatConfig(int seconds) { 
    heartbeatMonitor.setPeriod(seconds); 
    noteServerActivity(); // Config message is activity
}

void WiThrottleDelegate::receivedVersion(String version) { 
    noteServerActivity();
    debug_printf("Received Version: %s\n", version.c_str()); 
}

void WiThrottleDelegate::receivedServerDescription(String description) {
    noteServerActivity();
    locoManager.setServerType(description.substring(0, description.indexOf(" ")));
    if (locoManager.serverType().equals("DCC-EX")) { serverDataStore.setTurnoutPrefix(DCC_EX_TURNOUT_PREFIX); serverDataStore.setRoutePrefix(DCC_EX_ROUTE_PREFIX); }
}

void WiThrottleDelegate::receivedMessage(String message) {
    noteServerActivity();
    if ((!message.equals("Connected")) && (!message.equals("Connecting.."))) { broadcastMessageText = String(message); broadcastMessageTime = millis(); refreshOled(); }
}

void WiThrottleDelegate::receivedAlert(String message) {
    noteServerActivity();
    if ((!message.equals("Connected")) && (!message.equals("Connecting..")) && (!message.equals("Steal from other WiThrottle or JMRI throttle Required"))) { broadcastMessageText = String(message); broadcastMessageTime = millis(); refreshOled(); }
}

void WiThrottleDelegate::receivedSpeedMultiThrottle(char multiThrottle, int speed) {
    noteServerActivity();
    int idx = getMultiThrottleIndex(multiThrottle);
    
    // When momentum is active for this throttle, ignore server speed echoes - we manage speed locally
    if (throttleManager.momentum().isActive(idx)) {
        return;
    }
    
    // Use throttleManager internal arrays
    if (throttleManager.getCurrentSpeed(idx) != speed) {
        // basic debounce similar to previous logic
        if ((throttleManager.getLastSpeedThrottleIndex() != idx) || ((millis() - throttleManager.getLastSpeedSentTime()) > 500)) {
            // Directly set internal array (avoid triggering outbound command) then refresh
            throttleManager.speeds()[idx] = speed;
            displayUpdateFromWit(idx);
        }
    }
}
void WiThrottleDelegate::receivedDirectionMultiThrottle(char multiThrottle, Direction dir) {
    noteServerActivity();
    int idx = getMultiThrottleIndex(multiThrottle);
    if (throttleManager.directions()[idx] != dir) { throttleManager.directions()[idx] = dir; displayUpdateFromWit(idx); }
}

void WiThrottleDelegate::receivedDirectionMultiThrottle(char multiThrottle, String loco, Direction dir) { 
    noteServerActivity();
    /* placeholder */ 
}

void WiThrottleDelegate::receivedFunctionStateMultiThrottle(char multiThrottle, uint8_t func, bool state) {
    noteServerActivity();
    int idx = getMultiThrottleIndex(multiThrottle);
    if (throttleManager.getFunctionState(idx, func) != state) { throttleManager.setFunctionState(idx, func, state); displayUpdateFromWit(idx); }
}

void WiThrottleDelegate::receivedRosterFunctionListMultiThrottle(char multiThrottle, String functions[MAX_FUNCTIONS]) {
    noteServerActivity();
    int idx = getMultiThrottleIndex(multiThrottle);
    throttleManager.setFunctionLabelsFromRoster(idx, functions);
}

void WiThrottleDelegate::receivedTrackPower(TrackPower state) {
    noteServerActivity();
    if (trackPower != state) { trackPower = state; displayUpdateFromWit(-1); refreshOled(); }
}

void WiThrottleDelegate::receivedRosterEntries(int size) { 
    noteServerActivity();
    serverDataStore.setRosterSize(size); if (size == 0) { locoManager.restoreLocos(); } 
}

void WiThrottleDelegate::receivedRosterEntry(int index, String name, int address, char length) {
    noteServerActivity();
    serverDataStore.addRosterEntry(index, name, address, length);
    // When last entry arrives, addRosterEntry triggers sort internally
    if (index == serverDataStore.rosterSize() - 1) {
        locoManager.restoreLocos();
    }
    receivingServerInfoOled(index, serverDataStore.rosterSize());
#if ACQUIRE_ROSTER_ENTRY_IF_ONLY_ONE
    if ((serverDataStore.rosterSize() == 1) && (index == 0)) { doOneStartupCommand("*1#0"); }
#endif
}
void WiThrottleDelegate::receivedTurnoutEntries(int size) { 
    noteServerActivity();
    serverDataStore.setTurnoutListSize(size); 
}

void WiThrottleDelegate::receivedTurnoutEntry(int index, String sysName, String userName, int state) {
    noteServerActivity();
    serverDataStore.addTurnoutEntry(index, sysName, userName, state);
    receivingServerInfoOled(index, serverDataStore.turnoutListSize());
}

void WiThrottleDelegate::receivedRouteEntries(int size) { 
    noteServerActivity();
    serverDataStore.setRouteListSize(size); 
}

void WiThrottleDelegate::receivedRouteEntry(int index, String sysName, String userName, int state) {
    noteServerActivity();
    serverDataStore.addRouteEntry(index, sysName, userName, state);
    receivingServerInfoOled(index, serverDataStore.routeListSize());
}
void WiThrottleDelegate::addressStealNeeded(String address, String entry) { locoManager.stealLoco(throttleManager.getCurrentThrottleChar(), address); }
void WiThrottleDelegate::addressStealNeededMultiThrottle(char multiThrottle, String address, String entry) { locoManager.stealLoco(multiThrottle, address); }
void WiThrottleDelegate::receivedUnknownCommand(String unknownCommand) { 
    noteServerActivity();
    debug_print("Received unknown command: "); debug_println(unknownCommand); 
}
