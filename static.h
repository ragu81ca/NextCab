// Added traditional include guard plus pragma once to satisfy Arduino preprocessor quirks
#ifndef WIT_STATIC_H_INCLUDED
#define WIT_STATIC_H_INCLUDED
#pragma once

#include <Arduino.h>

extern const String appVersion;
extern const String appName;

#ifndef USE_WIFI_COUNTRY_CODE
   #define USE_COUNTRY_CODE false
#endif
#ifndef COUNTRY_CODE
   #define COUNTRY_CODE "01"
#endif
#ifndef ENFORCED_COUNTRY_CODE
   #define ENFORCED_COUNTRY_CODE false
#endif

#ifndef MENU_TEXT_MENU
   #define MENU_TEXT_MENU                      "* Menu                # Key Defs"
#endif
#ifndef MENU_TEXT_MENU_HASH_IS_FUNCTIONS
   #define MENU_TEXT_MENU_HASH_IS_FUNCTIONS    "* Menu                          # Fn"
#endif
#ifndef MENU_TEXT_FINISH
   #define MENU_TEXT_FINISH                    "                        # Finish"
#endif
#ifndef MENU_TEXT_CANCEL
   #define MENU_TEXT_CANCEL                    "* Cancel"
#endif
#ifndef MENU_TEXT_SHOW_DIRECT
   #define MENU_TEXT_SHOW_DIRECT               "                  # Show Direct"
#endif
#ifndef MENU_TEXT_ROSTER
   #define MENU_TEXT_ROSTER                    "* Cancel      0-9      #Pg"
#endif
#ifndef MENU_TEXT_TURNOUT_LIST
   #define MENU_TEXT_TURNOUT_LIST              "* Cancel      0-9      #Pg"
#endif
#ifndef MENU_TEXT_ROUTE_LIST
   #define MENU_TEXT_ROUTE_LIST                "* Cancel      0-9      #Pg"
#endif
#ifndef MENU_TEXT_FUNCTION_LIST
   #define MENU_TEXT_FUNCTION_LIST             "* Cancel      0-9      #Pg"
#endif
#ifndef MENU_TEXT_SELECT_WIT_SERVICE
   #define MENU_TEXT_SELECT_WIT_SERVICE        "* Rescan  # Pg  E.btn OFF"
#endif
#ifndef MENU_TEXT_SELECT_WIT_ENTRY
   #define MENU_TEXT_SELECT_WIT_ENTRY          "* Back  # Go  E.btn OFF"
#endif
#ifndef MENU_TEXT_SELECT_SSIDS
   #define MENU_TEXT_SELECT_SSIDS              "* Rescan  # Pg  E.btn OFF"
#endif
#ifndef MENU_TEXT_SELECT_SSIDS_FROM_FOUND
   #define MENU_TEXT_SELECT_SSIDS_FROM_FOUND   "* Rescan  # Pg  E.btn OFF"
#endif
#ifndef MENU_TEXT_ENTER_SSID_PASSWORD
   #define MENU_TEXT_ENTER_SSID_PASSWORD       "E Chrs  E.btn Slct  # Go  * Bck"
#endif

extern const String menu_text[14];

// Menu indices (legacy numeric constants). Using a single enum class to avoid unnamed-duplicate compilation issues.
enum MenuIndex : uint8_t {
   menu_menu = 0,
   menu_menu_hash_is_functions,
   menu_finish,
   menu_cancel,
   menu_show_direct,
   menu_roster,
   menu_turnout_list,
   menu_route_list,
   menu_function_list,
   menu_select_wit_service,
   menu_select_wit_entry,
   menu_select_ssids,
   menu_select_ssids_from_found,
   menu_enter_ssid_password
};

// OLED screen identifiers
enum OledScreen : uint8_t {
   last_oled_screen_speed = 0,
   last_oled_screen_roster,
   last_oled_screen_turnout_list,
   last_oled_screen_route_list,
   last_oled_screen_function_list,
   last_oled_screen_menu,
   last_oled_screen_extra_submenu,
   last_oled_screen_all_locos,
   last_oled_screen_edit_consist,
   last_oled_screen_direct_commands
};

// Battery display options
enum ShowBattery : uint8_t {
    NONE = 0,
    ICON_ONLY = 1,
    ICON_AND_PERCENT = 2
};

// Optional battery percentage smoothing (reduces visible jitter).
// Enable and set alpha (0.0..1.0). Higher alpha reacts faster; lower alpha is smoother.
#ifndef BATTERY_SMOOTHING_ENABLED
   #define BATTERY_SMOOTHING_ENABLED true
#endif
#ifndef BATTERY_SMOOTHING_ALPHA
   #define BATTERY_SMOOTHING_ALPHA 0.2f
#endif

#ifndef DIRECT_COMMAND_LIST
  #define DIRECT_COMMAND_LIST            "Direct Commands"
#endif 
#ifndef DIRECTION_FORWARD_TEXT
  #define DIRECTION_FORWARD_TEXT         "Fwd"
