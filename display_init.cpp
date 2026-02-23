// Concrete display object definition
#include <Arduino.h>
#include <U8g2lib.h>
#include "config_buttons.h"
#include "src/core/DisplayConfig.h"

// ── U8G2 hardware instance ──
#ifndef OLED_TYPE
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE, /* clock=*/ 22, /* data=*/ 23);
#else
OLED_TYPE // user macro expands to concrete declaration with name u8g2
#endif

// ── Display abstraction layer instances ──
// These are the global instances referenced via extern in DisplayConfig.h.
// Renderer and all other code should use these rather than u8g2 directly.
static DisplayDriver_U8g2 _displayDriverInstance(u8g2);

const DisplayLayout& activeLayout = ACTIVE_LAYOUT;
const FontSet& activeFonts = ACTIVE_FONTS;
DisplayDriver& displayDriver = _displayDriverInstance;
