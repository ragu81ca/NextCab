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
#ifndef USE_TFT_ESPI
  #include <U8g2lib.h>            // https://github.com/olikraus/u8g2  (Just get "U8g2" via the Arduino IDE Library Manager)   new-bsd
#endif
#include <WiThrottleProtocol.h>   // https://github.com/flash62au/WiThrottleProtocol                           Creative Commons 4.0  Attribution-ShareAlike

// Pangodream_18650_CL.h now only needed inside BatteryMonitor implementation

#include "config_buttons.h"      // keypad buttons assignments
#include "WiTcontroller.h"       // legacy macros & extern mappings (must precede usage of max* constants)

// Battery monitoring — compile-time type alias selects concrete implementation
#include "src/core/BatteryMonitor.h"
BatteryMonitor batteryMonitor;
// Core managers & devices
#include "src/core/input/InputManager.h"
#include "src/core/SystemState.h"
#ifdef USE_QWIIC_KEYPAD
  #include "src/core/input/QwiicKeypadInput.h"
#elif defined(USE_KEYPAD_4X4)
  #include "src/core/input/MatrixKeypad4x4Input.h"
#else
  #include "src/core/input/MatrixKeypad3x4Input.h"
#endif
#include "src/core/input/RotaryEncoderInput.h"
#ifdef USE_MODULINO_KNOB
  #include "src/core/input/ModulinoKnobInput.h"
#endif
#include "src/core/input/PotThrottleInput.h"
#include "src/core/input/AdditionalButtonsInput.h"
#include "src/core/input/InputEvents.h"
#include "src/core/ThrottleManager.h"
#include "src/core/network/WifiSsidManager.h"
#include "src/core/Renderer.h"
#include "src/core/DisplayConfig.h"
#include "src/core/input/OperationModeHandler.h"
#include "src/core/input/PasswordEntryModeHandler.h"
#include "src/core/input/WifiSelectionHandler.h"
#include "src/core/input/WiThrottleServerSelectionHandler.h"
#include "src/core/input/RosterSelectionHandler.h"
#include "src/core/input/TurnoutSelectionHandler.h"
#include "src/core/input/RouteSelectionHandler.h"
#include "src/core/input/FunctionSelectionHandler.h"
#include "src/core/input/DropLocoSelectionHandler.h"
#include "src/core/input/EditConsistSelectionHandler.h"
#include "src/core/input/SystemActionHandler.h"
#include "src/core/protocol/WiThrottleDelegate.h" // ensure debug_print macros before first use
#include "src/core/network/WiThrottleConnectionManager.h"
#include "src/core/menu/MenuSystem.h"
#include "src/core/menu/MenuDefinitions.h"
#include "src/core/ui/TitleScreen.h"
#include "src/core/ui/ThrottleScreen.h"
#include "src/core/storage/ConfigStore.h"
#include "src/core/ServerDataStore.h"
#include "src/core/LocoManager.h"

ThrottleManager throttleManager; // speed/direction/throttle index
InputManager inputManager;       // generic input dispatcher
WifiSsidManager wifiSsidManager; // Wi-Fi SSID manager
ConfigStore configStore;         // LittleFS + JSON persistent storage
ServerDataStore serverDataStore; // roster, turnout, route data from server
LocoManager locoManager;         // loco acquisition, release, consist management
Renderer renderer(displayDriver, activeLayout, activeFonts); // display-agnostic renderer
MenuSystem menuSystem;           // New table-driven menu system
WiThrottleConnectionManager connectionManager; // WiThrottle server discovery/connection

// ----------------- Legacy global variables (bridging for refactor) -----------------
bool menuCommandStarted = false;
String menuCommand = "";
TrackPower trackPower = PowerOff; // initial power state
// turnoutPrefix / routePrefix now in serverDataStore

// debug_print / debug_println macros now supplied by WiThrottleDelegate.h

// Throttle pot globals required by PotThrottleInput.cpp (retain legacy semantics)
// TODO: Move these config values to a dedicated Config.h or pass via constructor
int throttlePotPin = 36; // default analog pin (can be overridden later)
bool throttlePotUseNotches = false;
int throttlePotNotchValues[8] = {0,200,400,600,800,1000,2000,4000};
int throttlePotNotchSpeeds[8] = {0,20,40,60,80,100,120,127};

// ----------------- Rotary / Pot selection flag -----------------
#ifndef USE_ROTARY_ENCODER_FOR_THROTTLE
  #define USE_ROTARY_ENCODER_FOR_THROTTLE true
#endif

// ----------------- Startup commands stub -----------------
String startupCommands[4] = {"","","",""};

