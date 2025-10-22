/**
 * This app turns an ESP32 into a dedicated Model Railway Controller for use
 * with JMRI or any wiThrottle server.

  Instructions:
  - Refer to the readme at https://github.com/flash62au/WiTcontroller
 */

#include <string>

// Use the Arduino IDE 'Boards' Manager to get these libraries
// They will be installed with the 'ESP32' Board library
// DO NOT DOWNLOAD THEM DIRECTLY!!!
#include <WiFi.h>                 // https://github.com/espressif/arduino-esp32/tree/master/libraries/WiFi     GPL 2.1
#include <ESPmDNS.h>              // https://github.com/espressif/arduino-esp32/blob/master/libraries/ESPmDNS  GPL 2.1

// ----------------------

#include <Preferences.h>

// use the Arduino IDE 'Library' Manager to get these libraries
#include <Keypad.h>               // https://www.arduinolibraries.info/libraries/keypad                        GPL 3.0
#include <U8g2lib.h>              // https://github.com/olikraus/u8g2  (Just get "U8g2" via the Arduino IDE Library Manager)   new-bsd
#include <WiThrottleProtocol.h>   // https://github.com/flash62au/WiThrottleProtocol                           Creative Commons 4.0  Attribution-ShareAlike

// Pangodream_18650_CL.h now only needed inside BatteryMonitor implementation

// create these files by copying the example files and editing them as needed
#include "config_network.h"      // LAN networks (SSIDs and passwords)
#include "config_buttons.h"      // keypad buttons assignments
#include "WiTcontroller.h"       // legacy macros & extern mappings (must precede usage of max* constants)

// Battery monitoring moved to BatteryMonitor class (core/BatteryMonitor.*)
#include "src/core/BatteryMonitor.h"
BatteryMonitor batteryMonitor; // encapsulates previous battery globals
// Core managers & devices
#include "src/core/input/InputManager.h"
#include "src/core/input/KeypadInput.h"
#include "src/core/input/RotaryEncoderInput.h"
#include "src/core/input/PotThrottleInput.h"
#include "src/core/input/AdditionalButtonsInput.h"
#include "src/core/input/InputEvents.h"
#include "src/core/ThrottleManager.h"
#include "src/core/network/WifiSsidManager.h"
#include "src/core/OledRenderer.h"
#include "src/core/input/OperationModeHandler.h"
#include "src/core/input/PasswordEntryModeHandler.h"
#include "src/core/input/SystemActionHandler.h"
#include "src/core/protocol/WiThrottleDelegate.h" // ensure debug_print macros before first use

ThrottleManager throttleManager; // speed/direction/throttle index
InputManager inputManager;       // generic input dispatcher
WifiSsidManager wifiSsidManager; // Wi-Fi SSID manager
OledRenderer oledRenderer(u8g2); // OLED renderer instance (requires U8G2 reference)

// ----------------- Legacy global variables (bridging for refactor) -----------------
int keypadUseType = 0;
int encoderUseType = 0;
bool menuCommandStarted = false;
String menuCommand = "";
TrackPower trackPower = PowerOff; // initial power state
String turnoutPrefix = ""; // updated when selecting SSID or server
String routePrefix = "";  // updated when selecting SSID or server

// debug_print / debug_println macros now supplied by WiThrottleDelegate.h

// Throttle pot globals required by PotThrottleInput.cpp (retain legacy semantics)
// TODO: Move these config values to a dedicated Config.h or pass via constructor
int throttlePotPin = 36; // default analog pin (can be overridden later)
bool throttlePotUseNotches = false;
int throttlePotNotchValues[8] = {0,200,400,600,800,1000,2000,4000};
int throttlePotNotchSpeeds[8] = {0,20,40,60,80,100,120,127};

// ----------------- Keypad defaults (if not overridden in config_buttons.h) -----------------
#ifndef ROW_NUM
  #define ROW_NUM 4
#endif
#ifndef COLUMN_NUM
  #define COLUMN_NUM 3
#endif
#ifndef KEYPAD_KEYS
  static char KEYPAD_KEYS[ROW_NUM][COLUMN_NUM] = {
    {'1','2','3'},
    {'4','5','6'},
    {'7','8','9'},
    {'*','0','#'}
  };
#endif
#ifndef KEYPAD_ROW_PINS
  static byte KEYPAD_ROW_PINS[ROW_NUM] = {19,18,17,16};
#endif
#ifndef KEYPAD_COLUMN_PINS
  static byte KEYPAD_COLUMN_PINS[COLUMN_NUM] = {4,0,2};
#endif
#ifndef KEYPAD_DEBOUNCE_TIME
  #define KEYPAD_DEBOUNCE_TIME 10
#endif
#ifndef KEYPAD_HOLD_TIME
  #define KEYPAD_HOLD_TIME 200
#endif
static Keypad keypad(makeKeymap((char*)KEYPAD_KEYS), KEYPAD_ROW_PINS, KEYPAD_COLUMN_PINS, ROW_NUM, COLUMN_NUM);

// ----------------- Rotary / Pot selection flag -----------------
#ifndef USE_ROTARY_ENCODER_FOR_THROTTLE
  #define USE_ROTARY_ENCODER_FOR_THROTTLE true
#endif
bool useRotaryEncoderForThrottle = USE_ROTARY_ENCODER_FOR_THROTTLE;

// ----------------- Startup commands stub -----------------
String startupCommands[4] = {"","","",""};

// Preferences flag forward declaration (needed by menu logic earlier in file)
bool preferencesRead = false;

// Additional buttons device (static storage to avoid heap fragmentation)
static AdditionalButtonsInput additionalButtonsInputInstance([](const InputEvent &ev){ inputManager.dispatch(ev); });
static bool additionalButtonsConfigured = false;

// Configuration mapping old arrays -> new definitions (minimal bridge). We reuse existing arrays if defined.
// Expect: additionalButtonPin[], additionalButtonType[], additionalButtonActions[], maxAdditionalButtons.
struct AdditionalButtonDef buttonDefsStatic[MAX_ADDITIONAL_BUTTONS];
// Instantiate mode handlers
OperationModeHandler operationModeHandler(throttleManager);
static void onPasswordCommit();
PasswordEntryModeHandler passwordModeHandler(32, onPasswordCommit); // arbitrary max len
SystemActionHandler systemActionHandler(throttleManager);

// server variables
// bool ssidConnected = false;
String selectedSsid = "";
String selectedSsidPassword = "";
int ssidConnectionState = CONNECTION_STATE_DISCONNECTED;

// ssid password entry
String ssidPasswordEntered = "";
bool ssidPasswordChanged = true;
char ssidPasswordCurrentChar = ssidPasswordBlankChar; 

IPAddress selectedWitServerIP;
int selectedWitServerPort = 0;
String selectedWitServerName ="";
int noOfWitServices = 0;
int witConnectionState = CONNECTION_STATE_DISCONNECTED;
String serverType = "";

//found wiThrottle servers
IPAddress foundWitServersIPs[maxFoundWitServers];
int foundWitServersPorts[maxFoundWitServers];
String foundWitServersNames[maxFoundWitServers];
int foundWitServersCount = 0;
bool autoConnectToFirstDefinedServer = AUTO_CONNECT_TO_FIRST_DEFINED_SERVER;
bool autoConnectToFirstWiThrottleServer = AUTO_CONNECT_TO_FIRST_WITHROTTLE_SERVER;
int outboundCmdsMininumDelay = OUTBOUND_COMMANDS_MINIMUM_DELAY;
bool commandsNeedLeadingCrLf = SEND_LEADING_CR_LF_FOR_COMMANDS;

//found ssids
String foundSsids[maxFoundSsids];
long foundSsidRssis[maxFoundSsids];
bool foundSsidsOpen[maxFoundSsids];
int foundSsidsCount = 0;
int ssidSelectionSource;
double startWaitForSelection;

// wit Server ip entry
String witServerIpAndPortConstructed = "###.###.###.###:#####";
String witServerIpAndPortEntered = DEFAULT_IP_AND_PORT;
bool witServerIpAndPortChanged = true;

// roster variables
int rosterSize = 0;
int rosterIndex[maxRoster]; 
String rosterName[maxRoster]; 
int rosterAddress[maxRoster];
char rosterLength[maxRoster];
char rosterSortStrings[maxRoster][14]; 
char* rosterSortPointers[maxRoster]; 
int rosterSortedIndex[maxRoster]; 

// migrated: page, functionPage, functionHasBeenSelected now in uiState

// Broadcast msessage
String broadcastMessageText = "";
long broadcastMessageTime = 0;
long lastReceivingServerDetailsTime = 0;

// remember oLED state
// migrated: lastOled* variables now in uiState (except lastOledIntParameter retained locally if still needed)
int lastOledIntParameter = 0; // TODO: consider moving this as presenters evolve

// turnout variables
int turnoutListSize = 0;
int turnoutListIndex[maxTurnoutList]; 
String turnoutListSysName[maxTurnoutList]; 
String turnoutListUserName[maxTurnoutList];
int turnoutListState[maxTurnoutList];

// route variables
int routeListSize = 0;
int routeListIndex[maxRouteList]; 
String routeListSysName[maxRouteList]; 
String routeListUserName[maxRouteList];
int routeListState[maxRouteList];

// function states
bool functionStates[6][MAX_FUNCTIONS];   // set to maximum possible (6 throttles)

// function labels
String functionLabels[6][MAX_FUNCTIONS];   // set to maximum possible (6 throttles)

// consist function follow
int functionFollow[6][MAX_FUNCTIONS];   // set to maximum possible (6 throttles)

// speedstep
// migrated to ThrottleManager: int currentSpeedStep[6];

// throttle
// (moved to ThrottleManager) currentThrottleIndex / currentThrottleIndexChar / maxThrottles

// Heartbeat monitoring now encapsulated
#include "src/core/heartbeat/HeartbeatMonitor.h"
HeartbeatMonitor heartbeatMonitor; // manages heartbeat period / last response / enabled flag (global for extern linkage)

// used to stop speed bounces
// (moved to ThrottleManager) lastSpeedSentTime / lastSpeedSent / lastSpeedThrottleIndex

bool dropBeforeAcquire = DROP_BEFORE_ACQUIRE;

// don't alter the assignments here
// alter them in config_buttons.h


// rotary encoder debounce variables removed (handled inside RotaryEncoderInput)

const bool encoderRotationClockwiseIsIncreaseSpeed = ENCODER_ROTATION_CLOCKWISE_IS_INCREASE_SPEED;
// false = Counterclockwise  true = clockwise

const bool toggleDirectionOnEncoderButtonPressWhenStationary = TOGGLE_DIRECTION_ON_ENCODER_BUTTON_PRESSED_WHEN_STATIONAY;
// true = if the locos(s) are stationary, clicking the encoder button will toggle the direction

