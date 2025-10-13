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