// Additional buttons device (static storage to avoid heap fragmentation)
static AdditionalButtonsInput additionalButtonsInputInstance([](const InputEvent &ev){ inputManager.dispatch(ev); });
static bool additionalButtonsConfigured = false;

// Configuration mapping old arrays -> new definitions (minimal bridge). We reuse existing arrays if defined.
// Expect: additionalButtonPin[], additionalButtonType[], additionalButtonActions[], maxAdditionalButtons.
struct AdditionalButtonDef buttonDefsStatic[MAX_ADDITIONAL_BUTTONS];
// Instantiate mode handlers
OperationModeHandler operationModeHandler(throttleManager, menuSystem, inputManager, renderer);
static void onPasswordCommit();
static void onPasswordCancel();
PasswordEntryModeHandler passwordModeHandler(32, onPasswordCommit); // arbitrary max len
WifiSelectionHandler wifiSelectionHandler(renderer, wifiSsidManager);
WiThrottleServerSelectionHandler witServerSelectionHandler(renderer);
RosterSelectionHandler rosterSelectionHandler(renderer);
TurnoutSelectionHandler turnoutSelectionHandler(renderer);
RouteSelectionHandler routeSelectionHandler(renderer);
FunctionSelectionHandler functionSelectionHandler(renderer);
DropLocoSelectionHandler dropLocoSelectionHandler(renderer);
EditConsistSelectionHandler editConsistSelectionHandler(renderer);
SystemActionHandler systemActionHandler(throttleManager, renderer, batteryMonitor, wiThrottleProtocol);

// server variables
// bool ssidConnected = false;
String selectedSsid = "";

// Unified system state manager (replaces ssidConnectionState and witConnectionState)
SystemStateManager systemStateManager(inputManager);

// Connection-related state now lives in WiThrottleConnectionManager.
// serverType now in locoManager

// Protocol config (still needed by WifiSsidManager)
bool commandsNeedLeadingCrLf = SEND_LEADING_CR_LF_FOR_COMMANDS;

// WiFi selection source (remains global — written by WiThrottleConnectionManager on disconnect)
int ssidSelectionSource;
double startWaitForSelection;

// roster / turnout / route data now in serverDataStore

// migrated: page, functionPage, functionHasBeenSelected now in uiState

// Broadcast message state migrated to UIState (macros in WiTcontroller.h)

// remember oLED state
// migrated: lastOled* variables now in uiState
int lastOledIntParameter = 0; // TODO: consider moving this as presenters evolve

// turnout / route arrays removed — now in serverDataStore

// function states / labels / consist follow — migrated to ThrottleManager

// speedstep
// migrated to ThrottleManager: int currentSpeedStep[6];

// throttle
// (moved to ThrottleManager) currentThrottleIndex / currentThrottleIndexChar / maxThrottles

// Heartbeat monitoring now encapsulated
#include "src/core/heartbeat/HeartbeatMonitor.h"
HeartbeatMonitor heartbeatMonitor; // manages heartbeat period / last response / enabled flag (global for extern linkage)

// used to stop speed bounces
// (moved to ThrottleManager) lastSpeedSentTime / lastSpeedSent / lastSpeedThrottleIndex

// dropBeforeAcquire now in locoManager

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

// stealCurrentLoco removed — delegate now calls locoManager.stealLoco() directly

WiFiClient client;
WiThrottleProtocol wiThrottleProtocol;
// Identity now managed by connectionManager; bridge for WifiSsidManager's extern
// connectionManager.hostname() is only valid after computeIdentity(), which runs in setup().

// Loco persistence (save/restore) now lives in LocoManager.

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

// *********************************************************************************
//  Setup and Loop
// *********************************************************************************

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("[BOOT] Serial up");
#ifndef USE_TFT_ESPI
  // u8g2.setI2CAddress(0x3C * 2);
  // u8g2.setBusClock(100000);
#endif
  Serial.println("[BOOT] Calling displayDriver.begin()...");
  displayDriver.begin();
  Serial.println("[BOOT] displayDriver.begin() done");
  displayDriver.firstPage();

  delay(1000);
  debug_println("Start"); 
  debug_print("WiTcontroller - Version: "); debug_println(appVersion);

  batteryMonitor.begin();
  // Initial battery service
  batteryMonitor.loop();
  if (batteryMonitor.shouldSleepForLowBattery()) { deepSleepStart(SLEEP_REASON_BATTERY); }

  { TitleScreen ts;
    ts.setAppHeader(appName, appVersion);
    ts.addBody(MSG_START);
    renderer.renderTitle(ts);
    renderer.renderBattery(); }

  // Rotary initialization now handled by RotaryEncoderInput begin() if rotary selected.