//4x3 keypad only uses 0-9
//4x4 uses all 14 
int buttonActions[14] = { CHOSEN_KEYPAD_0_FUNCTION,
                          CHOSEN_KEYPAD_1_FUNCTION,
                          CHOSEN_KEYPAD_2_FUNCTION,
                          CHOSEN_KEYPAD_3_FUNCTION,
                          CHOSEN_KEYPAD_4_FUNCTION,
                          CHOSEN_KEYPAD_5_FUNCTION,
                          CHOSEN_KEYPAD_6_FUNCTION,
                          CHOSEN_KEYPAD_7_FUNCTION,
                          CHOSEN_KEYPAD_8_FUNCTION,
                          CHOSEN_KEYPAD_9_FUNCTION,
                          CHOSEN_KEYPAD_A_FUNCTION,
                          CHOSEN_KEYPAD_B_FUNCTION,
                          CHOSEN_KEYPAD_C_FUNCTION,
                          CHOSEN_KEYPAD_D_FUNCTION
};

// text that will appear when you press #
const String directCommandText[4][3] = {
    {CHOSEN_KEYPAD_1_DISPLAY_NAME, CHOSEN_KEYPAD_2_DISPLAY_NAME, CHOSEN_KEYPAD_3_DISPLAY_NAME},
    {CHOSEN_KEYPAD_4_DISPLAY_NAME, CHOSEN_KEYPAD_5_DISPLAY_NAME, CHOSEN_KEYPAD_6_DISPLAY_NAME},
    {CHOSEN_KEYPAD_7_DISPLAY_NAME, CHOSEN_KEYPAD_8_DISPLAY_NAME, CHOSEN_KEYPAD_9_DISPLAY_NAME},
    {CHOSEN_KEYPAD_DISPLAY_PREFIX, CHOSEN_KEYPAD_0_DISPLAY_NAME, CHOSEN_KEYPAD_DISPLAY_SUFIX}
};

bool oledDirectCommandsAreBeingDisplayed = false;
#ifdef HASH_SHOWS_FUNCTIONS_INSTEAD_OF_KEY_DEFS
  bool hashShowsFunctionsInsteadOfKeyDefs = HASH_SHOWS_FUNCTIONS_INSTEAD_OF_KEY_DEFS;
#else
  bool hashShowsFunctionsInsteadOfKeyDefs = false;
#endif

//optional additional buttons
#if !USE_NEW_ADDITIONAL_BUTTONS_FORMAT
  int maxAdditionalButtons = MAX_ADDITIONAL_BUTTONS;
  int additionalButtonPin[MAX_ADDITIONAL_BUTTONS] =          ADDITIONAL_BUTTONS_PINS;
  int additionalButtonType[MAX_ADDITIONAL_BUTTONS] =         ADDITIONAL_BUTTONS_TYPE;
  int additionalButtonActions[MAX_ADDITIONAL_BUTTONS] = {
                            CHOSEN_ADDITIONAL_BUTTON_0_FUNCTION,
                            CHOSEN_ADDITIONAL_BUTTON_1_FUNCTION,
                            CHOSEN_ADDITIONAL_BUTTON_2_FUNCTION,
                            CHOSEN_ADDITIONAL_BUTTON_3_FUNCTION,
                            CHOSEN_ADDITIONAL_BUTTON_4_FUNCTION,
                            CHOSEN_ADDITIONAL_BUTTON_5_FUNCTION,
                            CHOSEN_ADDITIONAL_BUTTON_6_FUNCTION
  };
  int additionalButtonLatching[MAX_ADDITIONAL_BUTTONS] = {
                            ADDITIONAL_BUTTON_0_LATCHING,
                            ADDITIONAL_BUTTON_1_LATCHING,
                            ADDITIONAL_BUTTON_2_LATCHING,
                            ADDITIONAL_BUTTON_3_LATCHING,
                            ADDITIONAL_BUTTON_4_LATCHING,
                            ADDITIONAL_BUTTON_5_LATCHING,
                            ADDITIONAL_BUTTON_6_LATCHING
  };

  unsigned long lastAdditionalButtonDebounceTime[MAX_ADDITIONAL_BUTTONS];  // the last time the output pin was toggled
  bool additionalButtonRead[MAX_ADDITIONAL_BUTTONS];
  bool additionalButtonLastRead[MAX_ADDITIONAL_BUTTONS];

#else
  #if NEW_MAX_ADDITIONAL_BUTTONS>0
    int maxAdditionalButtons = NEW_MAX_ADDITIONAL_BUTTONS;
    int additionalButtonPin[NEW_MAX_ADDITIONAL_BUTTONS] = NEW_ADDITIONAL_BUTTON_PIN;
    int additionalButtonType[NEW_MAX_ADDITIONAL_BUTTONS] = NEW_ADDITIONAL_BUTTON_TYPE;
    int additionalButtonActions[NEW_MAX_ADDITIONAL_BUTTONS] = NEW_ADDITIONAL_BUTTON_ACTIONS;
    int additionalButtonLatching[NEW_MAX_ADDITIONAL_BUTTONS] = NEW_ADDITIONAL_BUTTON_LATCHING;

    unsigned long lastAdditionalButtonDebounceTime[NEW_MAX_ADDITIONAL_BUTTONS];  // the last time the output pin was toggled
    bool additionalButtonRead[NEW_MAX_ADDITIONAL_BUTTONS];
    bool additionalButtonLastRead[NEW_MAX_ADDITIONAL_BUTTONS];
  #else
    int maxAdditionalButtons = 0;
    int additionalButtonPin[1] = {-1};
    int additionalButtonType[1] = {INPUT_PULLUP};
    int additionalButtonActions[1] = {FUNCTION_NULL};
    int additionalButtonLatching[1] = {false};

    unsigned long lastAdditionalButtonDebounceTime[1];  // the last time the output pin was toggled
    bool additionalButtonRead[1];
    bool additionalButtonLastRead[1];
  #endif
#endif

bool additionalButtonOverrideDefaultLatching = ADDITIONAL_BUTTON_OVERRIDE_DEFAULT_LATCHING;
unsigned long additionalButtonDebounceDelay = ADDITIONAL_BUTTON_DEBOUNCE_DELAY;    // the debounce time

// *********************************************************************************

void displayUpdateFromWit(int multiThrottleIndex) {
  debug_print("displayUpdateFromWit(): keyapdeUseType "); debug_print(keypadUseType); 
  debug_print(" menuIsShowing "); debug_print(menuIsShowing);
  debug_print(" multiThrottleIndex "); debug_print(multiThrottleIndex);
  debug_println("");
  if ( (keypadUseType == KEYPAD_USE_OPERATION) && (!menuIsShowing) 
  && (multiThrottleIndex==throttleManager.getCurrentThrottleIndex()) ) {
  oledRenderer.renderSpeed();
  }
}

#include "src/core/protocol/WiThrottleDelegate.h"
static WiThrottleDelegate myDelegate; // delegate class (file renamed)

int getMultiThrottleIndex(char multiThrottle) {
    int mThrottle = multiThrottle - '0';
    if ((mThrottle >= 0) && (mThrottle<=5)) {
        return mThrottle;
    } else {
        return 0;
    }
}

char getMultiThrottleChar(int multiThrottleIndex) {
  return '0' + multiThrottleIndex;
}

// Convenience wrapper: steal the specified locomotive address for the current throttle.
// Reduces repeated calls passing throttleManager.getCurrentThrottleChar() explicitly.
void stealCurrentLoco(String address) {
  stealLoco(throttleManager.getCurrentThrottleChar(), address);
}

WiFiClient client;
WiThrottleProtocol wiThrottleProtocol;
// Identity components based on MAC (last 4 hex chars)
static String macLast4;        // e.g. "EEFF"
String wifiHostname;    // appName + "-" + macLast4 (exposed)
static String witDeviceId;     // macLast4 used as WiThrottle device ID

static void computeIdentityFromMac() {
  String macStr = WiFi.macAddress(); // Expected format "AA:BB:CC:DD:EE:FF"
  if (macStr.length() == 17) {
    int lastSep = macStr.lastIndexOf(':');
    String byte5 = macStr.substring(lastSep-2, lastSep);
    String byte6 = macStr.substring(lastSep+1);
    macLast4 = byte5 + byte6;
    macLast4.toUpperCase();
    debug_print("computeIdentityFromMac(): MAC via WiFi.macAddress() -> "); debug_println(macLast4);
  } else {
    macLast4 = "0000";
    debug_print("computeIdentityFromMac(): macAddress invalid ('"); debug_print(macStr); debug_println("'), using 0000");
  }
  wifiHostname = appName + "-" + macLast4;
  witDeviceId = macLast4;
}
// (No stray brace here)

// *********************************************************************************
// WiThrottle 
// *********************************************************************************

void witServiceLoop() {
  if (witConnectionState == CONNECTION_STATE_DISCONNECTED) {
    browseWitService(); 
  }

  if (witConnectionState == CONNECTION_STATE_ENTRY_REQUIRED) {
    enterWitServer();
  }

  if ( (witConnectionState == CONNECTION_STATE_SELECTED) 
  || (witConnectionState == CONNECTION_STATE_ENTERED) ) {
    connectWitServer();
  }
}

// (Removed misplaced global-level identity initialization; now performed inside setup())