#endif 
#ifndef DIRECTION_REVERSE_TEXT
  #define DIRECTION_REVERSE_TEXT         "Rev"
#endif 
#ifndef DIRECTION_FORWARD_TEXT_SHORT
  #define DIRECTION_FORWARD_TEXT_SHORT   ">"
#endif 
#ifndef DIRECTION_REVERSE_TEXT_SHORT
  #define DIRECTION_REVERSE_TEXT_SHORT   "<"
#endif 
#ifndef DIRECTION_REVERSE_INDICATOR
  #define DIRECTION_REVERSE_INDICATOR    "'"
#endif 

// const String function_states = "fn ";

#ifndef MSG_START
  #define MSG_START                     "Start"
#endif
#ifndef MSG_BROWSING_FOR_SERVICE
  #define MSG_BROWSING_FOR_SERVICE      "Browsing for WiT services"
#endif
#ifndef MSG_BROWSING_FOR_SSIDS
  #define MSG_BROWSING_FOR_SSIDS        "Browsing for SSIDs"
#endif
#ifndef MSG_NO_SSIDS_FOUND
  #define MSG_NO_SSIDS_FOUND            "No SSIDs found"
#endif
#ifndef MSG_SSIDS_LISTED
  #define MSG_SSIDS_LISTED              "  SSIDs listed"
#endif
#ifndef MSG_SSIDS_FOUND
  #define MSG_SSIDS_FOUND               "    SSIDs found"
#endif
#ifndef MSG_BOUNJOUR_SETUP_FAILED
  #define MSG_BOUNJOUR_SETUP_FAILED     "Unable to setup Listener"
#endif
#ifndef MSG_NO_SERVICES_FOUND
  #define MSG_NO_SERVICES_FOUND         "No services found"
#endif
#ifndef MSG_NO_SERVICES_FOUND_ENTRY_REQUIRED
  #define MSG_NO_SERVICES_FOUND_ENTRY_REQUIRED "Enter witServer IP:Port"
#endif
#ifndef MSG_SERVICES_FOUND
  #define MSG_SERVICES_FOUND            " Service(s) found"
#endif
#ifndef MSG_TRYING_TO_CONNECT
  #define MSG_TRYING_TO_CONNECT         "Trying to Connect"
#endif
#ifndef MSG_CONNECTED
  #define MSG_CONNECTED                 "Connected"
#endif
#ifndef MSG_CONNECTING
  #define MSG_CONNECTING                "Connecting..."
#endif

#ifndef MSG_ADDRESS_LABEL
   #define MSG_ADDRESS_LABEL            "IP address: "
#endif
#ifndef MSG_CONNECTION_FAILED
   #define MSG_CONNECTION_FAILED        "Connection failed"
#endif
#ifndef MSG_DISCONNECTED
   #define MSG_DISCONNECTED             "Disconnected"
#endif
#ifndef MSG_AUTO_SLEEP
   #define MSG_AUTO_SLEEP               "Waited too long for Select"
#endif
#ifndef MSG_BATTERY_SLEEP
   #define MSG_BATTERY_SLEEP            "Battery critically low"
#endif
#ifndef MSG_START_SLEEP
   #define MSG_START_SLEEP              "Shutting Down.        E.btn ON"
#endif
#ifndef MSG_THROTTLE_NUMBER
   #define MSG_THROTTLE_NUMBER          "Throttle #"
#endif
#ifndef MSG_NO_LOCO_SELECTED
   #define MSG_NO_LOCO_SELECTED         "No Loco Selected"
#endif
#ifndef MSG_ENTER_PASSWORD
   #define MSG_ENTER_PASSWORD           "Enter Password"
#endif
#ifndef MSG_GUESSED_EX_CS_WIT_SERVER
   #define MSG_GUESSED_EX_CS_WIT_SERVER "'Guessed' EX-CS WiT server"
#endif
#ifndef MSG_BYPASS_WIT_SERVER_SEARCH
   #define MSG_BYPASS_WIT_SERVER_SEARCH     "Bypass WiT server search"
#endif
#ifndef MSG_NO_FUNCTIONS
   #define MSG_NO_FUNCTIONS             "Function List - No Functions"
#endif
#ifndef MSG_HEARTBEAT_CHECK_ENABLED
   #define MSG_HEARTBEAT_CHECK_ENABLED  "Heartbeat Check Enabled"
#endif
#ifndef MSG_HEARTBEAT_CHECK_DISABLED
   #define MSG_HEARTBEAT_CHECK_DISABLED "Heartbeat Check Disabled"
#endif
#ifndef MSG_RECEIVING_SERVER_DETAILS
   #define MSG_RECEIVING_SERVER_DETAILS  "Getting data from server"
#endif

extern const String label_track_power;

extern const int glyph_heartbeat_off;
extern const int glyph_track_power;
extern const int glyph_speed_step;
extern const int glyph_arrow_up;
extern const int glyph_arrow_down;
// const int glyph_direction_forward = 0x0070;
// const int glyph_direction_reverse = 0x006d;

