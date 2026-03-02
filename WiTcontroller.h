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
#define maxRoster 70     // must be a multiple of rosterItemsPerPage (both 5 and 10)
#define maxTurnoutList 60     // must be a multiple of turnoutItemsPerPage
#define maxRouteList 60     // must be a multiple of routeItemsPerPage


extern bool menuCommandStarted;
extern String menuCommand;
// UI state (bridging): map legacy names directly to uiState fields via macros (no separate globals)
#define menuIsShowing uiState.menuIsShowing
#define oledText uiState.lines
#define oledTextInvert uiState.invert


extern TrackPower trackPower;
extern String turnoutPrefix;
extern String routePrefix;

extern bool circleValues;

extern String selectedSsid;

// WiThrottle server connection state now lives in WiThrottleConnectionManager
// (selectedWitServerIP, foundWitServers*, witServerIpAndPort* removed)

// Found-SSID storage now lives inside WifiSsidManager
extern int ssidSelectionSource;

// witServerIpAndPort* now in WiThrottleConnectionManager

extern int rosterSize;
extern int rosterIndex[]; 
extern String rosterName[]; 
extern int rosterAddress[];
extern char rosterLength[];
extern int rosterSortedIndex[]; // sorted index order

extern int turnoutListSize;
extern int turnoutListIndex[]; 
extern String turnoutListSysName[]; 
extern String turnoutListUserName[];
extern int turnoutListState[];

extern int routeListSize;
extern int routeListIndex[]; 
extern String routeListSysName[]; 
extern String routeListUserName[];
extern int routeListState[];
extern bool functionStates[][MAX_FUNCTIONS];
extern String functionLabels[][MAX_FUNCTIONS];
extern int functionFollow[][MAX_FUNCTIONS];
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

// extern AiEsp32RotaryEncoder rotaryEncoder;

extern WiThrottleProtocol wiThrottleProtocol; // protocol instance
// Additional globals used by Renderer (migrated rendering logic)
extern bool oledDirectCommandsAreBeingDisplayed;
extern bool hashShowsFunctionsInsteadOfKeyDefs;
String getDisplayLocoString(int multiThrottleIndex, int index); // helper for speed screen
int getDisplaySpeed(int multiThrottleIndex);

// function prototypes

void displayUpdateFromWit(int);
// Legacy SSID functions removed; handled by WifiSsidManager

// WiThrottle server connection functions now in WiThrottleConnectionManager

void keypadEvent(KeypadEvent);
void initialiseAdditionalButtons(void);
void additionalButtonLoop(void);

void setup(void);
void loop(void);

void doKeyPress(char, bool);
void doDirectCommand (char, bool);
void doDirectAction(int);
void doMenu(void);
void resetMenu(void);

void resetFunctionStates(int);
void resetFunctionLabels(int); 
void resetAllFunctionLabels(void); 
String getLocoWithLength(String);
void speedEstop(void);
void speedDown(int, int);
void speedUp(int, int);
void speedSet(int, int);
void releaseAllLocos(int);
void releaseOneLocoByIndex(int, int);
void toggleAdditionalMultiplier(void);
void toggleHeartbeatCheck(void);
void toggleDirection(int);
void toggleLocoFacing(int, String);
void changeDirection(int, Direction);

void doDirectFunction(int, bool);
void doDirectFunction(int, bool, bool force);
void doFunction(int, int, bool);
void doFunction(int, int, bool, bool force);

void doFunctionWhichLocosInConsist(int multiThrottleIndex, int functionNumber, bool pressed);
void doFunctionWhichLocosInConsist(int multiThrottleIndex, int functionNumber, bool pressed, bool force);

void powerOnOff(TrackPower);
void powerToggle(void);
void nextThrottle(void);
void reconnect(void);

void selectRoster(int);
void selectTurnoutList(int, TurnoutAction);
void selectRouteList(int);
void selectFunctionList(int);
void selectEditConsistList(int);

// *********************************************************************************
//  oLED functions
// *********************************************************************************

void setAppnameForOled(void);
void receivingServerInfoOled(int, int);
void setMenuTextForOled(int);
void refreshOled();
// Legacy OLED menu/all-locos helpers migrated into Renderer (renderMenu, renderAllLocos)

void deepSleepStart();
void deepSleepStart(int);

char getMultiThrottleChar(int);
int getMultiThrottleIndex(char);