void browseWitService() {
  debug_println("browseWitService()");

  keypadUseType = KEYPAD_USE_SELECT_WITHROTTLE_SERVER;

  double startTime = millis();
  double nowTime = startTime;

  const char * service = "withrottle";
  const char * proto= "tcp";

  debug_printf("Browsing for service _%s._%s.local. on %s ... ", service, proto, selectedSsid.c_str());
  clearOledArray(); 
  oledText[0] = appName; oledText[6] = appVersion; 
  oledText[1] = selectedSsid;   oledText[2] = MSG_BROWSING_FOR_SERVICE;
  oledRenderer.renderBattery();
  oledRenderer.renderArray(false, false, true, true);
  
  startWaitForSelection = millis();

  noOfWitServices = 0;
  if ( (selectedSsid.substring(0,6) == "DCCEX_") && (selectedSsid.length()==12) ) {
    debug_println(MSG_BYPASS_WIT_SERVER_SEARCH);
    oledText[1] = MSG_BYPASS_WIT_SERVER_SEARCH;
  oledRenderer.renderBattery();
  oledRenderer.renderArray(false, false, true, true);
    delay(500);
  } else {
    int j = 0;
    while ( (noOfWitServices == 0) 
    && ((nowTime-startTime) <= 10000)) { // try for 10 seconds 
      noOfWitServices = MDNS.queryService(service, proto);
      oledText[3] = getDots(j);
  oledRenderer.renderBattery();
  oledRenderer.renderArray(false, false, true, true);
      j++;
      debug_print(".");
      nowTime = millis();
    }
    debug_println("");
  }
  

  foundWitServersCount = noOfWitServices;
  if (noOfWitServices > 0) {
    for (int i = 0; ((i < noOfWitServices) && (i<maxFoundWitServers)); ++i) {
      foundWitServersNames[i] = MDNS.hostname(i);
      // foundWitServersIPs[i] = MDNS.IP(i);
      foundWitServersIPs[i] = ESPMDNS_IP_ATTRIBUTE_NAME;
      foundWitServersPorts[i] = MDNS.port(i);
      // debug_print("txt 0: key: "); debug_print(MDNS.txtKey(i,0)); debug_print(" value: '"); debug_print(MDNS.txt(i,0)); debug_println("'");
      // debug_print("txt 1: key: "); debug_print(MDNS.txtKey(i,1)); debug_print(" value: '"); debug_print(MDNS.txt(i,1)); debug_println("'");
      // debug_print("txt 2: key: "); debug_print(MDNS.txtKey(i,2)); debug_print(" value: '"); debug_print(MDNS.txt(i,2)); debug_println("'");
      // debug_print("txt 3: key: "); debug_print(MDNS.txtKey(i,3)); debug_print(" value: '"); debug_println(MDNS.txt(i,3)); debug_println("'");
      if (MDNS.hasTxt(i,"jmri")) {
        String node = MDNS.txt(i,"node");
        node.toLowerCase();
        if (foundWitServersNames[i].equals(node)) {
          foundWitServersNames[i] = "JMRI  (v" + MDNS.txt(i,"jmri") + ")";
        }
      }
    }
  }
  if ( (selectedSsid.substring(0,6) == "DCCEX_") && (selectedSsid.length()==12) ) {
    foundWitServersIPs[foundWitServersCount].fromString("192.168.4.1");
    foundWitServersPorts[foundWitServersCount] = 2560;
    foundWitServersNames[foundWitServersCount] = MSG_GUESSED_EX_CS_WIT_SERVER;
    foundWitServersCount++;
  }

  if (foundWitServersCount == 0) {
    oledText[1] = MSG_NO_SERVICES_FOUND;
  oledRenderer.renderBattery();
  oledRenderer.renderArray(false, false, true, true);
    debug_println(oledText[1]);
    delay(1000);
    buildWitEntry();
    witConnectionState = CONNECTION_STATE_ENTRY_REQUIRED;
  
  } else {
    debug_print(noOfWitServices);  debug_println(MSG_SERVICES_FOUND);
    clearOledArray(); oledText[3] = MSG_SERVICES_FOUND;

    for (int i = 0; i < foundWitServersCount; ++i) {
      // Print details for each service found
      debug_print("  "); debug_print(i); debug_print(": '"); debug_print(foundWitServersNames[i]);
      debug_print("' ("); debug_print(foundWitServersIPs[i]); debug_print(":"); debug_print(foundWitServersPorts[i]); debug_println(")");
      if (i<5) {  // only have room for 5
        String truncatedIp = ".." + foundWitServersIPs[i].toString().substring(foundWitServersIPs[i].toString().lastIndexOf("."));
        oledText[i] = String(i) + ": " + truncatedIp + ":" + String(foundWitServersPorts[i]) + " " + foundWitServersNames[i];
      }
    }

    if (foundWitServersCount > 0) {
      // oledText[5] = menu_select_wit_service;
      setMenuTextForOled(menu_select_wit_service);
    }
  oledRenderer.renderArray(false, false);

    if ( (foundWitServersCount == 1) && (autoConnectToFirstWiThrottleServer) ) {
      debug_println("WiT Selection - only 1");
      selectedWitServerIP = foundWitServersIPs[0];
      selectedWitServerPort = foundWitServersPorts[0];
      selectedWitServerName = foundWitServersNames[0];
      witConnectionState = CONNECTION_STATE_SELECTED;
    } else {
      debug_println("WiT Selection required");
      witConnectionState = CONNECTION_STATE_SELECTION_REQUIRED;
    }
  }
}

void selectWitServer(int selection) {
  debug_print("selectWitServer() "); debug_println(selection);

  if ((selection>=0) && (selection < foundWitServersCount)) {
    witConnectionState = CONNECTION_STATE_SELECTED;
    selectedWitServerIP = foundWitServersIPs[selection];
    selectedWitServerPort = foundWitServersPorts[selection];
    selectedWitServerName = foundWitServersNames[selection];
    keypadUseType = KEYPAD_USE_OPERATION;
  }
}

void connectWitServer() {
  // Pass the delegate instance to wiThrottleProtocol
  wiThrottleProtocol.setDelegate(&myDelegate);
#if WITHROTTLE_PROTOCOL_DEBUG == 0
  wiThrottleProtocol.setLogStream(&Serial);
  wiThrottleProtocol.setLogLevel(DEBUG_LEVEL);
#endif

  debug_println("Connecting to the server...");
  clearOledArray(); 
  setAppnameForOled(); 
  oledText[1] = "        " + selectedWitServerIP.toString() + " : " + String(selectedWitServerPort); 
  oledText[2] = "        " + selectedWitServerName; oledText[3] + MSG_CONNECTING;
  oledRenderer.renderBattery();
  oledRenderer.renderArray(false, false, true, true);
  
  startWaitForSelection = millis();

  if (!client.connect(selectedWitServerIP, selectedWitServerPort)) {
    debug_println(MSG_CONNECTION_FAILED);
    oledText[3] = MSG_CONNECTION_FAILED;
  oledRenderer.renderArray(false, false, true, true);
    delay(5000);
    
    witConnectionState = CONNECTION_STATE_DISCONNECTED;
    ssidConnectionState = CONNECTION_STATE_DISCONNECTED;
    ssidSelectionSource = SSID_CONNECTION_SOURCE_LIST;
    witServerIpAndPortChanged = true;

  } else {
    debug_print("Connected to server: ");   debug_println(selectedWitServerIP); debug_println(selectedWitServerPort);

    // Pass the communication to WiThrottle. + Set the mimimum period between sent commands
    wiThrottleProtocol.connect(&client, outboundCmdsMininumDelay);
    debug_println("WiThrottle connected");

  wiThrottleProtocol.setDeviceName(appName);  // Device name = Application Name
  wiThrottleProtocol.setDeviceID(witDeviceId); // Device ID = last 4 hex chars of MAC
  debug_print("WiThrottle Device Name: "); debug_println(appName);
  debug_print("WiThrottle Device ID: "); debug_println(witDeviceId);
    wiThrottleProtocol.setCommandsNeedLeadingCrLf(commandsNeedLeadingCrLf);
    if (HEARTBEAT_ENABLED) {
      wiThrottleProtocol.requireHeartbeat(true);
    }

    witConnectionState = CONNECTION_STATE_CONNECTED;
    // Heartbeat: we rely on WiThrottleProtocol internal checkHeartbeat() to send '*' periodically.
    // Client does NOT enforce a timeout; the command station will disconnect us if it stops hearing heartbeats.

    oledText[3] = MSG_CONNECTED;
    if (!hashShowsFunctionsInsteadOfKeyDefs) {
      // oledText[5] = menu_menu;
      setMenuTextForOled(menu_menu);
    } else {
      // oledText[5] = menu_menu_hash_is_functions;
      setMenuTextForOled(menu_menu_hash_is_functions);
    }
  oledRenderer.renderArray(false, false, true, true);
  oledRenderer.renderBattery();
    u8g2.sendBuffer();

    keypadUseType = KEYPAD_USE_OPERATION;

    doStartupCommands();
  }
}

void enterWitServer() {
  keypadUseType = KEYPAD_USE_ENTER_WITHROTTLE_SERVER;
  if (witServerIpAndPortChanged) { // don't refresh the screen if nothing nothing has changed
    debug_println("enterWitServer()");
    clearOledArray(); 
    setAppnameForOled(); 
    oledText[1] = MSG_NO_SERVICES_FOUND_ENTRY_REQUIRED;
    oledText[3] = witServerIpAndPortConstructed;
    // oledText[5] = menu_select_wit_entry;
      setMenuTextForOled(menu_select_wit_entry);
  oledRenderer.renderArray(false, false, true, true);
    witServerIpAndPortChanged = false;
  }
}

void disconnectWitServer() {
  debug_println("disconnectWitServer()");
  for (int i=0; i<throttleManager.getMaxThrottles(); i++) {
    releaseAllLocos(i);
  }
  wiThrottleProtocol.disconnect();
  debug_println("Disconnected from wiThrottle server\n");
  clearOledArray(); oledText[0] = MSG_DISCONNECTED;
  oledRenderer.renderArray(false, false, true, true);
  witConnectionState = CONNECTION_STATE_DISCONNECTED;
  witServerIpAndPortChanged = true;
}

void witEntryAddChar(char key) {
  if (witServerIpAndPortEntered.length() < 17) {
    witServerIpAndPortEntered = witServerIpAndPortEntered + key;
    debug_print("wit entered: ");
    debug_println(witServerIpAndPortEntered);
    buildWitEntry();
    witServerIpAndPortChanged = true;
  }
}

void witEntryDeleteChar(char key) {
  if (witServerIpAndPortEntered.length() > 0) {
    witServerIpAndPortEntered = witServerIpAndPortEntered.substring(0, witServerIpAndPortEntered.length()-1);
    debug_print("wit deleted: ");
    debug_println(witServerIpAndPortEntered);
    buildWitEntry();
    witServerIpAndPortChanged = true;
  }
}

// Legacy ssidPasswordAddChar / ssidPasswordDeleteChar removed; handled via PasswordEntryModeHandler events.

void buildWitEntry() {
  debug_println("buildWitEntry()");
  witServerIpAndPortConstructed = "";
  for (int i=0; i < witServerIpAndPortEntered.length(); i++) {
    if ( (i==3) || (i==6) || (i==9) ) {
      witServerIpAndPortConstructed = witServerIpAndPortConstructed + ".";
    } else if (i==12) {
      witServerIpAndPortConstructed = witServerIpAndPortConstructed + ":";
    }
    witServerIpAndPortConstructed = witServerIpAndPortConstructed + witServerIpAndPortEntered.substring(i,i+1);
  }
  debug_print("wit Constructed: ");
  debug_println(witServerIpAndPortConstructed);
  if (witServerIpAndPortEntered.length() < 17) {
    witServerIpAndPortConstructed = witServerIpAndPortConstructed + witServerIpAndPortEntryMask.substring(witServerIpAndPortConstructed.length());
  }
  debug_print("wit Constructed: ");
  debug_println(witServerIpAndPortConstructed);

  if (witServerIpAndPortEntered.length() == 17) {
     selectedWitServerIP.fromString( witServerIpAndPortConstructed.substring(0,15));
     selectedWitServerPort = witServerIpAndPortConstructed.substring(16).toInt();
  }
}

// *********************************************************************************
//   Preferences (moved to PreferencesManager)
//   Legacy function names kept as thin wrappers for now for minimal intrusion.

#include "src/core/PreferencesManager.h"
PreferencesManager preferencesManager;

void setupPreferences(bool forceClear) { preferencesManager.begin(forceClear); }
void readPreferences() { preferencesManager.restoreLocos(wiThrottleProtocol); }
void writePreferences() { preferencesManager.saveLocos(wiThrottleProtocol, throttleManager.getMaxThrottles()); }
void clearPreferences() { preferencesManager.clear(); }


// *********************************************************************************
//   Rotary Encoder (moved into RotaryEncoderInput abstraction)
// *********************************************************************************

// rotary_onButtonClick removed; handled directly by RotaryEncoderInput abstraction
// encoderSpeedChange removed (handled by unified throttle input manager now)