#ifdef USE_TFT_ESPI
  // GPIO 13 is used for TFT_DC on ESP32-S3 TFT builds — skip ext0 wakeup
  // TODO: assign a different wakeup pin for ESP32-S3 builds if deep-sleep wake is needed
#else
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_13,0); //1 = High, 0 = Low
#endif

  ssidSelectionSource = SSID_CONNECTION_SOURCE_BROWSE; // Always scan for available networks first

  initialiseAdditionalButtons();

  // Function arrays + speed/direction initialized inside throttleManager.begin()
  throttleManager.begin(&wiThrottleProtocol);
  locoManager.begin(&wiThrottleProtocol, &throttleManager, &serverDataStore, &renderer, &uiState,
                    &configStore, &connectionManager);
  throttleManager.sound().begin(&throttleManager, &wiThrottleProtocol, &locoManager);
  locoManager.setDropBeforeAcquire(DROP_BEFORE_ACQUIRE);
  uiState.functionPage = 0;
  // Wire up generic input manager mode & action handlers
  inputManager.setOperationHandler(&operationModeHandler);
  inputManager.setPasswordHandler(&passwordModeHandler);
  passwordModeHandler.setCancelCallback(onPasswordCancel);
  inputManager.setWifiSelectionHandler(&wifiSelectionHandler);
  inputManager.setWiThrottleServerSelectionHandler(&witServerSelectionHandler);
  inputManager.setRosterSelectionHandler(&rosterSelectionHandler);
  inputManager.setTurnoutSelectionHandler(&turnoutSelectionHandler);
  inputManager.setRouteSelectionHandler(&routeSelectionHandler);
  inputManager.setFunctionSelectionHandler(&functionSelectionHandler);
  inputManager.setDropLocoSelectionHandler(&dropLocoSelectionHandler);
  inputManager.setEditConsistHandler(&editConsistSelectionHandler);
  inputManager.setActionFallbackHandler(&systemActionHandler);
  inputManager.forceMode(InputMode::WifiSelection); // Start with WiFi selection at boot
  
  // Initialize HeartbeatMonitor but don't start it yet (will start after WiThrottle connection)
  // Register heartbeat timeout handler
  heartbeatMonitor.setOnTimeout([](){
    debug_println("Heartbeat timeout - server not responding");
    
    // Check if TCP connection is actually dead vs just no heartbeat response
    if (!client.connected()) {
      debug_println("TCP connection lost - genuine disconnect");
    } else {
      debug_println("TCP still connected - possible server issue or network latency");
    }
    
    // Show disconnection message to user
    { TitleScreen ts;
      ts.title = appName;
      ts.addBody(MSG_DISCONNECTED);
      ts.addBody("Server timeout");
      renderer.renderTitle(ts); }
    
    // Clean disconnect - don't try to release locos as connection may be dead
    wiThrottleProtocol.disconnect();
    systemStateManager.setState(SystemState::WifiConnected);
    connectionManager.ipAndPortChanged() = true;
    
    debug_println("Returning to server selection");
  });
  
  // Register period change handler
  heartbeatMonitor.setOnPeriodChange([](unsigned long p){
    debug_printf("Heartbeat period updated by server: %lu seconds\n", p);
  });
  // ── Initialise I2C bus early, before any Qwiic/I2C device begin() calls ──
#if defined(USE_QWIIC_KEYPAD) || defined(USE_MODULINO_KNOB)
  #ifdef PIN_I2C_POWER
    pinMode(PIN_I2C_POWER, OUTPUT);
    digitalWrite(PIN_I2C_POWER, HIGH);
    delay(100); // let Qwiic bus power stabilise
    Serial.printf("[I2C] Power pin %d set HIGH\n", PIN_I2C_POWER);
  #endif
  Wire.begin();
  Wire.setClock(400000); // 400 kHz I2C speed for faster device response
  delay(50); // let bus settle
  Serial.println("[I2C] Bus initialised");
#endif

  // Register throttle input device — compile-time selection
#ifdef USE_MODULINO_KNOB
  static ModulinoKnobInput throttleDev([&](const InputEvent &ev){ inputManager.dispatch(ev); });
#elif USE_ROTARY_ENCODER_FOR_THROTTLE
  static RotaryEncoderInput throttleDev([&](const InputEvent &ev){ inputManager.dispatch(ev); });
#else
  static PotThrottleInput throttleDev([&](const InputEvent &ev){ inputManager.dispatch(ev); });
