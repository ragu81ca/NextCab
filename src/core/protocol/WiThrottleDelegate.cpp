#include "WiThrottleDelegate.h"
#include "../ThrottleManager.h" // full definition for method calls
#include "../../../WiTcontroller.h" // extern throttleManager instance declaration

// Forward declaration for helper defined in sketch
void stealCurrentLoco(String address);

void WiThrottleDelegate::heartbeatConfig(int seconds) { heartbeatPeriod = seconds; }
void WiThrottleDelegate::receivedVersion(String version) { debug_printf("Received Version: %s\n", version.c_str()); }
void WiThrottleDelegate::receivedServerDescription(String description) {
    serverType = description.substring(0, description.indexOf(" "));
    if (serverType.equals("DCC-EX")) { turnoutPrefix = DCC_EX_TURNOUT_PREFIX; routePrefix = DCC_EX_ROUTE_PREFIX; }
}
void WiThrottleDelegate::receivedMessage(String message) {
    if ((!message.equals("Connected")) && (!message.equals("Connecting.."))) { broadcastMessageText = String(message); broadcastMessageTime = millis(); refreshOled(); }
}
void WiThrottleDelegate::receivedAlert(String message) {
    if ((!message.equals("Connected")) && (!message.equals("Connecting..")) && (!message.equals("Steal from other WiThrottle or JMRI throttle Required"))) { broadcastMessageText = String(message); broadcastMessageTime = millis(); refreshOled(); }
}
void WiThrottleDelegate::receivedSpeedMultiThrottle(char multiThrottle, int speed) {
    int idx = getMultiThrottleIndex(multiThrottle);
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
    int idx = getMultiThrottleIndex(multiThrottle);
    if (throttleManager.directions()[idx] != dir) { throttleManager.directions()[idx] = dir; displayUpdateFromWit(idx); }
}
void WiThrottleDelegate::receivedDirectionMultiThrottle(char multiThrottle, String loco, Direction dir) { /* placeholder */ }
void WiThrottleDelegate::receivedFunctionStateMultiThrottle(char multiThrottle, uint8_t func, bool state) {
    int idx = getMultiThrottleIndex(multiThrottle);
    if (functionStates[idx][func] != state) { functionStates[idx][func] = state; displayUpdateFromWit(idx); }
}
void WiThrottleDelegate::receivedRosterFunctionListMultiThrottle(char multiThrottle, String functions[MAX_FUNCTIONS]) {
    int idx = getMultiThrottleIndex(multiThrottle);
    for (int i = 0; i < MAX_FUNCTIONS; i++) { functionLabels[idx][i] = functions[i]; }
}
void WiThrottleDelegate::receivedTrackPower(TrackPower state) {
    if (trackPower != state) { trackPower = state; displayUpdateFromWit(-1); refreshOled(); }
}
void WiThrottleDelegate::receivedRosterEntries(int size) { rosterSize = size; if (rosterSize == 0) { setupPreferences(false); } }
void WiThrottleDelegate::receivedRosterEntry(int index, String name, int address, char length) {
    if (index < rosterSize) {
        rosterIndex[index] = index; rosterSortedIndex[index] = index; rosterName[index] = name; rosterAddress[index] = address; rosterLength[index] = length;
        if (ROSTER_SORT_SEQUENCE == 1) { strncpy(rosterSortStrings[index], ((name + "          ").substring(0, 10) + ":" + (index < 10 ? "0" : "") + String(index)).c_str(), 13); rosterSortPointers[index] = rosterSortStrings[index]; }
        else if (ROSTER_SORT_SEQUENCE == 2) { char buf[11]; sprintf(buf, "%10d", rosterAddress[index]); strncpy(rosterSortStrings[index], (String(buf) + ":" + (index < 10 ? "0" : "") + String(index)).c_str(), 13); rosterSortPointers[index] = rosterSortStrings[index]; }
        if ((index == (rosterSize - 1)) && (ROSTER_SORT_SEQUENCE > 0)) { qsort(rosterSortPointers, rosterSize, sizeof rosterSortPointers[0], compareStrings); for (int i = 0; i < rosterSize; i++) { rosterSortedIndex[i] = (rosterSortPointers[i][11] - '0') * 10 + (rosterSortPointers[i][12] - '0'); } setupPreferences(false); }
    }
    receivingServerInfoOled(index, rosterSize);
#if ACQUIRE_ROSTER_ENTRY_IF_ONLY_ONE
    if ((rosterSize == 1) && (index == 0)) { doOneStartupCommand("*1#0"); }
#endif
}
void WiThrottleDelegate::receivedTurnoutEntries(int size) { turnoutListSize = size; }
void WiThrottleDelegate::receivedTurnoutEntry(int index, String sysName, String userName, int state) {
    if (index < turnoutListSize) { turnoutListIndex[index] = index; turnoutListSysName[index] = sysName; turnoutListUserName[index] = userName; turnoutListState[index] = state; }
    receivingServerInfoOled(index, turnoutListSize);
}
void WiThrottleDelegate::receivedRouteEntries(int size) { routeListSize = size; }
void WiThrottleDelegate::receivedRouteEntry(int index, String sysName, String userName, int state) {
    if (index < routeListSize) { routeListIndex[index] = index; routeListSysName[index] = sysName; routeListUserName[index] = userName; routeListState[index] = state; }
    receivingServerInfoOled(index, routeListSize);
}
void WiThrottleDelegate::addressStealNeeded(String address, String entry) { stealCurrentLoco(address); }
void WiThrottleDelegate::addressStealNeededMultiThrottle(char multiThrottle, String address, String entry) { stealLoco(multiThrottle, address); }
void WiThrottleDelegate::receivedUnknownCommand(String unknownCommand) { debug_print("Received unknown command: "); debug_println(unknownCommand); }