// *********************************************************************************
//   Throttle Pot
// *********************************************************************************


// Battery monitoring now called directly at setup and in main loop (wrapper removed)

// *********************************************************************************
//   keypad (legacy listener removed; events bridged via InputManager dispatch)
// *********************************************************************************


// *********************************************************************************
//   Optional Additional Buttons
// *********************************************************************************

void initialiseAdditionalButtons() {
  // Build definitions for unified producer
  int count = maxAdditionalButtons;
  for (int i = 0; i < count; i++) {
    AdditionalButtonDef def; 
    def.pin = additionalButtonPin[i];
    def.mode = additionalButtonType[i];
    def.actionCode = additionalButtonActions[i];
    // Derive toggle / oneShot from action code ranges (legacy heuristic):
    // Example: direct commands < 100, functions 100-199, toggles 500+, adjust as needed.
    def.toggle = (additionalButtonActions[i] >= 500 && additionalButtonActions[i] < 600);
    def.oneShot = (additionalButtonActions[i] >= 600); // arbitrary mapping; refine later
    buttonDefsStatic[i] = def;
    if (additionalButtonActions[i] != FUNCTION_NULL) {
      debug_print("[AdditionalButtonsInput] Config index "); debug_print(i); debug_print(" pin:"); debug_print(def.pin); debug_print(" action:"); debug_println(def.actionCode);
    }
  }
  additionalButtonsInputInstance.begin(buttonDefsStatic, maxAdditionalButtons);
  inputManager.registerDevice(&additionalButtonsInputInstance);
  additionalButtonsConfigured = true;
}

void additionalButtonLoop() {
  // Legacy loop retained for compatibility during transition; now delegated to unified producer.
  if (additionalButtonsConfigured) { /* now polled via inputManager.pollAllDevices() */ }
}

// *********************************************************************************
//  Setup and Loop
// *********************************************************************************

void setup() {
  Serial.begin(115200);
  // u8g2.setI2CAddress(0x3C * 2);
  // u8g2.setBusClock(100000);
  u8g2.begin();
  u8g2.firstPage();

  delay(1000);
  debug_println("Start"); 
  debug_print("WiTcontroller - Version: "); debug_println(appVersion);

  batteryMonitor.begin();
  // Initial battery service
  batteryMonitor.loop();
  if (batteryMonitor.shouldSleepForLowBattery()) { deepSleepStart(SLEEP_REASON_BATTERY); }

  clearOledArray(); oledText[0] = appName; oledText[6] = appVersion; oledText[2] = MSG_START;
  oledRenderer.renderBattery();
  oledRenderer.renderArray(false, false, true, true);

  // Rotary initialization now handled by RotaryEncoderInput begin() if rotary selected.

  // Keypad event listener removed; KeypadInput now polls press/release states
  keypad.setDebounceTime(KEYPAD_DEBOUNCE_TIME);
  keypad.setHoldTime(KEYPAD_HOLD_TIME);

  esp_sleep_enable_ext0_wakeup(GPIO_NUM_13,0); //1 = High, 0 = Low

  keypadUseType = KEYPAD_USE_SELECT_SSID;
  encoderUseType = ENCODER_USE_OPERATION;
  ssidSelectionSource = SSID_CONNECTION_SOURCE_BROWSE;

  initialiseAdditionalButtons();

  resetAllFunctionLabels();
  resetAllFunctionFollow();

  // migrated arrays initialized inside throttleManager.begin(); legacy loop retained as no-op
  for (int i=0; i< 6; i++) { /* no-op */ }

  // Initialize new ThrottleManager (modular refactor)
  throttleManager.begin(&wiThrottleProtocol);
  // Wire up generic input manager mode & action handlers
  inputManager.setOperationHandler(&operationModeHandler);
  inputManager.setPasswordHandler(&passwordModeHandler);
  inputManager.setActionFallbackHandler(&systemActionHandler);
  inputManager.forceMode(InputMode::Operation); // ensure active_ bound even if mode already Operation
  // Initialize HeartbeatMonitor with defaults
  heartbeatMonitor.begin(DEFAULT_HEARTBEAT_PERIOD, HEARTBEAT_ENABLED); // Slim wrapper: enabled + period only
  heartbeatMonitor.setOnPeriodChange([](unsigned long p){
    debug_printf("Heartbeat period updated by server: %lu seconds\n", p);
  });
  // Register rotary or pot device depending on configuration
  if (useRotaryEncoderForThrottle) {
    static RotaryEncoderInput rotaryDev([&](const InputEvent &ev){ inputManager.dispatch(ev); });
    inputManager.registerDevice(&rotaryDev);
    rotaryDev.begin();
  } else {
    static PotThrottleInput potDev([&](const InputEvent &ev){ inputManager.dispatch(ev); });
    inputManager.registerDevice(&potDev);
    potDev.begin();
  }
  // Register keypad device wrapper
  static KeypadInput keypadDev(keypad, [&](const InputEvent &ev){ inputManager.dispatch(ev); });
  inputManager.registerDevice(&keypadDev);
  keypadDev.begin();

  // Initialize WifiSsidManager (Phase 1): turnout/route prefixes remain global for now.
  wifiSsidManager.begin(ssids, passwords, maxSsids);
  
  // Ensure station mode so MAC retrieval succeeds before connect.
  WiFi.mode(WIFI_STA); // still set STA mode for consistency
  // Derive identity (MAC-based) and set WiFi hostname BEFORE WiFi connect attempts.
  computeIdentityFromMac(); // derive macLast4 / wifiHostname / witDeviceId
  WiFi.setHostname(wifiHostname.c_str());
  debug_print("Hostname set to: "); debug_println(wifiHostname);
  // Country code feature removed with esp_wifi include elimination; reintroduce if regulatory needs arise.
}

void loop() {
  
  if (ssidConnectionState != CONNECTION_STATE_CONNECTED) {
    wifiSsidManager.loop();
    // Password entry input mode now internalized; ensureInputModeForPassword removed.
    checkForShutdownOnNoResponse();
  } else {  
    if (witConnectionState != CONNECTION_STATE_CONNECTED) {
      witServiceLoop();
      checkForShutdownOnNoResponse();
    } else {
      wiThrottleProtocol.check();    // parse incoming messages
  // Slim heartbeat monitor: no activity tracking or timeout logic; library handles periodic sends.
    }
  }
  // Unified device polling (rotary/pot, keypad, additional buttons)
  inputManager.pollAllDevices();

  if (batteryMonitor.enabled()) {
    batteryMonitor.loop();
    if (batteryMonitor.shouldSleepForLowBattery()) { deepSleepStart(SLEEP_REASON_BATTERY); }
  }

	// debug_println("loop:" );
}

// *********************************************************************************
//  Key press and Menu
// *********************************************************************************

