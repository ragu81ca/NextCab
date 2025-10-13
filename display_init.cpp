// Concrete display object definition
#include <Arduino.h>
#include <U8g2lib.h>
#include "config_buttons.h"

#ifndef OLED_TYPE
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE, /* clock=*/ 22, /* data=*/ 23);
#else
OLED_TYPE // user macro expands to concrete declaration with name u8g2
#endif
