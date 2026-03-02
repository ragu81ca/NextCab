#pragma once
// WiThrottleDelegate.h - extracted protocol delegate to reduce sketch size
#include <Arduino.h>
#include <WiThrottleProtocol.h>
#include "static.h"

#ifndef debug_print
#define debug_print(...) Serial.print(__VA_ARGS__)
#endif
#ifndef debug_println
#define debug_println(...) Serial.println(__VA_ARGS__)
#endif
#ifndef debug_printf
#define debug_printf(...) Serial.printf(__VA_ARGS__)
#endif

void refreshOled();
void receivingServerInfoOled(int index, int total);
void setupPreferences(bool forceClear);
void doOneStartupCommand(String cmd);

// Speed/direction/function arrays migrated to ThrottleManager\n// Roster/turnout/route data migrated to ServerDataStore
extern TrackPower trackPower;
extern long lastSpeedSentTime;
extern int lastSpeedThrottleIndex;
extern int currentThrottleIndex;
// serverType now in locoManager
extern String broadcastMessageText;
extern long broadcastMessageTime;

int getMultiThrottleIndex(char multiThrottle);
char getMultiThrottleChar(int multiThrottleIndex);
// stealLoco now in LocoManager
void displayUpdateFromWit(int multiThrottleIndex);

#ifndef ROSTER_SORT_SEQUENCE
#define ROSTER_SORT_SEQUENCE 0
#endif
#ifndef ACQUIRE_ROSTER_ENTRY_IF_ONLY_ONE
#define ACQUIRE_ROSTER_ENTRY_IF_ONLY_ONE 0
#endif

class WiThrottleDelegate : public WiThrottleProtocolDelegate {
public:
    WiThrottleDelegate() = default;
    void heartbeatConfig(int seconds) override;
    void receivedVersion(String version) override;
    void receivedServerDescription(String description) override;
    void receivedMessage(String message) override;
    void receivedAlert(String message) override;
    void receivedSpeedMultiThrottle(char multiThrottle, int speed) override;
    void receivedDirectionMultiThrottle(char multiThrottle, Direction dir) override;
    void receivedDirectionMultiThrottle(char multiThrottle, String loco, Direction dir) override;
    void receivedFunctionStateMultiThrottle(char multiThrottle, uint8_t func, bool state) override;
    void receivedRosterFunctionListMultiThrottle(char multiThrottle, String functions[MAX_FUNCTIONS]) override;
    void receivedTrackPower(TrackPower state) override;
    void receivedRosterEntries(int size) override;
    void receivedRosterEntry(int index, String name, int address, char length) override;
    void receivedTurnoutEntries(int size) override;
    void receivedTurnoutEntry(int index, String sysName, String userName, int state) override;
    void receivedRouteEntries(int size) override;
    void receivedRouteEntry(int index, String sysName, String userName, int state) override;
    void addressStealNeeded(String address, String entry) override;
    void addressStealNeededMultiThrottle(char multiThrottle, String address, String entry) override;
    void receivedUnknownCommand(String unknownCommand) override;
};