void doKeyPress(char key, bool pressed) {
    debug_print("doKeyPress(): key: "); debug_print(key); debug_print(" keypadUseType: ");debug_println(keypadUseType);

  if (pressed)  { //pressed
    debug_println("doKeyPress(): pressed"); 
    switch (keypadUseType) {
      case KEYPAD_USE_OPERATION:
        debug_print("doKeyPress(): key operation... "); debug_println(key);

        switch (key) {
          case '*':  // menu command
            menuCommand = "";
            if (menuCommandStarted) { // then cancel the menu
              resetMenu();
              oledRenderer.renderSpeed();
            } else {
              menuCommandStarted = true;
              debug_println("doKeyPress(): Command started");
              oledRenderer.renderMenu("", true);
            }
            break;

          case '#': // end of command
            debug_print("doKeyPress(): end of command... "); debug_print(key); debug_print ("  :  "); debug_println(menuCommand);
            if ((menuCommandStarted) && (menuCommand.length()>=1)) {
              doMenu();
            } else {
              if (!hashShowsFunctionsInsteadOfKeyDefs) {
                if (!oledDirectCommandsAreBeingDisplayed) {
                  oledRenderer.renderDirectCommands();
                } else {
                  oledDirectCommandsAreBeingDisplayed = false;
                  oledRenderer.renderSpeed();
                }
              } else {
                oledRenderer.renderFunctionList(""); 
              }
            }
            break;

          case '0': case '1': case '2': case '3': case '4': 
          case '5': case '6': case '7': case '8': case '9': 
          {
            debug_print("doKeyPress(): number... "); debug_print(key); 
            debug_print ("  cmd: '"); debug_println(menuCommand); 
            debug_print("' : "); debug_println(menuCharsRequired[menuCommand.substring(0,1).toInt()]);

            int index0 = key-48;
            int index1 = 0;
            if (menuCommand.length() >= 0) index1 = menuCommand.charAt(0)-48;
            int index2 = 0;
            if (menuCommand.length() >= 1) index2 = menuCommand.charAt(1)-48;

            if (menuCommandStarted) { // append to the string

              // if ( ((menuCommand.length() == 0) && (menuCharsRequired[key-48] == MENU_ITEM_TYPE_DIRECT_COMMAND))
              // || ((menuCommand.length() == 1) && (menuCharsRequired[menuCommand.charAt(0)-48] == MENU_ITEM_TYPE_SUB_MENU))
              // || ((menuCommand.length() == 1) && (menuCharsRequired[menuCommand.charAt(0)-48] == MENU_ITEM_TYPE_DIRECT_COMMAND))
              // || ((menuCommand.length() == 2) && (menuCharsRequired[menuCommand.charAt(1)-38] == MENU_ITEM_TYPE_DIRECT_COMMAND)) ) { 

              if ( ((menuCommand.length() == 0) 
                    && (menuCharsRequired[index0] == MENU_ITEM_TYPE_DIRECT_COMMAND))
              || ((menuCommand.length() == 1) 
                    &&  ( (menuCharsRequired[index1] == MENU_ITEM_TYPE_SUB_MENU)
                       || (menuCharsRequired[index1] == MENU_ITEM_TYPE_DIRECT_COMMAND) ))
              || ((menuCommand.length() == 2) 
                    &&  (menuCharsRequired[index1] != MENU_ITEM_TYPE_ONE_OR_MORE_CHARS)
                    &&  (menuCharsRequired[index2] == MENU_ITEM_TYPE_DIRECT_COMMAND)) 
              ) { 
                debug_println("doKeyPress(): MENU_ITEM_TYPE_DIRECT_COMMAND : ");
                menuCommand += key;
                doMenu();
              } else {
                if ((menuCharsRequired[index0] == MENU_ITEM_TYPE_SUB_MENU) 
                && (menuCommand.length() == 0)) {  // menu type needs only one char
                  debug_println("doKeyPress(): MENU_ITEM_TYPE_SUB_MENU : "); 
                  menuCommand += key;
                  oledRenderer.renderMenu(menuCommand, false);
                // } else if ( (menuCharsRequired[index0] == MENU_ITEM_TYPE_SELECT_FROM_LIST) 
                //           && (menuCommand.length() == 0)) {  // menu type needs only one char
                //   debug_println("doKeyPress(): MENU_ITEM_TYPE_SELECT_FROM_LIST : "); 
                //   menuCommand += key;
                //   writeOledMenu(menuCommand, false);
                } else {  //menu type allows/requires more than one char
                  debug_println("doKeyPress(): MENU_ITEM_TYPE_ONE_OR_MORE_CHARS");
                  menuCommand += key;
                  oledRenderer.renderMenu(menuCommand, true);
                }
              }
            } else {
              doDirectCommand(key, true);
            }
            break;
          }
          default:  // A, B, C, D
            doDirectCommand(key, true);
            break;
        }
        break;

      case KEYPAD_USE_ENTER_WITHROTTLE_SERVER:
        debug_print("doKeyPress(): key: Enter wit... "); debug_println(key);
        switch (key){
          case '0': case '1': case '2': case '3': case '4': 
          case '5': case '6': case '7': case '8': case '9':
            witEntryAddChar(key);
            break;
          case '*': // backspace
            witEntryDeleteChar(key);
            break;
          case '#': // end of command
            if (witServerIpAndPortEntered.length() == 17) {
              witConnectionState = CONNECTION_STATE_ENTERED;
            }
            break;
          default:  // do nothing 
            break;
        }

        break;

      case KEYPAD_USE_ENTER_SSID_PASSWORD: {
        debug_print("doKeyPress(): key: Enter password... "); debug_println(key);
        if (pressed) {
          InputEvent ev; ev.timestamp = millis();
          if (key == '#') {
            ev.type = InputEventType::PasswordCommit; ev.ivalue = 0; ev.cvalue = '#';
          } else if (key == '*') {
            ev.type = InputEventType::KeypadSpecial; ev.ivalue = 0; ev.cvalue = '*';
          } else if (key >= '0' && key <= '9') {
            ev.type = InputEventType::KeypadChar; ev.ivalue = key - '0'; ev.cvalue = key;
          } else {
            ev.type = InputEventType::KeypadSpecial; ev.ivalue = 0; ev.cvalue = key;
          }
          inputManager.dispatch(ev);
        }
        break; }

      case KEYPAD_USE_SELECT_WITHROTTLE_SERVER:
        debug_print("doKeyPress(): key: Select wit... "); debug_println(key);
        switch (key){
          case '0': case '1': case '2': case '3': case '4':
            selectWitServer(key - '0');
            break;
          case '#': // show ip address entry screen
            witConnectionState = CONNECTION_STATE_ENTRY_REQUIRED;
            buildWitEntry();
            enterWitServer();
            break;
          default:  // do nothing 
            break;
        }
        break;

      case KEYPAD_USE_SELECT_SSID:
        debug_print("doKeyPress(): key ssid... "); debug_println(key);
        switch (key){
          case '0': case '1': case '2': case '3': case '4': 
          case '5': case '6': case '7': case '8': case '9':
            wifiSsidManager.selectConfigured(key - '0');
            break;
          case '#': // show found SSIds
            ssidConnectionState = CONNECTION_STATE_DISCONNECTED;
            keypadUseType = KEYPAD_USE_SELECT_SSID_FROM_FOUND;
            ssidSelectionSource = SSID_CONNECTION_SOURCE_BROWSE;
            // browseSsids();
            break;
          default:  // do nothing 
            break;
        }
        break;

      case KEYPAD_USE_SELECT_SSID_FROM_FOUND:
        debug_print("doKeyPress(): key ssid from found... "); debug_println(key);
        switch (key){
          case '0': case '1': case '2': case '3': case '4': 
            wifiSsidManager.selectFound(key - '0'+(page*5));
            break;
          case '#': // next page
            if (foundSsidsCount>5) {
              if ( (page+1)*5 < foundSsidsCount ) {
                page++;
              } else {
                page = 0;
              }
              oledRenderer.renderFoundSsids(""); 
            }
            break;
          case '9': // rescan for SSIDs
            page = 0; // reset to first page
            wifiSsidManager.browseSsids();
            break;
          default:  // do nothing 
            break;
        }
        break;

      case KEYPAD_USE_SELECT_ROSTER:
        debug_print("doKeyPress(): key Roster... "); debug_println(key);
        switch (key){
          case '0': case '1': case '2': case '3': case '4': 
          case '5': case '6': case '7': case '8': case '9':
            selectRoster((key - '0')+(page*5));
            break;
          case '#':  // next page
            if ( rosterSize > 5 ) {
              if ( (page+1)*5 < rosterSize ) {
                page++;
              } else {
                page = 0;
              }
              oledRenderer.renderRoster(""); 
            }
            break;
          case '*':  // cancel
            resetMenu();
            oledRenderer.renderSpeed();
            break;
          default:  // do nothing 
            break;
        }
        break;

      case KEYPAD_USE_SELECT_TURNOUTS_THROW:
      case KEYPAD_USE_SELECT_TURNOUTS_CLOSE:
        debug_print("doKeyPress(): key turnouts... "); debug_println(key);
        switch (key){
          case '0': case '1': case '2': case '3': case '4': 
          case '5': case '6': case '7': case '8': case '9':
            selectTurnoutList((key - '0')+(page*10), (keypadUseType == KEYPAD_USE_SELECT_TURNOUTS_THROW) ? TurnoutThrow : TurnoutClose);
            break;
          case '#':  // next page
            if ( turnoutListSize > 10 ) {
              if ( (page+1)*10 < turnoutListSize ) {
                page++;
              } else {
                page = 0;
              }
              oledRenderer.renderTurnoutList("", (keypadUseType == KEYPAD_USE_SELECT_TURNOUTS_THROW) ? TurnoutThrow : TurnoutClose); 
            }
            break;
          case '*':  // cancel
            resetMenu();
            oledRenderer.renderSpeed();
            break;
          default:  // do nothing 
            break;
        }
        break;

      case KEYPAD_USE_SELECT_ROUTES:
        debug_print("doKeyPress(): key routes... "); debug_println(key);
        switch (key){
          case '0': case '1': case '2': case '3': case '4': 
          case '5': case '6': case '7': case '8': case '9':
            selectRouteList((key - '0')+(page*10));
            break;
          case '#':  // next page
            if ( routeListSize > 10 ) {
              if ( (page+1)*10 < routeListSize ) {
                page++;
              } else {
                page = 0;
              }
              oledRenderer.renderRouteList(""); 
            }
            break;
          case '*':  // cancel
            resetMenu();
            oledRenderer.renderSpeed();
            break;
          default:  // do nothing 
            break;
        }
        break;

      case KEYPAD_USE_SELECT_FUNCTION:
        debug_print("doKeyPress(): key function... "); debug_println(key);
        switch (key){
          case '0': case '1': case '2': case '3': case '4': 
          case '5': case '6': case '7': case '8': case '9':
            selectFunctionList((key - '0')+(functionPage*10));
            break;
          case '#':  // next page
            if ( (functionPage+1)*10 < MAX_FUNCTIONS ) {
              functionPage++;
              oledRenderer.renderFunctionList(""); 
            } else {
              functionPage = 0;
              keypadUseType = KEYPAD_USE_OPERATION;
              oledRenderer.renderDirectCommands();
            }
            break;
          case '*':  // cancel
            resetMenu();
            oledRenderer.renderSpeed();
            break;
          default:  // do nothing 
            break;
        }
        break;

      case KEYPAD_USE_EDIT_CONSIST:
        debug_print("key Edit Consist... "); debug_println(key);
        switch (key){
          case '0': case '1': case '2': case '3': case '4': 
          case '5': case '6': case '7': case '8': case '9':
            if ( (key-'0') <= wiThrottleProtocol.getNumberOfLocomotives(throttleManager.getCurrentThrottleChar())) {
              selectEditConsistList(key - '0');
            }
            break;
          case '*':  // cancel
            resetMenu();
            oledRenderer.renderSpeed();
            break;
          default:  // do nothing 
            break;
        }
        break;

      default:  // do nothing 
        break;
    }

  } else {  // released
    debug_println("doKeyPress(): released"); 
    switch (keypadUseType) {
      case KEYPAD_USE_OPERATION:
        if ( (!menuCommandStarted) && (key>='0') && (key<='D')) { // only process releases for the numeric keys + A,B,C,D and only if a menu command has not be started
          debug_println("doKeyPress(): Operation - Process key release KEYPAD_USE_OPERATION");
          doDirectCommand(key, false);
        } else {
          debug_println("doKeyPress(): Non-Operation - Don't process key release");
        }
        break;
    
      case KEYPAD_USE_SELECT_FUNCTION:
        if (functionHasBeenSelected) {
          debug_println("doKeyPress(): Operation - Process key release KEYPAD_USE_SELECT_FUNCTION");
          doFunction(throttleManager.getCurrentThrottleIndex(), (key - '0')+(functionPage*10), false);
          keypadUseType = KEYPAD_USE_OPERATION;
          functionHasBeenSelected = false;
        }
        break;
    }
  }

  debug_println("doKeyPress(): end"); 
}

void doDirectCommand(char key, bool pressed) {
  debug_print("doDirectCommand(): key: "); debug_println(key);
  int buttonAction = 0 ;
  if (key<=57) {
    buttonAction = buttonActions[(key - '0')];
  } else {
    buttonAction = buttonActions[(key - 55)]; // A, B, C, D
  }
  debug_print("doDirectCommand(): Action: "); debug_println(buttonAction);
  if (buttonAction!=FUNCTION_NULL) {
    if ( (buttonAction>=FUNCTION_0) && (buttonAction<=FUNCTION_31) ) {
  doDirectFunction(throttleManager.getCurrentThrottleIndex(), buttonAction, pressed);
  } else {
      if (pressed) { // only process these on key press
        InputEvent ev; ev.timestamp = millis(); ev.type = InputEventType::Action; ev.ivalue = buttonAction; ev.cvalue = 0;
        inputManager.dispatch(ev);
      }
    }
  }
  // debug_println("doDirectCommand(): end"); 
}

// Legacy doDirectAdditionalButtonCommand removed: state machine now emits canonical AdditionalButton events


void doDirectAction(int buttonAction) {
  // Unified path: dispatch as InputEvent Action so mode handlers / fallback handle it.
  InputEvent ev; ev.timestamp = millis(); ev.type = InputEventType::Action; ev.ivalue = buttonAction; ev.cvalue = 0;
  inputManager.dispatch(ev);
}

void doMenu() {
  String loco = "";
  String function = "";
  bool menuCommandStartedTemp = false;
  debug_print("doMenu(): "); debug_println(menuCommand);
  
  switch (menuCommand[0]) {
    case MENU_ITEM_EXTRAS: { // Extra menu 
        char subCommand = menuCommand.charAt(1);
        if (menuCommand.length() > 1) { // must be a submenu command
          // debug_println("doMenu(): length>1");
          doMenuCommand(subCommand+65-48); // convert '0'-'9' to 'A' - 'J'
          menuCommandStartedTemp = true;
        } else {
          // debug_println("doMenu(): else");
          oledRenderer.renderSpeed();
        }
        break;
      }
      default: {
        // debug_println("doMenu(): default");
        doMenuCommand(menuCommand[0]);
        // menuCommandStartedTemp = false;
      }
  }
  menuCommandStarted = menuCommandStartedTemp; 
  debug_println("doMenu(): end");
}