#define SLEEP_REASON_COMMAND 0
#define SLEEP_REASON_INACTIVITY 1
#define SLEEP_REASON_BATTERY 2

// *******************************************************************************************************************
// System State Management
//
// Legacy CONNECTION_STATE_* defines have been REPLACED by SystemState enum (see src/core/SystemState.h)
// The unified SystemState enum provides clear application state tracking:
//   Boot → WiFi states → WifiConnected → Server states → Operating
// Eliminates redundant ssidConnectionState and witConnectionState variables
// *******************************************************************************************************************

#define SSID_CONNECTION_SOURCE_LIST 0
#define SSID_CONNECTION_SOURCE_BROWSE 1

#define CMD_FUNCTION 0

#define MAX_LOCOS     10  // maximum number of locos that can be added to the consist

#define MAX_FUNCTIONS 32

// if defined in config_buttons.h these values will be overwritten
//
#ifndef ENCODER_ROTATION_CLOCKWISE_IS_INCREASE_SPEED
   #define ENCODER_ROTATION_CLOCKWISE_IS_INCREASE_SPEED false // counter clockwise
#endif 
#ifndef TOGGLE_DIRECTION_ON_ENCODER_BUTTON_PRESSED_WHEN_STATIONAY
   #define TOGGLE_DIRECTION_ON_ENCODER_BUTTON_PRESSED_WHEN_STATIONAY true
#endif
// speed increase for each click of the encoder 
#if defined(SPEED_STEP)
static constexpr int speedStep = SPEED_STEP;
#else
static constexpr int speedStep = 1;
#endif

#if defined(SPEED_STEP_MULTIPLIER)
static constexpr int speedStepMultiplier = SPEED_STEP_MULTIPLIER;
#else
static constexpr int speedStepMultiplier = 3;
#endif

// Additional multiplier.  If enabled from the menu, each rotation becomes speedStep * AdditionalMultiplier
#if defined(SPEED_STEP_MULTIPLIER)
static constexpr int speedStepAdditionalMultiplier = SPEED_STEP_MULTIPLIER;
#else
static constexpr int speedStepAdditionalMultiplier = 2;
#endif
#ifndef SPEED_STEP_ADDITIONAL_MULTIPLIER
   #define SPEED_STEP_ADDITIONAL_MULTIPLIER 2
#endif

#ifdef  DISPLAY_SPEED_AS_PERCENT
   extern const bool speedDisplayAsPercent; // defined in static.cpp
#else
   extern const bool speedDisplayAsPercent; // defined in static.cpp
#endif

#ifdef  DISPLAY_SPEED_AS_0_TO_28
   extern const bool speedDisplayAs0to28; // defined in static.cpp
#else
   extern const bool speedDisplayAs0to28; // defined in static.cpp
#endif

extern String witServerIpAndPortEntryMask; // defined in static.cpp

#ifndef DEFAULT_IP_AND_PORT 
  #define DEFAULT_IP_AND_PORT ""
#endif

#ifndef SSID_CONNECTION_TIMEOUT 
  #define SSID_CONNECTION_TIMEOUT 10000
#endif

#endif // WIT_STATIC_H_INCLUDED


#ifndef SHORT_DCC_ADDRESS_LIMIT
  #define SHORT_DCC_ADDRESS_LIMIT 127
#endif

// *******************************************************************************************************************

#ifndef FONT_DEFAULT
   #define FONT_DEFAULT u8g2_font_NokiaSmallPlain_te
#endif
#define FONT_FUNCTION_INDICATORS u8g2_font_tiny_simon_tr
#define FONT_THROTTLE_NUMBER u8g2_font_neuecraft_tr
#define FONT_PASSWORD u8g2_font_9x15_tf
#define FONT_SPEED u8g2_font_profont29_mr
#define FONT_DIRECTION u8g2_font_neuecraft_tr
// #define FONT_TRACK_POWER u8g2_font_profont10_tf
#define FONT_NEXT_THROTTLE u8g2_font_6x12_m_symbols
#define FONT_GLYPHS u8g2_font_open_iconic_all_1x_t

// *******************************************************************************************************************

# define DCC_EX_TURNOUT_PREFIX ""
# define DCC_EX_ROUTE_PREFIX "R"

#define CONSIST_LEAD_LOCO 0
#define CONSIST_ALL_LOCOS 1

// *******************************************************************************************************************

#ifndef WITCONTROLLER_DEBUG 
  #define WITCONTROLLER_DEBUG    0
#endif

#ifndef WITHROTTLE_PROTOCOL_DEBUG
  #define WITHROTTLE_PROTOCOL_DEBUG   1
#endif

#ifndef DEBUG_LEVEL
  #define DEBUG_LEVEL   1
#endif

// *******************************************************************************************************************