#endif
  inputManager.registerDevice(&throttleDev);
  throttleDev.begin();
  // Register keypad device — compile-time selection
#ifdef USE_QWIIC_KEYPAD
  static QwiicKeypadInput keypadDev([&](const InputEvent &ev){ inputManager.dispatch(ev); });
#elif defined(USE_KEYPAD_4X4)
  static MatrixKeypad4x4Input keypadDev([&](const InputEvent &ev){ inputManager.dispatch(ev); });
#else
  static MatrixKeypad3x4Input keypadDev([&](const InputEvent &ev){ inputManager.dispatch(ev); });
#endif
  inputManager.registerDevice(&keypadDev);
  keypadDev.begin();

  // Initialize persistent storage (LittleFS + JSON)
  configStore.begin();

  // Initialize WifiSsidManager — loads stored credentials from ConfigStore
  wifiSsidManager.begin(systemStateManager, renderer, configStore);
  
  // Initialize new menu system
  menuSystem.begin(MenuDefinitions::mainMenuItems, MenuDefinitions::mainMenuSize);
  debug_println("MenuSystem initialized");
  
  // Initialize WiThrottle Connection Manager
  connectionManager.begin(client, wiThrottleProtocol, myDelegate,
                          renderer, throttleManager, inputManager,
                          systemStateManager, heartbeatMonitor,
                          witServerSelectionHandler);
  connectionManager.setOnReleaseAllLocos([](int idx){ locoManager.releaseAllLocos(idx); });
  connectionManager.setOnStartupCommands([](){ doStartupCommands(); });

  // Ensure station mode so MAC retrieval succeeds before connect.
  WiFi.mode(WIFI_STA); // still set STA mode for consistency
  // Derive identity (MAC-based) and set WiFi hostname BEFORE WiFi connect attempts.
  connectionManager.computeIdentity();
  WiFi.setHostname(connectionManager.hostname().c_str());
  debug_print("Hostname set to: "); debug_println(connectionManager.hostname());
  // Country code feature removed with esp_wifi include elimination; reintroduce if regulatory needs arise.
}

void loop() {
  
  // State machine: execute appropriate logic based on current system state
  SystemState currentState = systemStateManager.getState();
  
  switch (currentState) {
    case SystemState::Boot:
    case SystemState::WifiScanning:
    case SystemState::WifiSelection:
    case SystemState::WifiConnecting:
      // WiFi connection sequence
      wifiSsidManager.loop();
      checkForShutdownOnNoResponse(); // Check for user inactivity
      break;

    case SystemState::WifiPasswordEntry: {
      // Password entry — drive caret blink animation
      static unsigned long lastPasswordTick = 0;
      unsigned long now = millis();
      if (now - lastPasswordTick >= 125) {
        lastPasswordTick = now;
        passwordModeHandler.tick();
      }
      checkForShutdownOnNoResponse();
      break;
    }
      
    case SystemState::WifiConnected:
    case SystemState::ServerScanning:
    case SystemState::ServerSelection:
    case SystemState::ServerManualEntry:
    case SystemState::ServerConnecting:
      // WiThrottle server connection sequence
      connectionManager.loop();
      checkForShutdownOnNoResponse(); // Check for user inactivity
      break;
      
    case SystemState::Operating:
      // Normal operation - controlling locos

      // Auto-dismiss the "Connected" info splash after ~3 s
      if (connectionManager.isShowingConnectedSplash()) {
        if (millis() >= connectionManager.connectedSplashEndTime()) {
          connectionManager.clearConnectedSplash();
          renderer.renderSpeed();
        }
        // While splash is visible, still process protocol traffic below
      }
      
      // Check TCP connection health - detect WiFi/connection drops early
      if (!client.connected()) {
        debug_println("TCP connection lost during operation");
        heartbeatMonitor.setEnabled(false); // Disable heartbeat immediately
        { TitleScreen ts;
          ts.title = appName;
          ts.addBody(MSG_DISCONNECTED);
          ts.addBody("Connection lost");
          renderer.renderTitle(ts); }
        wiThrottleProtocol.disconnect();
        systemStateManager.setState(SystemState::WifiConnected);
        connectionManager.ipAndPortChanged() = true;
        break;
      }
      
      wiThrottleProtocol.check();    // parse incoming messages, send keepalives
      
      // Note: DCC-EX doesn't send periodic responses during idle — the library
      // sends keepalives every ~5s to keep the SERVER's timeout from firing,
      // but the server doesn't echo anything back. So we note activity here
      // (TCP is verified alive above) to prevent false heartbeat timeouts.
      // Real disconnects are caught immediately by client.connected() above.
      heartbeatMonitor.noteActivity(millis() / 1000, false);
      
      heartbeatMonitor.loop();       // backstop for truly stuck states

      // Drive menu TEXT_INPUT caret blink when active
      if (menuSystem.isInInputMode()) {
        static unsigned long lastMenuInputTick = 0;
        unsigned long now = millis();
        if (now - lastMenuInputTick >= 125) {
          lastMenuInputTick = now;
          menuSystem.tickInput();
        }
      }
      break;
  }
  
  // Unified device polling (rotary/pot, keypad, additional buttons)
  inputManager.pollAllDevices();
  
  // Reset inactivity timer on any potential user input during pre-connection sequence
  if (systemStateManager.isInPreConnectionSequence()) {
    startWaitForSelection = millis();
  }
  
  // Update momentum (handles smooth speed ramping)
  throttleManager.updateMomentum();

  if (batteryMonitor.enabled()) {
    batteryMonitor.loop();
    if (batteryMonitor.shouldSleepForLowBattery()) { deepSleepStart(SLEEP_REASON_BATTERY); }
  }

	// debug_println("loop:" );
}