void doMenuCommand(char menuItem) {
  debug_print("doMenuCommand(): "); debug_print(menuCommand); debug_print(" : "); debug_println(menuItem);
  String loco = "";
  String function = "";
  int startAt = 1;               // in main menu
  if (menuItem>'9') startAt = 2; // in the submenu

    switch (menuItem) {
    case MENU_ITEM_ADD_LOCO: { // select loco
        if (menuCommand.length()>startAt) {
          if ( (dropBeforeAcquire) && (wiThrottleProtocol.getNumberOfLocomotives(throttleManager.getCurrentThrottleChar())>0) ) {
            wiThrottleProtocol.releaseLocomotive(throttleManager.getCurrentThrottleChar(), "*");
          }
          loco = menuCommand.substring(startAt, menuCommand.length());
          loco = getLocoWithLength(loco);
          debug_print("add Loco: "); debug_println(loco);
          wiThrottleProtocol.addLocomotive(throttleManager.getCurrentThrottleChar(), loco);
          wiThrottleProtocol.getDirection(throttleManager.getCurrentThrottleChar(), loco);
          wiThrottleProtocol.getSpeed(throttleManager.getCurrentThrottleChar());
          resetFunctionStates(throttleManager.getCurrentThrottleIndex());
          oledRenderer.renderSpeed();
        } else {
          page = 0;
          oledRenderer.renderRoster("");
        }
        break;
      }
    case MENU_ITEM_DROP_LOCO: { // de-select loco
        loco = menuCommand.substring(startAt, menuCommand.length());
        if (loco!="") { // a loco is specified
          if (!CONSIST_RELEASE_BY_INDEX) {
            loco = getLocoWithLength(loco);
            releaseOneLoco(throttleManager.getCurrentThrottleIndex(), loco);
          } else {
            releaseOneLocoByIndex(throttleManager.getCurrentThrottleIndex(), loco.toInt());
          }
        } else { //not loco specified so release all
          releaseAllLocos(throttleManager.getCurrentThrottleIndex());
        }
  oledRenderer.renderSpeed();
        break;
      }
  case MENU_ITEM_TOGGLE_DIRECTION: { // change direction
    toggleDirection(throttleManager.getCurrentThrottleIndex());
        break;
      }
     case MENU_ITEM_SPEED_STEP_MULTIPLIER: { // toggle speed step additional Multiplier
  cycleSpeedStep();
        break;
      }
   case MENU_ITEM_THROW_POINT: {  // throw point
        if (menuCommand.length()>startAt) {
          String turnout = turnoutPrefix + menuCommand.substring(startAt, menuCommand.length());
          // if (!turnout.equals("")) { // a turnout is specified
            debug_print("throw point: "); debug_println(turnout);
            wiThrottleProtocol.setTurnout(turnout, TurnoutThrow);
          // }
          oledRenderer.renderSpeed();
        } else {
          page = 0;
          oledRenderer.renderTurnoutList("", TurnoutThrow);
        }
        break;
      }
    case MENU_ITEM_CLOSE_POINT: {  // close point
        if (menuCommand.length()>startAt) {
          String turnout = turnoutPrefix + menuCommand.substring(startAt, menuCommand.length());
          // if (!turnout.equals("")) { // a turnout is specified
            debug_print("close point: "); debug_println(turnout);
            wiThrottleProtocol.setTurnout(turnout, TurnoutClose);
          // }
          oledRenderer.renderSpeed();
        } else {
          page = 0;
          oledRenderer.renderTurnoutList("",TurnoutClose);
        }
        break;
      }
    case MENU_ITEM_ROUTE: {  // route
        if (menuCommand.length()>startAt) {
          String route = routePrefix + menuCommand.substring(startAt, menuCommand.length());
          // if (!route.equals("")) { // a loco is specified
            debug_print("route: "); debug_println(route);
            wiThrottleProtocol.setRoute(route);
          // }
          oledRenderer.renderSpeed();
        } else {
          page = 0;
          oledRenderer.renderRouteList("");
        }
        break;
      }
    case MENU_ITEM_TRACK_POWER: {
        powerToggle();
        break;
      }
    case MENU_ITEM_FUNCTION_KEY_TOGGLE: { // toggle showing Def Keys vs Function labels
        hashShowsFunctionsInsteadOfKeyDefs = !hashShowsFunctionsInsteadOfKeyDefs;
  oledRenderer.renderSpeed();
        break;
      } 
    case MENU_ITEM_EDIT_CONSIST: { // edit consist - loco facings
        // writeOledEditConsist();
        // break;
        char key = menuCommand.charAt(startAt);
        if (menuCommand.length()>startAt) {
          if ( ((key-'0') > 0) // can't change lead
          && ((key-'0') <= wiThrottleProtocol.getNumberOfLocomotives(throttleManager.getCurrentThrottleChar())) ) {
            selectEditConsistList(key - '0');
          }
          oledRenderer.renderSpeed();
        } else {
          writeOledEditConsist();
        }
        break;
      } 
    case MENU_ITEM_HEARTBEAT_TOGGLE: { // disable/enable the heartbeat Check
  heartbeatMonitor.toggleEnabled(); wiThrottleProtocol.requireHeartbeat(heartbeatMonitor.enabled()); writeHeartbeatCheck();
  oledRenderer.renderSpeed();
        break;
      }
    case MENU_ITEM_DROP_BEFORE_ACQUIRE_TOGGLE: { // disable/enable drop before Acquire
        toggleDropBeforeAquire();
  oledRenderer.renderSpeed();
        break;
      }
    case MENU_ITEM_SAVE_CURRENT_LOCOS: {
        writePreferences();
  oledRenderer.renderSpeed();
        break;
      }
    case MENU_ITEM_INCREASE_MAX_THROTTLES: { //increase number of Throttles
        changeNumberOfThrottles(true);
        break;
      }
    case MENU_ITEM_DECREASE_MAX_THROTTLES: { // decrease numbe rof throttles
        changeNumberOfThrottles(false);
        break;
      }
    case MENU_ITEM_DISCONNECT: { // disconnect   
        if (witConnectionState == CONNECTION_STATE_CONNECTED) {
          witConnectionState = CONNECTION_STATE_DISCONNECTED;
          // clearPreferences();
          // writePreferences();
          preferencesRead = false;
          disconnectWitServer();
        } else {
          connectWitServer();
        }
        break;
      }
    case MENU_ITEM_OFF_SLEEP: { // sleep/off
        // clearPreferences();
        // writePreferences();
        deepSleepStart();
        break;
      }
    case MENU_ITEM_FUNCTION: { // function button
        if (menuCommand.length()>startAt) {
          function = menuCommand.substring(startAt, menuCommand.length());
          int functionNumber = function.toInt();
          if (function != "") { // a function is specified
            doFunction(throttleManager.getCurrentThrottleIndex(), functionNumber, true, true);  // always act like latching i.e. pressed
          }
          oledRenderer.renderSpeed();
        } else {
          functionPage = 0;
          oledRenderer.renderFunctionList("");
        }
        break;
      }
  }
}

static void onPasswordCommit() {
  // Accept the entered password and return to normal mode
  selectedSsidPassword = ssidPasswordEntered;
  ssidConnectionState = CONNECTION_STATE_SELECTED;
  keypadUseType = KEYPAD_USE_OPERATION;
  encoderUseType = ENCODER_USE_OPERATION;
  inputManager.setMode(InputMode::Operation);
  oledRenderer.renderSpeed();
}

// *********************************************************************************
//  Actions
// *********************************************************************************

// Preferences read flag (referenced during disconnect/reconnect)
// (definition moved earlier near top of file)

// Legacy password entry function referenced by WifiSsidManager (UI now handled via PasswordEntryModeHandler)
void enterSsidPassword() {
  // When WifiSsidManager sets state to PASSWORD_ENTRY, InputManager already drives password UI.
  // This stub avoids unresolved symbol errors during phased refactor.
}

void resetMenu() {
  debug_println("resetMenu()");
  page = 0;
  menuCommand = "";
  menuCommandStarted = false;
  if ( (keypadUseType != KEYPAD_USE_SELECT_SSID) 
    && (keypadUseType != KEYPAD_USE_SELECT_WITHROTTLE_SERVER) ) {
    keypadUseType = KEYPAD_USE_OPERATION; 
  }
}

void resetFunctionStates(int multiThrottleIndex) {
  debug_println("resetFunctionStates()");
  for (int i=0; i<MAX_FUNCTIONS; i++) {
    functionStates[multiThrottleIndex][i] = false;
  }
}

void resetFunctionLabels(int multiThrottleIndex) {
  debug_print("resetFunctionLabels(): "); debug_println(multiThrottleIndex);
  for (int i=0; i<MAX_FUNCTIONS; i++) {
    functionLabels[multiThrottleIndex][i] = "";
  }
  functionPage = 0;
}

void resetAllFunctionLabels() {
  for (int i=0; i<throttleManager.getMaxThrottles(); i++) {
    resetFunctionLabels(i);
  }
}

void resetAllFunctionFollow() {
  for (int i=0; i<6; i++) {
    functionFollow[i][0] = CONSIST_FUNCTION_FOLLOW_F0;
    functionFollow[i][1] = CONSIST_FUNCTION_FOLLOW_F1;
    functionFollow[i][2] = CONSIST_FUNCTION_FOLLOW_F2;
    functionFollow[i][3] = CONSIST_FUNCTION_FOLLOW_F3;
    functionFollow[i][4] = CONSIST_FUNCTION_FOLLOW_F4;
    functionFollow[i][5] = CONSIST_FUNCTION_FOLLOW_F5;
    functionFollow[i][6] = CONSIST_FUNCTION_FOLLOW_F6;
    functionFollow[i][7] = CONSIST_FUNCTION_FOLLOW_F7;
    functionFollow[i][8] = CONSIST_FUNCTION_FOLLOW_F8;
    functionFollow[i][9] = CONSIST_FUNCTION_FOLLOW_F9;
    functionFollow[i][10] = CONSIST_FUNCTION_FOLLOW_F10;
    functionFollow[i][11] = CONSIST_FUNCTION_FOLLOW_F11;
    functionFollow[i][12] = CONSIST_FUNCTION_FOLLOW_F12;
    functionFollow[i][13] = CONSIST_FUNCTION_FOLLOW_F13;
    functionFollow[i][14] = CONSIST_FUNCTION_FOLLOW_F14;
    functionFollow[i][15] = CONSIST_FUNCTION_FOLLOW_F15;
    functionFollow[i][16] = CONSIST_FUNCTION_FOLLOW_F16;
    functionFollow[i][17] = CONSIST_FUNCTION_FOLLOW_F17;
    functionFollow[i][18] = CONSIST_FUNCTION_FOLLOW_F18;
    functionFollow[i][19] = CONSIST_FUNCTION_FOLLOW_F19;
    functionFollow[i][20] = CONSIST_FUNCTION_FOLLOW_F20;
    functionFollow[i][21] = CONSIST_FUNCTION_FOLLOW_F21;
    functionFollow[i][22] = CONSIST_FUNCTION_FOLLOW_F22;
    functionFollow[i][23] = CONSIST_FUNCTION_FOLLOW_F23;
    functionFollow[i][24] = CONSIST_FUNCTION_FOLLOW_F24;
    functionFollow[i][25] = CONSIST_FUNCTION_FOLLOW_F25;
    functionFollow[i][26] = CONSIST_FUNCTION_FOLLOW_F26;
    functionFollow[i][27] = CONSIST_FUNCTION_FOLLOW_F27;
    functionFollow[i][28] = CONSIST_FUNCTION_FOLLOW_F28;
    functionFollow[i][29] = CONSIST_FUNCTION_FOLLOW_F29;
    functionFollow[i][30] = CONSIST_FUNCTION_FOLLOW_F30;
    functionFollow[i][31] = CONSIST_FUNCTION_FOLLOW_F31;
  }
}

