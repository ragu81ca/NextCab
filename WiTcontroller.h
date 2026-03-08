// Prevent multiple inclusion and circular dependency amplification
#pragma once

#include <Keypad.h> // for KeypadEvent typedef
#include <WiThrottleProtocol.h> // bring in Direction, TrackPower, TurnoutAction enums
#include "src/core/UIState.h" // relocated centralized UI state

// Forward declarations to break cyclic dependency (definitions in respective headers)
class ThrottleManager;
class Renderer;

extern ThrottleManager throttleManager;
extern Renderer renderer;
//
// DO NOT alter this file
// 

#define maxFoundWitServers 5     // must be 5 for the moment
// maxRoster / maxTurnoutList / maxRouteList now in ServerDataStore.h (MAX_ROSTER etc.)


extern bool menuCommandStarted;
extern String menuCommand;
// UI state (bridging): map legacy names directly to uiState fields via macros (no separate globals)
#define menuIsShowing uiState.menuIsShowing
#define oledText uiState.lines
#define oledTextInvert uiState.invert


extern TrackPower trackPower;
// turnoutPrefix / routePrefix now in serverDataStore

extern bool circleValues;

extern String selectedSsid;

// WiThrottle server connection state now lives in WiThrottleConnectionManager
// (selectedWitServerIP, foundWitServers*, witServerIpAndPort* removed)

// Found-SSID storage now lives inside WifiSsidManager
extern int ssidSelectionSource;

// witServerIpAndPort* now in WiThrottleConnectionManager

// roster / turnout / route data now in serverDataStore
#include "src/core/ServerDataStore.h"
extern ServerDataStore serverDataStore;
#include "src/core/LocoManager.h"
extern LocoManager locoManager;
// Function arrays migrated to ThrottleManager
// Heartbeat state migrated to HeartbeatMonitor (no longer exposed as globals)
// extern int heartbeatPeriod;
// extern long lastServerResponseTime;
// extern bool heartbeatCheckEnabled;
extern const char* deviceName;
extern const bool encoderRotationClockwiseIsIncreaseSpeed;
extern const bool toggleDirectionOnEncoderButtonPressWhenStationary;
extern int buttonActions[];
extern const String directCommandText[][3];
extern int additionalButtonActions[];

// renderer state caching (used for refresh logic)
#define lastOledScreen uiState.lastScreen
#define lastOledStringParameter uiState.lastStringParam
#define lastOledBoolParameter uiState.lastBoolParam
#define lastOledTurnoutParameter uiState.lastTurnoutParam

// broadcast / server-info overlay state (migrated to UIState)
#define broadcastMessageText uiState.broadcastMessageText
#define broadcastMessageTime uiState.broadcastMessageTime
#define lastReceivingServerDetailsTime uiState.lastReceivingServerDetailsTime

// extern AiEsp32RotaryEncoder rotaryEncoder;

extern WiThrottleProtocol wiThrottleProtocol; // protocol instance
// Additional globals used by Renderer (migrated rendering logic)
extern bool oledDirectCommandsAreBeingDisplayed;
extern bool hashShowsFunctionsInsteadOfKeyDefs;
// getDisplaySpeed now in ThrottleManager, getDisplayLocoString in LocoManager

// function prototypes

// displayUpdateFromWit now in Renderer
// Legacy SSID functions removed; handled by WifiSsidManager

// WiThrottle server connection functions now in WiThrottleConnectionManager

void keypadEvent(KeypadEvent);
void initialiseAdditionalButtons(void);
void additionalButtonLoop(void);

void setup(void);
void loop(void);

void doMenu(void);
void resetMenu(void);

// Speed/direction/power/throttle wrappers eliminated — callers use managers directly
// (ThrottleManager, LocoManager, SystemActionHandler, HeartbeatMonitor)

// OLED helper functions moved to Renderer:
// setAppnameForOled, receivingServerInfoOled, setMenuTextForOled, refreshOled, displayUpdateFromWit

void deepSleepStart();
void deepSleepStart(int);

char getMultiThrottleChar(int);
int getMultiThrottleIndex(char);