// *********************************************************************************

static void onPasswordCommit() {
  // Accept the entered password and connect
  wifiSsidManager.setPassword(passwordModeHandler.entered());
  systemStateManager.setState(SystemState::WifiConnecting);
}

static void onPasswordCancel() {
  // Go back to the WiFi selection list
  systemStateManager.forceState(SystemState::WifiSelection);
}

// *********************************************************************************
//  Actions
// *********************************************************************************

// Legacy free functions removed — callers now use managers directly:
// speedEstop/Current, speedDown/Up/Set, getDisplaySpeed → ThrottleManager
// cycleSpeedStep, toggleDirection, changeDirection, nextThrottle → ThrottleManager
// changeNumberOfThrottles → ThrottleManager
// powerOnOff, powerToggle, batteryShowToggle → SystemActionHandler (inlined)
// toggleDropBeforeAcquire → LocoManager
// toggleHeartbeatCheck → HeartbeatMonitor + MenuDefinitions
// stopThenToggleDirection, reconnect → dead code (unused)

// compareStrings() moved to ServerDataStore

void checkForShutdownOnNoResponse() {
  // Only check inactivity during pre-connection sequence
  // Once operating, this timeout doesn't apply
  if (systemStateManager.isOperating()) {
    return; // In operating mode - inactivity timeout doesn't apply
  }
  
  if (millis() - startWaitForSelection > PRE_WIFI_INACTIVITY_TIMEOUT) {
    debug_println("Pre-connection inactivity timeout. Going to sleep");
    deepSleepStart(SLEEP_REASON_INACTIVITY);
  }
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
      // Simulate keypad press
      InputEvent press;
      press.timestamp = millis();
      press.type = (jKey >= '0' && jKey <= '9') ? InputEventType::KeypadChar : InputEventType::KeypadSpecial;
      press.cvalue = jKey;
      press.ivalue = (jKey >= '0' && jKey <= '9') ? (jKey - '0') : 0;
      inputManager.dispatch(press);
      if (firstKey != '*') {
        // Simulate keypad release
        InputEvent release;
        release.timestamp = millis();
        release.type = (jKey >= '0' && jKey <= '9') ? InputEventType::KeypadCharRelease : InputEventType::KeypadSpecialRelease;
        release.cvalue = jKey;
        release.ivalue = press.ivalue;
        inputManager.dispatch(release);
      }
    }
  }
}

// OLED helper functions moved to Renderer (setAppnameForOled, receivingServerInfoOled,
// setMenuTextForOled, refreshOled, displayUpdateFromWit)

// *********************************************************************************

void deepSleepStart() {
  deepSleepStart(SLEEP_REASON_COMMAND);
}

void deepSleepStart(int shutdownReason) {
  int delayPeriod = 2000;
  { TitleScreen ts;
    ts.setAppHeader(appName, appVersion);
    if (shutdownReason==SLEEP_REASON_INACTIVITY) {
      ts.addBody(MSG_AUTO_SLEEP);
      delayPeriod = 10000;
    } else if (shutdownReason==SLEEP_REASON_BATTERY) {
      ts.addBody(MSG_BATTERY_SLEEP);
      delayPeriod = 10000;
    }
    ts.addBody(MSG_START_SLEEP);
    renderer.renderTitle(ts);
    renderer.renderBattery(); }
  delay(delayPeriod);

  displayDriver.setPowerSave(true);
  esp_deep_sleep_start();
}