String getLocoWithLength(String loco) {
  int locoNo = loco.toInt();
  String locoWithLength = "";
  if ( (locoNo > SHORT_DCC_ADDRESS_LIMIT) 
  || ( (locoNo <= SHORT_DCC_ADDRESS_LIMIT) && (loco.charAt(0)=='0') && (!serverType.equals("DCC-EX" ) ) ) 
  ) {
    locoWithLength = "L" + loco;
  } else {
    locoWithLength = "S" + loco;
  }
  return locoWithLength;
}

void speedEstop() {
  throttleManager.speedEstopAll();
}

void speedEstopCurrentLoco() {
  throttleManager.speedEstopCurrent();
}

void speedDown(int multiThrottleIndex, int amt) {
  throttleManager.speedDown(multiThrottleIndex, amt);
}

void speedUp(int multiThrottleIndex, int amt) {
  throttleManager.speedUp(multiThrottleIndex, amt);
}

void speedSet(int multiThrottleIndex, int amt) {
  throttleManager.speedSet(multiThrottleIndex, amt);
}

int getDisplaySpeed(int multiThrottleIndex) {
  return throttleManager.getDisplaySpeed(multiThrottleIndex);
}

void stealLoco(int multiThrottleIndex, String loco) {
  wiThrottleProtocol.stealLocomotive(multiThrottleIndex, loco);  
}

// Overload added to support protocol delegate call using multiThrottle char
void stealLoco(char multiThrottle, String loco) {
  int idx = getMultiThrottleIndex(multiThrottle);
  stealLoco(idx, loco);
}

void toggleLocoFacing(int multiThrottleIndex, String loco) {
  debug_println("toggleLocoFacing()");
  char multiThrottleIndexChar = getMultiThrottleChar(multiThrottleIndex);
  debug_print("toggleLocoFacing(): "); debug_println(loco); 
  for(int i=0;i<wiThrottleProtocol.getNumberOfLocomotives(multiThrottleIndexChar);i++) {
    if (wiThrottleProtocol.getLocomotiveAtPosition(multiThrottleIndexChar, i).equals(loco)) {
      Direction currentDirection = wiThrottleProtocol.getDirection(multiThrottleIndexChar, loco);
      debug_print("toggleLocoFacing(): loco: ");  debug_print(loco);  debug_print(" current direction: "); debug_println(currentDirection);
      if (wiThrottleProtocol.getDirection(multiThrottleIndexChar, loco) == Forward) {
        wiThrottleProtocol.setDirection(multiThrottleIndexChar, loco, Reverse, true);
      } else {
        wiThrottleProtocol.setDirection(multiThrottleIndexChar, loco, Forward, true);
      }
      break;
    }
  } 
}

int getLocoFacing(int multiThrottleIndex, String loco) {
  char multiThrottleIndexChar = getMultiThrottleChar(multiThrottleIndex);
  int result = Forward;
  for(int i=0;i<wiThrottleProtocol.getNumberOfLocomotives(multiThrottleIndexChar);i++) {
    if (wiThrottleProtocol.getLocomotiveAtPosition(multiThrottleIndexChar, i).equals(loco)) {
      result = wiThrottleProtocol.getDirection(multiThrottleIndexChar, loco);
      break;
    }
  }
  return result;
}

String getDisplayLocoString(int multiThrottleIndex, int index) {
  char multiThrottleIndexChar = getMultiThrottleChar(multiThrottleIndex);
  String loco = wiThrottleProtocol.getLocomotiveAtPosition(multiThrottleIndexChar, index);
  String locoNumber = loco.substring(1);
  
  #ifdef DISPLAY_LOCO_NAME
    for (int i = 0; i < maxRoster; i++) {
      if (String(rosterAddress[i]) == locoNumber) {
        if (rosterName[i] != "") {
          locoNumber = rosterName[i];
        }
        break;
      }
    }
  #endif
  
  if (!wiThrottleProtocol.getLocomotiveAtPosition(multiThrottleIndexChar, 0).equals(loco)) { // not the lead loco
    Direction leadLocoDirection 
        = wiThrottleProtocol.getDirection(multiThrottleIndexChar, 
                                          wiThrottleProtocol.getLocomotiveAtPosition(multiThrottleIndexChar, 0));
    // Direction locoDirection = leadLocoDirection;

    for(int i=0;i<wiThrottleProtocol.getNumberOfLocomotives(multiThrottleIndexChar);i++) {
      if (wiThrottleProtocol.getLocomotiveAtPosition(multiThrottleIndexChar, i).equals(loco)) {
        if (wiThrottleProtocol.getDirection(multiThrottleIndexChar, loco) != leadLocoDirection) {
          locoNumber = locoNumber + DIRECTION_REVERSE_INDICATOR;
        }
        break;
      }
    }
  }
  return locoNumber;
}

void releaseAllLocos(int multiThrottleIndex) {
  char multiThrottleIndexChar = getMultiThrottleChar(multiThrottleIndex);
  String loco;
  if (wiThrottleProtocol.getNumberOfLocomotives(multiThrottleIndexChar)>0) {
    for(int index=wiThrottleProtocol.getNumberOfLocomotives(multiThrottleIndexChar)-1;index>=0;index--) {
      loco = wiThrottleProtocol.getLocomotiveAtPosition(multiThrottleIndexChar, index);
      wiThrottleProtocol.releaseLocomotive(multiThrottleIndexChar, loco);
  oledRenderer.renderSpeed();  // note the released locos may not be visible
    } 
    resetFunctionLabels(multiThrottleIndex);
  }
}

void releaseOneLoco(int multiThrottleIndex, String loco) {
  debug_print("releaseOneLoco(): "); debug_print(multiThrottleIndex); debug_print(": "); debug_println(loco);
  char multiThrottleIndexChar = getMultiThrottleChar(multiThrottleIndex);
  wiThrottleProtocol.releaseLocomotive(multiThrottleIndexChar, loco);
  resetFunctionLabels(multiThrottleIndex);
  debug_println("releaseOneLoco(): end"); 
}

void releaseOneLocoByIndex(int multiThrottleIndex, int index) {
  debug_print("releaseOneLocoByIndex(): "); debug_print(multiThrottleIndex); debug_print(": "); debug_println(index);
  char multiThrottleIndexChar = getMultiThrottleChar(multiThrottleIndex);
  if (index <= wiThrottleProtocol.getNumberOfLocomotives(multiThrottleIndexChar)) {
    String loco = wiThrottleProtocol.getLocomotiveAtPosition(multiThrottleIndexChar, index);
    wiThrottleProtocol.releaseLocomotive(multiThrottleIndexChar, loco);
    resetFunctionLabels(multiThrottleIndex);
  }
  debug_println("releaseOneLocoByIndex(): end");
}

// Speed step cycling now handled directly by ThrottleManager
void cycleSpeedStep() { throttleManager.cycleSpeedStep(); }

void toggleHeartbeatCheck() {
  heartbeatMonitor.toggleEnabled();
  debug_print("Heartbeat Check: "); debug_println(heartbeatMonitor.enabled() ? "Enabled" : "Disabled");
  wiThrottleProtocol.requireHeartbeat(heartbeatMonitor.enabled());
  writeHeartbeatCheck();
}

void toggleDropBeforeAquire() {
  dropBeforeAcquire = !dropBeforeAcquire;
  debug_print("Drop Before Acquire: "); 
  debug_println(dropBeforeAcquire ? "Enabled" : "Disabled"); 
}

void toggleDirection(int multiThrottleIndex) {
  throttleManager.toggleDirection(multiThrottleIndex);
}

void changeDirection(int multiThrottleIndex, Direction direction) {
  throttleManager.changeDirection(multiThrottleIndex, direction);
}

void doDirectFunction(int multiThrottleIndex, int functionNumber, bool pressed) {
  doDirectFunction(multiThrottleIndex, functionNumber, pressed, false);
}
void doDirectFunction(int multiThrottleIndex, int functionNumber, bool pressed, bool force) {
  char multiThrottleIndexChar = getMultiThrottleChar(multiThrottleIndex);
  debug_println("doDirectFunction(): "); 
  if (wiThrottleProtocol.getNumberOfLocomotives(multiThrottleIndexChar) > 0) {
    debug_print("direct fn: "); debug_print(functionNumber); debug_println( pressed ? " Pressed" : " Released");
    doFunctionWhichLocosInConsist(multiThrottleIndex, functionNumber, pressed, force);
  oledRenderer.renderSpeed(); 
  }
  // debug_print("doDirectFunction(): end"); 
}

void doFunction(int multiThrottleIndex, int functionNumber, bool pressed) {
  doFunction(multiThrottleIndex, functionNumber, pressed, false);
}
void doFunction(int multiThrottleIndex, int functionNumber, bool pressed, bool force) {
  char multiThrottleIndexChar = getMultiThrottleChar(multiThrottleIndex);
  debug_print("doFunction(): multiThrottleIndex "); debug_println(multiThrottleIndex);
  if (wiThrottleProtocol.getNumberOfLocomotives(multiThrottleIndexChar)>0) {
    if (force) {
      doFunctionWhichLocosInConsist(multiThrottleIndex, functionNumber, true, force);
      if (!functionStates[multiThrottleIndex][functionNumber]) {
        debug_print("fn: "); debug_print(functionNumber); debug_println(" Pressed FORCED");
        // functionStates[functionNumber] = true;
      } else {
        doFunctionWhichLocosInConsist(multiThrottleIndex, functionNumber, false, force);
        debug_print("fn: "); debug_print(functionNumber); debug_println(" Released FORCED");
        // functionStates[functionNumber] = false;
      }
    } else {
      doFunctionWhichLocosInConsist(multiThrottleIndex, functionNumber, pressed, false);
      debug_print("fn: "); debug_print(functionNumber); debug_println(" NOT FORCED");
    }
  oledRenderer.renderSpeed(); 
  }
  // debug_println("doFunction(): ");
}