#ifndef OUTBOUND_COMMANDS_MINIMUM_DELAY
  #define OUTBOUND_COMMANDS_MINIMUM_DELAY 50
#endif

#ifndef SEND_LEADING_CR_LF_FOR_COMMANDS
  #define SEND_LEADING_CR_LF_FOR_COMMANDS true
#endif

#ifndef ROTARY_ENCODER_DEBOUNCE_TIME
  #define ROTARY_ENCODER_DEBOUNCE_TIME 200
#endif

// *******************************************************************************************************************

#ifndef F0_LABEL
  #define F0_LABEL "Light"
#endif

#ifndef F1_LABEL
  #define F1_LABEL "Bell"
#endif

#ifndef F2_LABEL
  #define F2_LABEL "Horn"
#endif

// *******************************************************************************************************************

#ifndef ROTARY_ENCODER_DEBOUNCE_TIME
  #define ROTARY_ENCODER_DEBOUNCE_TIME 200
#endif

#ifndef SEARCH_ROSTER_ON_ENTRY_OF_DCC_ADDRESS
  #define SEARCH_ROSTER_ON_ENTRY_OF_DCC_ADDRESS false
#endif

// *******************************************************************************************************************
// if bare EC11 rotary encoder is used rather than KY040 then ESP32 GPIO internal pullups must be enabled. Set default to be false
// as the prototype build used KY040 encoder module that incorporates physical resistors

#ifndef EC11_PULLUPS_REQUIRED
  #define EC11_PULLUPS_REQUIRED false
#endif

// *******************************************************************************************************************
// consists follow functions
#ifndef CONSIST_FUNCTION_FOLLOW_F0
    #define CONSIST_FUNCTION_FOLLOW_F0                  CONSIST_ALL_LOCOS            // lights
#endif
#ifndef CONSIST_FUNCTION_FOLLOW_F1
    #define CONSIST_FUNCTION_FOLLOW_F1                  CONSIST_LEAD_LOCO            // bell
#endif
#ifndef CONSIST_FUNCTION_FOLLOW_F2
    #define CONSIST_FUNCTION_FOLLOW_F2                  CONSIST_LEAD_LOCO            // horn
#endif
#ifndef CONSIST_FUNCTION_FOLLOW_F3
    #define CONSIST_FUNCTION_FOLLOW_F3                  CONSIST_LEAD_LOCO
#endif
#ifndef CONSIST_FUNCTION_FOLLOW_F4
    #define CONSIST_FUNCTION_FOLLOW_F4                  CONSIST_LEAD_LOCO
#endif
#ifndef CONSIST_FUNCTION_FOLLOW_F5
    #define CONSIST_FUNCTION_FOLLOW_F5                  CONSIST_LEAD_LOCO
#endif
#ifndef CONSIST_FUNCTION_FOLLOW_F6
    #define CONSIST_FUNCTION_FOLLOW_F6                  CONSIST_LEAD_LOCO
#endif
#ifndef CONSIST_FUNCTION_FOLLOW_F7
    #define CONSIST_FUNCTION_FOLLOW_F7                  CONSIST_LEAD_LOCO
#endif
#ifndef CONSIST_FUNCTION_FOLLOW_F8
    #define CONSIST_FUNCTION_FOLLOW_F8                  CONSIST_LEAD_LOCO
#endif
#ifndef CONSIST_FUNCTION_FOLLOW_F9
    #define CONSIST_FUNCTION_FOLLOW_F9                  CONSIST_LEAD_LOCO
#endif
#ifndef CONSIST_FUNCTION_FOLLOW_F10
    #define CONSIST_FUNCTION_FOLLOW_F10                  CONSIST_LEAD_LOCO
#endif
#ifndef CONSIST_FUNCTION_FOLLOW_F11
    #define CONSIST_FUNCTION_FOLLOW_F11                  CONSIST_LEAD_LOCO
#endif
#ifndef CONSIST_FUNCTION_FOLLOW_F12
    #define CONSIST_FUNCTION_FOLLOW_F12                  CONSIST_LEAD_LOCO
#endif
#ifndef CONSIST_FUNCTION_FOLLOW_F13
    #define CONSIST_FUNCTION_FOLLOW_F13                  CONSIST_LEAD_LOCO
#endif
#ifndef CONSIST_FUNCTION_FOLLOW_F14
    #define CONSIST_FUNCTION_FOLLOW_F14                  CONSIST_LEAD_LOCO
#endif
#ifndef CONSIST_FUNCTION_FOLLOW_F15
    #define CONSIST_FUNCTION_FOLLOW_F15                  CONSIST_LEAD_LOCO
#endif
#ifndef CONSIST_FUNCTION_FOLLOW_F16
    #define CONSIST_FUNCTION_FOLLOW_F16                  CONSIST_LEAD_LOCO
#endif
#ifndef CONSIST_FUNCTION_FOLLOW_F17
    #define CONSIST_FUNCTION_FOLLOW_F17                  CONSIST_LEAD_LOCO
