#include "static.h"

// Centralized definitions to avoid multiple definition errors
const String appVersion = "v1.93";
#ifndef CUSTOM_APPNAME
const String appName = "WiTcontroller";
#else
const String appName = CUSTOM_APPNAME;
#endif

const String menu_text[14] = {
  MENU_TEXT_MENU,
  MENU_TEXT_MENU_HASH_IS_FUNCTIONS,
  MENU_TEXT_FINISH,
  MENU_TEXT_CANCEL,
  MENU_TEXT_SHOW_DIRECT,
  MENU_TEXT_ROSTER,
  MENU_TEXT_TURNOUT_LIST,
  MENU_TEXT_ROUTE_LIST,
  MENU_TEXT_FUNCTION_LIST,
  MENU_TEXT_SELECT_WIT_SERVICE,
  MENU_TEXT_SELECT_WIT_ENTRY,
  MENU_TEXT_SELECT_SSIDS,
  MENU_TEXT_SELECT_SSIDS_FROM_FOUND,
  MENU_TEXT_ENTER_SSID_PASSWORD
};

// Definition for extern declared in static.h
String witServerIpAndPortEntryMask = "###.###.###.###:#####";

// Added moved constant definitions from static.h to resolve Arduino IDE multiple inclusion redefinition errors
const String label_track_power = "TRK";

const int glyph_heartbeat_off = 0x00b7;
const int glyph_track_power = 0x00eb;
const int glyph_speed_step = 0x00d6;

#ifdef  DISPLAY_SPEED_AS_PERCENT
const bool speedDisplayAsPercent = DISPLAY_SPEED_AS_PERCENT;
#else
const bool speedDisplayAsPercent = false;
#endif

#ifdef  DISPLAY_SPEED_AS_0_TO_28
const bool speedDisplayAs0to28 = DISPLAY_SPEED_AS_0_TO_28;
#else
const bool speedDisplayAs0to28 = false;
#endif

const char ssidPasswordBlankChar = 164;