// Work out which locos in a consist should get the function
//
void doFunctionWhichLocosInConsist(int multiThrottleIndex, int functionNumber, bool pressed) {
  doFunctionWhichLocosInConsist(multiThrottleIndex, functionNumber, pressed, false);
}
void doFunctionWhichLocosInConsist(int multiThrottleIndex, int functionNumber, bool pressed, bool force) {
  char multiThrottleIndexChar = getMultiThrottleChar(multiThrottleIndex);
  if (functionFollow[multiThrottleIndex][functionNumber]==CONSIST_LEAD_LOCO) {
    wiThrottleProtocol.setFunction(multiThrottleIndexChar, "", functionNumber, pressed, force);
  } else {  // at the momemnt the only other option in CONSIST_ALL_LOCOS
    wiThrottleProtocol.setFunction(multiThrottleIndexChar, "*", functionNumber, pressed, force);
  }
  debug_print("doFunctionWhichLocosInConsist(): fn: "); debug_print(functionNumber); debug_println(" Released");
}

void powerOnOff(TrackPower powerState) {
  debug_println("powerOnOff()");
  wiThrottleProtocol.setTrackPower(powerState);
  trackPower = powerState;
  oledRenderer.renderSpeed();
}

void powerToggle() {
  debug_println("PowerToggle()");
  if (trackPower==PowerOn) {
    powerOnOff(PowerOff);
  } else {
    powerOnOff(PowerOn);
  }
}

void nextThrottle() {
  throttleManager.nextThrottle();
}

void throttle(int throttleIndex) {
  throttleManager.selectThrottle(throttleIndex);
}

void changeNumberOfThrottles(bool increase) {
  throttleManager.changeNumberOfThrottles(increase);
}

void batteryShowToggle() {
  debug_println("batteryShowToggle()");
  batteryMonitor.toggleDisplayMode();
  oledRenderer.renderSpeed();
}

void stopThenToggleDirection() {
  if (wiThrottleProtocol.getNumberOfLocomotives(throttleManager.getCurrentThrottleChar())>0) {
    if (throttleManager.getCurrentSpeed(throttleManager.getCurrentThrottleIndex()) != 0) {
      // wiThrottleProtocol.setSpeed(currentThrottleIndexChar, 0);
  speedSet(throttleManager.getCurrentThrottleIndex(),0);
    } else {
  if (toggleDirectionOnEncoderButtonPressWhenStationary) toggleDirection(throttleManager.getCurrentThrottleIndex());
    }
  throttleManager.speeds()[throttleManager.getCurrentThrottleIndex()] = 0;
  }
}

void reconnect() {
  clearOledArray(); 
  oledText[0] = appName; oledText[6] = appVersion; 
  oledText[2] = MSG_DISCONNECTED;
  oledRenderer.renderArray(false, false);
  delay(5000);
  disconnectWitServer();
}


void checkForShutdownOnNoResponse() {
  if (millis()-startWaitForSelection > MAX_HEARTBEAT_PERIOD) {  // default is 4 minutes
      debug_println("Waited too long for a selection. Shutting down");
      deepSleepStart();
  }
}

String getDots(int howMany) {
  //             123456789_123456789_123456789_123456789_123456789_123456789_
  String dots = "............................................................";
  if (howMany>dots.length()) howMany = dots.length();
  return dots.substring(0,howMany);
}

int compareStrings( const void *str1, const void *str2 ) {
    const char *rec1 = *(char**)str1;
    const char *rec2 = *(char**)str2;
    int val = strcmp(rec1, rec2);

    return val;
}

void doStartupCommands() {
      debug_println("doStartupCommands()");
  for(int i=0; i<4; i++) {
    doOneStartupCommand(startupCommands[i]);
  }
}

void doOneStartupCommand(String cmd) {
  if (cmd.length()>0) {
    menuCommandStarted = false;
    menuCommand = "";
    char firstKey = cmd.charAt(0);
    for(int j=0; j<cmd.length(); j++) {
      char jKey = cmd.charAt(j);
      doKeyPress(jKey, true);
      if (firstKey != '*') { 
        doKeyPress(jKey, false);
      }
    }
  }
}

// *********************************************************************************
//  Select functions
// *********************************************************************************

void selectRoster(int selection) {
  debug_print("selectRoster() "); debug_println(selection);

  if ((selection>=0) && (selection < rosterSize)) {
    if ( (dropBeforeAcquire) && (wiThrottleProtocol.getNumberOfLocomotives(throttleManager.getCurrentThrottleChar())>0) ) {
      wiThrottleProtocol.releaseLocomotive(throttleManager.getCurrentThrottleChar(), "*");
    }
    int index = rosterSortedIndex[selection];
    String loco = String(rosterLength[index]) + rosterAddress[index];
    
    // String loco = String(rosterLength[selection]) + rosterAddress[selection];
    debug_print("add Loco: "); debug_println(loco);
  wiThrottleProtocol.addLocomotive(throttleManager.getCurrentThrottleChar(), loco);
  wiThrottleProtocol.getDirection(throttleManager.getCurrentThrottleChar(), loco);
  wiThrottleProtocol.getSpeed(throttleManager.getCurrentThrottleChar());
  resetFunctionStates(throttleManager.getCurrentThrottleIndex());
  oledRenderer.renderSpeed();
    keypadUseType = KEYPAD_USE_OPERATION;
  }
}

void selectTurnoutList(int selection, TurnoutAction action) {
  debug_print("selectTurnoutList() "); debug_println(selection);

  if ((selection>=0) && (selection < turnoutListSize)) {
    String turnout = turnoutListSysName[selection];
    debug_print("Turnout Selected: "); debug_println(turnout);
    wiThrottleProtocol.setTurnout(turnout,action);
  oledRenderer.renderSpeed();
    keypadUseType = KEYPAD_USE_OPERATION;
  }
}

void selectRouteList(int selection) {
  debug_print("selectRouteList() "); debug_println(selection);

  if ((selection>=0) && (selection < routeListSize)) {
    String route = routeListSysName[selection];
    debug_print("Route Selected: "); debug_println(route);
    wiThrottleProtocol.setRoute(route);
  oledRenderer.renderSpeed();
    keypadUseType = KEYPAD_USE_OPERATION;
  }
}

void selectFunctionList(int selection) {
  debug_print("selectFunctionList() "); debug_println(selection);

  if ((selection>=0) && (selection < MAX_FUNCTIONS)) {
  String function = functionLabels[throttleManager.getCurrentThrottleIndex()][selection];
    debug_print("Function Selected: "); debug_println(function);
  doFunction(throttleManager.getCurrentThrottleIndex(), selection, true,false);
    functionHasBeenSelected = true;    
  oledRenderer.renderSpeed();
    // keypadUseType = KEYPAD_USE_OPERATION;   // don't reset it now.  Do so on the release.
  }
}

void selectEditConsistList(int selection) {
  debug_print("selectEditConsistList() "); debug_println(selection);

  if (wiThrottleProtocol.getNumberOfLocomotives(throttleManager.getCurrentThrottleChar()) > 1 ) {
    String loco = wiThrottleProtocol.getLocomotiveAtPosition(throttleManager.getCurrentThrottleChar(), selection);
    toggleLocoFacing(throttleManager.getCurrentThrottleIndex(), loco);
  oledRenderer.renderSpeed();
    keypadUseType = KEYPAD_USE_OPERATION;
    menuCommandStarted = false;
  }
}

// *********************************************************************************
//  oLED functions
// *********************************************************************************

void setAppnameForOled() {
  oledText[0] = appName; oledText[6] = appVersion; 
}

void receivingServerInfoOled(int index, int maxExpected) {
  debug_print("receivingServerInfoOled(): LastSent: ");
  debug_println(lastReceivingServerDetailsTime);
  if (index < (maxExpected-1) ) {
    if (millis()-lastReceivingServerDetailsTime >= 2000) {  // refresh it every X seconds if needed
      if (broadcastMessageText == "") broadcastMessageText = MSG_RECEIVING_SERVER_DETAILS;
      lastReceivingServerDetailsTime = millis();
      broadcastMessageTime = millis();
      setMenuTextForOled(menu_menu);
      refreshOled();
    } // else do nothing
  } else {
    lastReceivingServerDetailsTime = 0;
    broadcastMessageTime = 0;
    broadcastMessageText = "";
    refreshOled();
  }
}

void setMenuTextForOled(int menuTextIndex) {
  debug_print("setMenuTextForOled(): ");
  debug_println(menuTextIndex);
  oledText[5] = menu_text[menuTextIndex];
  if (broadcastMessageText != "") {
    if (millis()-broadcastMessageTime < 10000) {
      oledText[5] = broadcastMessageText;
    } else {
      broadcastMessageText = "";
      broadcastMessageTime = 0;
    }
  }
}

void refreshOled() {
     debug_print("refreshOled(): ");
     debug_println(lastOledScreen);
  switch (lastOledScreen) {
    case last_oled_screen_speed:
  oledRenderer.renderSpeed();
      break;
    case last_oled_screen_turnout_list:
  oledRenderer.renderTurnoutList(lastOledStringParameter, lastOledTurnoutParameter);
      break;
    case last_oled_screen_route_list:
  oledRenderer.renderRouteList(lastOledStringParameter);
      break;
    case last_oled_screen_function_list:
  oledRenderer.renderFunctionList(lastOledStringParameter);
      break;
    case last_oled_screen_menu:
  oledRenderer.renderMenu(lastOledStringParameter, true);
      break;
    case last_oled_screen_extra_submenu:
  oledRenderer.renderMenu(lastOledStringParameter, false);
      break;
    case last_oled_screen_all_locos:
  oledRenderer.renderAllLocosScreen(lastOledBoolParameter);
      break;
    case last_oled_screen_edit_consist:
      writeOledEditConsist();
      break;
    case last_oled_screen_direct_commands:
  oledRenderer.renderDirectCommands();
      break;
  }
}


// Legacy free-function OLED wrappers fully removed; use oledRenderer methods directly.

// Thin wrappers retained temporarily for in-progress migration paths
inline void clearOledArray() { oledRenderer.clearArray(); }
// inline void writeOledBattery() { oledRenderer.renderBattery(); } // deprecated
// inline void writeOledDirectCommands() { oledRenderer.renderDirectCommands(); } // deprecated
inline void writeHeartbeatCheck() { oledRenderer.renderHeartbeatCheck(); }
inline void writeOledEditConsist() { oledRenderer.renderEditConsist(); }

// *********************************************************************************

void deepSleepStart() {
  deepSleepStart(SLEEP_REASON_COMMAND);
}

void deepSleepStart(int shutdownReason) {
  clearOledArray(); 
  setAppnameForOled();
  int delayPeriod = 2000;
  if (shutdownReason==SLEEP_REASON_INACTIVITY) {
    oledText[2] = MSG_AUTO_SLEEP;
    delayPeriod = 10000;
  } else if (shutdownReason==SLEEP_REASON_BATTERY) {
    oledText[2] = MSG_BATTERY_SLEEP;
    delayPeriod = 10000;
  }
  oledText[3] = MSG_START_SLEEP;
  oledRenderer.renderBattery();
  oledRenderer.renderArray(false, false, true, true);
  delay(delayPeriod);

  u8g2.setPowerSave(1);
  esp_deep_sleep_start();
}