#endif
#ifndef CONSIST_FUNCTION_FOLLOW_F18
    #define CONSIST_FUNCTION_FOLLOW_F18                  CONSIST_LEAD_LOCO
#endif
#ifndef CONSIST_FUNCTION_FOLLOW_F19
    #define CONSIST_FUNCTION_FOLLOW_F19                  CONSIST_LEAD_LOCO
#endif
#ifndef CONSIST_FUNCTION_FOLLOW_F20
    #define CONSIST_FUNCTION_FOLLOW_F20                  CONSIST_LEAD_LOCO
#endif
#ifndef CONSIST_FUNCTION_FOLLOW_F21
    #define CONSIST_FUNCTION_FOLLOW_F21                  CONSIST_LEAD_LOCO
#endif
#ifndef CONSIST_FUNCTION_FOLLOW_F22
    #define CONSIST_FUNCTION_FOLLOW_F22                  CONSIST_LEAD_LOCO
#endif
#ifndef CONSIST_FUNCTION_FOLLOW_F23
    #define CONSIST_FUNCTION_FOLLOW_F23                  CONSIST_LEAD_LOCO
#endif
#ifndef CONSIST_FUNCTION_FOLLOW_F24
    #define CONSIST_FUNCTION_FOLLOW_F24                  CONSIST_LEAD_LOCO
#endif
#ifndef CONSIST_FUNCTION_FOLLOW_F25
    #define CONSIST_FUNCTION_FOLLOW_F25                  CONSIST_LEAD_LOCO
#endif
#ifndef CONSIST_FUNCTION_FOLLOW_F26
    #define CONSIST_FUNCTION_FOLLOW_F26                  CONSIST_LEAD_LOCO
#endif
#ifndef CONSIST_FUNCTION_FOLLOW_F27
    #define CONSIST_FUNCTION_FOLLOW_F27                  CONSIST_LEAD_LOCO
#endif
#ifndef CONSIST_FUNCTION_FOLLOW_F28
    #define CONSIST_FUNCTION_FOLLOW_F28                  CONSIST_LEAD_LOCO
#endif
#ifndef CONSIST_FUNCTION_FOLLOW_F29
    #define CONSIST_FUNCTION_FOLLOW_F29                  CONSIST_LEAD_LOCO
#endif
#ifndef CONSIST_FUNCTION_FOLLOW_F30
    #define CONSIST_FUNCTION_FOLLOW_F30                  CONSIST_LEAD_LOCO
#endif
#ifndef CONSIST_FUNCTION_FOLLOW_F31
    #define CONSIST_FUNCTION_FOLLOW_F31                  CONSIST_LEAD_LOCO
#endif



// *******************************************************************************************************************
// other options
#ifndef HASH_SHOWS_FUNCTIONS_INSTEAD_OF_KEY_DEFS
    #define HASH_SHOWS_FUNCTIONS_INSTEAD_OF_KEY_DEFS false  // default if not defined in config_buttons.h
#endif

#ifndef MAX_THROTTLES
    #define MAX_THROTTLES 2  // default if not defined in config_buttons.h
#endif

#ifndef ENCODER_BUTTON_ACTION
    #define ENCODER_BUTTON_ACTION SPEED_STOP_THEN_TOGGLE_DIRECTION  // default if not defined in config_buttons.h
#endif

// *******************************************************************************************************************
// OLED

// Extern declaration for global display instance created in display_init.cpp
#ifndef USE_TFT_ESPI
// U8g2 / OLED path — declare the concrete U8G2 type so legacy code can reference u8g2
#ifdef U8X8_HAVE_HW_SPI
    #include <SPI.h>                       // add to include path [Arduino install]\hardware\arduino\avr\libraries\SPI\src
#endif
#ifdef U8X8_HAVE_HW_I2C
    #include <Wire.h>                      // add to include path [Arduino install]\hardware\arduino\avr\libraries\Wire\src
#endif
#include <U8g2lib.h>

#ifdef OLED_TYPE
// NOTE: This assumes the configured OLED_TYPE in config_buttons.h corresponds to U8G2_SSD1309_128X64_NONAME2_F_HW_I2C; adjust if you change OLED_TYPE.
extern U8G2_SSD1309_128X64_NONAME2_F_HW_I2C u8g2;
#else
extern U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2;
#endif

// U8g2 Constructor List (Frame Buffer)
// you can overide this in config_buttons.h     DO NOT CHANGE IT HERE
// Display object defined in display_init.cpp (concrete type chosen via OLED_TYPE macro or default).
// Intentionally no extern declaration here to avoid type conflicts with user-specified constructor macro.

#endif // !USE_TFT_ESPI

// *******************************************************************************************************************
// additional / optional commands
//  these can be any legigitmate WiThrottle protocol command
// refer to https://www.jmri.org/help/en/package/jmri/jmrit/withrottle/Protocol.shtml

#ifndef CUSTOM_COMMAND_1
   #define CUSTOM_COMMAND_1 ""
#endif 
#ifndef CUSTOM_COMMAND_2
   #define CUSTOM_COMMAND_2 ""
#endif 
#ifndef CUSTOM_COMMAND_3
   #define CUSTOM_COMMAND_3 ""
#endif 
#ifndef CUSTOM_COMMAND_4
   #define CUSTOM_COMMAND_4 ""
#endif 
#ifndef CUSTOM_COMMAND_5
   #define CUSTOM_COMMAND_5 ""
#endif 
#ifndef CUSTOM_COMMAND_6
   #define CUSTOM_COMMAND_6 ""
#endif 
#ifndef CUSTOM_COMMAND_7
   #define CUSTOM_COMMAND_7 ""
#endif 

// *******************************************************************************************************************
// default direct functions

// Labels that will appear in the UI
// These should match the actual  command below

#ifndef CHOSEN_KEYPAD_DISPLAY_PREFIX
   #define CHOSEN_KEYPAD_DISPLAY_PREFIX "* Menu"
#endif
#ifndef CHOSEN_KEYPAD_DISPLAY_SUFIX
   #define CHOSEN_KEYPAD_DISPLAY_SUFIX "# This"
#endif

#ifndef CHOSEN_KEYPAD_0_DISPLAY_NAME
   #define CHOSEN_KEYPAD_0_DISPLAY_NAME "0 Lights"
#endif
#ifndef CHOSEN_KEYPAD_1_DISPLAY_NAME
   #define CHOSEN_KEYPAD_1_DISPLAY_NAME "1 Bell"
#endif
#ifndef CHOSEN_KEYPAD_2_DISPLAY_NAME
   #define CHOSEN_KEYPAD_2_DISPLAY_NAME "2 Horn"
#endif
#ifndef CHOSEN_KEYPAD_3_DISPLAY_NAME
   #define CHOSEN_KEYPAD_3_DISPLAY_NAME "3 F3"
#endif
#ifndef CHOSEN_KEYPAD_4_DISPLAY_NAME
   #define CHOSEN_KEYPAD_4_DISPLAY_NAME "4 F4"
#endif
#ifndef CHOSEN_KEYPAD_5_DISPLAY_NAME
   #define CHOSEN_KEYPAD_5_DISPLAY_NAME "5 Nxt Ttl"
#endif
#ifndef CHOSEN_KEYPAD_6_DISPLAY_NAME
   #define CHOSEN_KEYPAD_6_DISPLAY_NAME "6 X Spd"
#endif
#ifndef CHOSEN_KEYPAD_7_DISPLAY_NAME
   #define CHOSEN_KEYPAD_7_DISPLAY_NAME "7 Rev"
#endif
#ifndef CHOSEN_KEYPAD_8_DISPLAY_NAME
   #define CHOSEN_KEYPAD_8_DISPLAY_NAME "8 Estop"
#endif
#ifndef CHOSEN_KEYPAD_9_DISPLAY_NAME
   #define CHOSEN_KEYPAD_9_DISPLAY_NAME "9 Fwd"
#endif

// actual commands

#ifndef CHOSEN_KEYPAD_0_FUNCTION
   #define CHOSEN_KEYPAD_0_FUNCTION     FUNCTION_0
#endif
#ifndef CHOSEN_KEYPAD_1_FUNCTION
   #define CHOSEN_KEYPAD_1_FUNCTION     FUNCTION_1
#endif
#ifndef CHOSEN_KEYPAD_2_FUNCTION
   #define CHOSEN_KEYPAD_2_FUNCTION     FUNCTION_2
#endif
#ifndef CHOSEN_KEYPAD_3_FUNCTION
   #define CHOSEN_KEYPAD_3_FUNCTION     FUNCTION_3
#endif
#ifndef CHOSEN_KEYPAD_4_FUNCTION
   #define CHOSEN_KEYPAD_4_FUNCTION     FUNCTION_4
#endif
#ifndef CHOSEN_KEYPAD_5_FUNCTION
   #define CHOSEN_KEYPAD_5_FUNCTION     NEXT_THROTTLE
#endif
#ifndef CHOSEN_KEYPAD_6_FUNCTION
   #define CHOSEN_KEYPAD_6_FUNCTION     SPEED_MULTIPLIER
#endif
#ifndef CHOSEN_KEYPAD_7_FUNCTION
   #define CHOSEN_KEYPAD_7_FUNCTION     DIRECTION_REVERSE
#endif
#ifndef CHOSEN_KEYPAD_8_FUNCTION
   #define CHOSEN_KEYPAD_8_FUNCTION     E_STOP
#endif
#ifndef CHOSEN_KEYPAD_9_FUNCTION
   #define CHOSEN_KEYPAD_9_FUNCTION     DIRECTION_FORWARD
#endif
#ifndef CHOSEN_KEYPAD_A_FUNCTION
   #define CHOSEN_KEYPAD_A_FUNCTION     CUSTOM_1
#endif
#ifndef CHOSEN_KEYPAD_B_FUNCTION
   #define CHOSEN_KEYPAD_B_FUNCTION     CUSTOM_2
#endif
#ifndef CHOSEN_KEYPAD_C_FUNCTION
   #define CHOSEN_KEYPAD_C_FUNCTION     CUSTOM_3
#endif
#ifndef CHOSEN_KEYPAD_D_FUNCTION
   #define CHOSEN_KEYPAD_D_FUNCTION     CUSTOM_4
#endif


// *******************************************************************************************************************
// additional / optional buttons hardware

// This format for the additional buttons is now depriated
// Use the *New Additional / optional buttons* section instead

// To use the additional buttons, adjust the functions assigned to them in config_buttons.h
#ifndef MAX_ADDITIONAL_BUTTONS
    #define MAX_ADDITIONAL_BUTTONS 7  
#endif
#ifndef ADDITIONAL_BUTTONS_PINS
    #define ADDITIONAL_BUTTONS_PINS      {5,15,25,26,27,32,33}
#endif
#ifndef ADDITIONAL_BUTTONS_TYPE
    #define ADDITIONAL_BUTTONS_TYPE      {INPUT_PULLUP,INPUT_PULLUP,INPUT_PULLUP,INPUT_PULLUP,INPUT_PULLUP,INPUT_PULLUP,INPUT_PULLUP}
   // 34,35,36,39 don't have an internal pullup
#endif

#ifndef ADDITIONAL_BUTTON_DEBOUNCE_DELAY
    #define ADDITIONAL_BUTTON_DEBOUNCE_DELAY 50   // default if not defined in config_buttons.h
#endif

// *******************************************************************************************************************

// This format for the additional buttons is now depriated
// Use the *New Additional / optional buttons* section instead

#ifndef CHOSEN_ADDITIONAL_BUTTON_0_FUNCTION
  #define CHOSEN_ADDITIONAL_BUTTON_0_FUNCTION FUNCTION_NULL
#endif
#ifndef CHOSEN_ADDITIONAL_BUTTON_1_FUNCTION
  #define CHOSEN_ADDITIONAL_BUTTON_1_FUNCTION FUNCTION_NULL
#endif
#ifndef CHOSEN_ADDITIONAL_BUTTON_2_FUNCTION
  #define CHOSEN_ADDITIONAL_BUTTON_2_FUNCTION FUNCTION_NULL
#endif
#ifndef CHOSEN_ADDITIONAL_BUTTON_3_FUNCTION
  #define CHOSEN_ADDITIONAL_BUTTON_3_FUNCTION FUNCTION_NULL
#endif
#ifndef CHOSEN_ADDITIONAL_BUTTON_4_FUNCTION
  #define CHOSEN_ADDITIONAL_BUTTON_4_FUNCTION FUNCTION_NULL
#endif
#ifndef CHOSEN_ADDITIONAL_BUTTON_5_FUNCTION
  #define CHOSEN_ADDITIONAL_BUTTON_5_FUNCTION FUNCTION_NULL
#endif
#ifndef CHOSEN_ADDITIONAL_BUTTON_6_FUNCTION
  #define CHOSEN_ADDITIONAL_BUTTON_6_FUNCTION FUNCTION_NULL
#endif

#ifndef ADDITIONAL_BUTTON_OVERRIDE_DEFAULT_LATCHING
   #define ADDITIONAL_BUTTON_OVERRIDE_DEFAULT_LATCHING false
#endif

#ifndef ADDITIONAL_BUTTON_0_LATCHING
   #define ADDITIONAL_BUTTON_0_LATCHING true
#endif
#ifndef ADDITIONAL_BUTTON_1_LATCHING
   #define ADDITIONAL_BUTTON_1_LATCHING true
#endif
#ifndef ADDITIONAL_BUTTON_2_LATCHING
   #define ADDITIONAL_BUTTON_2_LATCHING false
#endif
#ifndef ADDITIONAL_BUTTON_3_LATCHING
   #define ADDITIONAL_BUTTON_3_LATCHING true
#endif
#ifndef ADDITIONAL_BUTTON_4_LATCHING
   #define ADDITIONAL_BUTTON_4_LATCHING true
#endif
#ifndef ADDITIONAL_BUTTON_5_LATCHING
   #define ADDITIONAL_BUTTON_5_LATCHING true
#endif
#ifndef ADDITIONAL_BUTTON_6_LATCHING
   #define ADDITIONAL_BUTTON_6_LATCHING true
#endif

// *******************************************************************************************************************
// New Additional / optional buttons

#ifndef USE_NEW_ADDITIONAL_BUTTONS_FORMAT
   #define USE_NEW_ADDITIONAL_BUTTONS_FORMAT false
#endif

#if USE_NEW_ADDITIONAL_BUTTONS_FORMAT
   #ifndef NEW_MAX_ADDITIONAL_BUTTONS

      #define NEW_MAX_ADDITIONAL_BUTTONS 0

      #define NEW_ADDITIONAL_BUTTON_ACTIONS {\
                              FUNCTION_NULL\
                              }

      #define NEW_ADDITIONAL_BUTTON_LATCHING {\
                              true\
                              }

      #define NEW_ADDITIONAL_BUTTON_PIN {\
                              -1\
                              }

      #define NEW_ADDITIONAL_BUTTON_TYPE {\
                              INPUT_PULLUP\
                              }

   #endif
#endif

// *******************************************************************************************************************

#ifndef USE_ROTARY_ENCODER_FOR_THROTTLE
   #define USE_ROTARY_ENCODER_FOR_THROTTLE true
#endif
#ifndef THROTTLE_POT_PIN
   #define THROTTLE_POT_PIN 39
#endif
#ifndef THROTTLE_POT_USE_NOTCHES
   #define THROTTLE_POT_USE_NOTCHES true
#endif
#ifndef THROTTLE_POT_NOTCH_VALUES   
   #define THROTTLE_POT_NOTCH_VALUES {1111,1347,1591,1833,2105,2379,2622,2837};
#endif
#ifndef THROTTLE_POT_NOTCH_SPEEDS
   #define THROTTLE_POT_NOTCH_SPEEDS {0,18,36,54,72,90,108,127}
#endif

// *******************************************************************************************************************

#ifndef USE_BATTERY_TEST
   #define USE_BATTERY_TEST false
#endif
#ifndef USE_MAX17048
   #define USE_MAX17048 0
#endif
#ifndef BATTERY_TEST_PIN
   #define BATTERY_TEST_PIN 34
#endif
#ifndef BATTERY_CONVERSION_FACTOR
   #define BATTERY_CONVERSION_FACTOR 1.7
#endif
#ifndef USE_BATTERY_PERCENT_AS_WELL_AS_ICON
   #define USE_BATTERY_PERCENT_AS_WELL_AS_ICON false
#endif
#ifndef USE_BATTERY_SLEEP_AT_PERCENT
   #define USE_BATTERY_SLEEP_AT_PERCENT 3
#endif

// ***************************************************
//  ESPmDNS problem

// #ifndef USING_OLDER_ESPMDNS
//    #define USING_OLDER_ESPMDNS false
// #endif

// #if USING_OLDER_ESPMDNS == true
//   #define ESPMDNS_IP_ATTRIBUTE_NAME MDNS.IP(i)
// #else
//   #define ESPMDNS_IP_ATTRIBUTE_NAME MDNS.address(i)
// #endif

#if ESP_IDF_VERSION_MAJOR < 5
  #define ESPMDNS_IP_ATTRIBUTE_NAME MDNS.IP(i)
#else
  #define ESPMDNS_IP_ATTRIBUTE_NAME MDNS.address(i)
#endif

// ***************************************************
// Pre-WiFi Inactivity Timeout

// Timeout for WiFi/password/server selection screens (2 minutes)
// Device sleeps if no user input during connection sequence
#ifndef PRE_WIFI_INACTIVITY_TIMEOUT
   #define PRE_WIFI_INACTIVITY_TIMEOUT 120000
#endif

// ***************************************************
// Heartbeat 

// max period 
#ifndef MAX_HEARTBEAT_PERIOD
   #define MAX_HEARTBEAT_PERIOD 240000
#endif

#ifndef HEARTBEAT_ENABLED
   #define HEARTBEAT_ENABLED true
#endif

#ifndef DEFAULT_HEARTBEAT_PERIOD
   #define DEFAULT_HEARTBEAT_PERIOD 10
#endif

// ***************************************************
// roster sorting

#ifndef ROSTER_SORT_SEQUENCE
   #define ROSTER_SORT_SEQUENCE 1
#endif

// ***************************************************
// startup commands

#ifndef STARTUP_COMMAND_1
   #define STARTUP_COMMAND_1 ""
#endif
#ifndef STARTUP_COMMAND_2
   #define STARTUP_COMMAND_2 ""
#endif
#ifndef STARTUP_COMMAND_3
   #define STARTUP_COMMAND_3 ""
#endif
#ifndef STARTUP_COMMAND_4
   #define STARTUP_COMMAND_4 ""
#endif

#ifndef ACQUIRE_ROSTER_ENTRY_IF_ONLY_ONE
   #define ACQUIRE_ROSTER_ENTRY_IF_ONLY_ONE false
#endif

// ***************************************************
// other text

#ifndef CONSIST_SPACE_BETWEEN_LOCOS
   #define CONSIST_SPACE_BETWEEN_LOCOS " "
#endif

// ***************************************************
// loco Acquire

#ifndef DROP_BEFORE_ACQUIRE
   #define DROP_BEFORE_ACQUIRE false
#endif

#ifndef RESTORE_ACQUIRED_LOCOS
   #define RESTORE_ACQUIRED_LOCOS true
#endif

#ifndef CONSIST_RELEASE_BY_INDEX
   #define CONSIST_RELEASE_BY_INDEX true
#endif