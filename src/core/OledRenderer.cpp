// OledRenderer.cpp relocated to src/core
#include "OledRenderer.h"
#include "UIState.h"
#include "RenderModel.h"
#include "BatteryMonitor.h"
#include "../../static.h" // provide macros before presenter uses them
#include "HeartbeatPresenter.h"
#include "heartbeat/HeartbeatMonitor.h"
#include "ThrottleManager.h" // full definition for throttleManager usage
#include "../../WiTcontroller.h"
#include "protocol/WiThrottleDelegate.h"

// u8g2 display instance declared in static.cpp with type depending on OLED configuration (see static.h)
extern BatteryMonitor batteryMonitor; 
extern HeartbeatMonitor heartbeatMonitor;
OledRenderer oledRenderer(u8g2);

OledRenderer::OledRenderer(U8G2 &d) : display(d) {}

void OledRenderer::renderArray(bool isThreeColumns, bool isPassword, bool sendBuffer, bool drawTopLine) {
	renderArrayInternal(isThreeColumns, isPassword, sendBuffer, drawTopLine);
}

void OledRenderer::renderFoundSsids(const String &soFar) {
	menuIsShowing = true;
	keypadUseType = KEYPAD_USE_SELECT_SSID_FROM_FOUND;
	if (soFar == "") {
	clearArray();
		for (int i=0; i<5 && i<foundSsidsCount; i++) {
			if (foundSsids[(page*5)+i].length()>0) {
				oledText[i] = String(i) + ": " + foundSsids[(page*5)+i] + "   (" + foundSsidRssis[(page*5)+i] + ")";
			}
		}
		oledText[5] = "(" + String(page+1) +  ") " + menu_text[menu_select_ssids_from_found];
	renderArrayInternal(false,false,true,false);
	}
}

void OledRenderer::renderRoster(const String &soFar) {
	lastOledScreen = last_oled_screen_roster;
	lastOledStringParameter = soFar;
	menuIsShowing = true;
	keypadUseType = KEYPAD_USE_SELECT_ROSTER;
	if (soFar == "") {
	clearArray();
		for (int i=0; i<5 && ((page*5)+i<rosterSize); i++) {
			int index = rosterSortedIndex[(page*5)+i];
			if (rosterAddress[index] != 0) {
				oledText[i] = String(i) + ": " + rosterName[index] + " (" + rosterAddress[index] + ")" ;
			}
		}
		oledText[5] = "(" + String(page+1) +  ") " + menu_text[menu_roster];
	renderArrayInternal(false,false,true,false);
	}
}

void OledRenderer::renderTurnoutList(const String &soFar, TurnoutAction action) {
	lastOledScreen = last_oled_screen_turnout_list;
	lastOledStringParameter = soFar;
	lastOledTurnoutParameter = action;
	menuIsShowing = true;
	keypadUseType = (action == TurnoutThrow) ? KEYPAD_USE_SELECT_TURNOUTS_THROW : KEYPAD_USE_SELECT_TURNOUTS_CLOSE;
	if (soFar == "") {
	clearArray();
		int j = 0;
		for (int i=0; i<10 && i<turnoutListSize; i++) {
			j = (i<5) ? i : i+1;
			if (turnoutListUserName[(page*10)+i].length()>0) {
				oledText[j] = String(turnoutListIndex[i]) + ": " + turnoutListUserName[(page*10)+i].substring(0,10);
			}
		}
		oledText[5] = "(" + String(page+1) +  ") " + menu_text[menu_turnout_list];
	renderArrayInternal(false,false,true,false);
	}
}

void OledRenderer::renderRouteList(const String &soFar) {
	lastOledScreen = last_oled_screen_route_list;
	lastOledStringParameter = soFar;
	menuIsShowing = true;
	keypadUseType = KEYPAD_USE_SELECT_ROUTES;
	if (soFar == "") {
	clearArray();
		int j = 0;
		for (int i=0; i<10 && i<routeListSize; i++) {
			j = (i<5) ? i : i+1;
			if (routeListUserName[(page*10)+i].length()>0) {
				oledText[j] = String(routeListIndex[i]) + ": " + routeListUserName[(page*10)+i].substring(0,10);
			}
		}
		oledText[5] =  "(" + String(page+1) +  ") " + menu_text[menu_route_list];
	renderArrayInternal(false,false,true,false);
	}
}

void OledRenderer::renderFunctionList(const String &soFar) {
	lastOledScreen = last_oled_screen_function_list;
	lastOledStringParameter = soFar;
	menuIsShowing = true;
	keypadUseType = KEYPAD_USE_SELECT_FUNCTION;
	functionHasBeenSelected = false;
	if (soFar == "") {
		clearArray();
		char currentChar = throttleManager.getCurrentThrottleChar();
		int currentIdx = throttleManager.getCurrentThrottleIndex();
		if (wiThrottleProtocol.getNumberOfLocomotives(currentChar) > 0 ) {
			int j = 0; int k = 0;
			for (int i=0; i<10; i++) {
				k = (functionPage*10) + i;
				if (k < MAX_FUNCTIONS) {
					j = (i<5) ? i : i+1;
					oledText[j] = String(i) + ": " + ((k<10) ? functionLabels[currentIdx][k].substring(0,10) : String(k) + "-" + functionLabels[currentIdx][k].substring(0,7));
					if (functionStates[currentIdx][k]) {
						oledTextInvert[j] = true;
					}
				}
			}
			oledText[5] = "(" + String(functionPage) +  ") " + menu_text[menu_function_list];
		} else {
			oledText[0] = MSG_NO_FUNCTIONS;
			oledText[2] = MSG_THROTTLE_NUMBER + String(currentIdx+1);
			oledText[3] = MSG_NO_LOCO_SELECTED;
			setMenuTextForOled(menu_cancel);
		}
		renderArrayInternal(false,false,true,false);
	}
}

void OledRenderer::renderEnterPassword() {
	keypadUseType = KEYPAD_USE_ENTER_SSID_PASSWORD;
	encoderUseType = KEYPAD_USE_ENTER_SSID_PASSWORD;
	clearArray();
	String temp = ssidPasswordEntered+ssidPasswordCurrentChar;
	if (temp.length()>12) { temp = "\253"+temp.substring(temp.length()-12); } else { temp = " "+temp; }
	oledText[0] = MSG_ENTER_PASSWORD;
	oledText[2] = temp;
	setMenuTextForOled(menu_enter_ssid_password);
	renderArrayInternal(false,true,true,false);
}

void OledRenderer::renderFunctions() {
	lastOledScreen = last_oled_screen_speed;
	int currentIdx = throttleManager.getCurrentThrottleIndex();
	for (int i=0; i < MAX_FUNCTIONS; i++) if (functionStates[currentIdx][i]) { display.drawRBox(i*4+12,12+1,5,7,2); display.setDrawColor(0); display.setFont(FONT_FUNCTION_INDICATORS); display.drawUTF8( i*4+1+12, 18+1, String( (i<10) ? i : ((i<20) ? i-10 : i-20)).c_str()); display.setDrawColor(1); }
}

void OledRenderer::renderEditConsist() {
	lastOledScreen = last_oled_screen_edit_consist; menuIsShowing = false; clearArray(); keypadUseType = KEYPAD_USE_EDIT_CONSIST; renderAllLocos(true); oledText[0] = MENU_ITEM_TEXT_TITLE_EDIT_CONSIST; oledText[5] = MENU_ITEM_TEXT_MENU_EDIT_CONSIST; renderArrayInternal(false,false,true,false);
}

void OledRenderer::renderHeartbeatCheck() {
	menuIsShowing = false; RenderModel model; buildHeartbeatRenderModel(model, uiState, heartbeatMonitor.enabled()); for (int i=0;i<18;i++){ oledText[i] = model.lines[i]; oledTextInvert[i] = model.invert[i]; } renderArrayInternal(false,false,model.sendBuffer,model.drawTopLine);
}

void OledRenderer::renderAllLocosScreen(bool hideLeadLoco) {
	// Fill buffer then draw using existing array renderer (no immediate sendBuffer to allow caller composition)
	clearArray();
	renderAllLocos(hideLeadLoco);
	renderArrayInternal(false,false,true,true); // drawTopLine true so battery bar etc. appears consistently
}

void OledRenderer::renderAllLocos(bool hideLeadLoco) {
	lastOledScreen = last_oled_screen_all_locos; lastOledBoolParameter = hideLeadLoco; int startAt = (hideLeadLoco) ? 1 : 0; String loco; int j=0; int i=0; char currentChar = throttleManager.getCurrentThrottleChar(); if (wiThrottleProtocol.getNumberOfLocomotives(currentChar) > 0) { for (int index=0; ((index < wiThrottleProtocol.getNumberOfLocomotives(currentChar)) && (i < 8)); index++) { j = (i<4) ? i : i+2; loco = wiThrottleProtocol.getLocomotiveAtPosition(currentChar, index); if (i>=startAt) { oledText[j+1] = String(i) + ": " + loco; if (wiThrottleProtocol.getDirection(currentChar, loco) == Reverse) oledTextInvert[j+1] = true; } i++; } }
}

void OledRenderer::renderMenu(const String &soFar, bool primeMenu) {
	lastOledStringParameter = soFar; int offset = 0; lastOledScreen = last_oled_screen_menu; if (!primeMenu) { offset = 10; lastOledScreen = last_oled_screen_extra_submenu; } menuIsShowing = true; bool drawTopLine = false; if ( (soFar == "") || ((!primeMenu) && (soFar.length()==1)) ) { clearArray(); int j=0; for (int i=1+offset; i<10+offset; i++) { j = (i<6+offset) ? i-offset : i+1-offset; oledText[j-1] = String(i-offset) + ": " + menuText[i][0]; } oledText[10] = "0: " + menuText[0+offset][0]; setMenuTextForOled(menu_cancel); renderArrayInternal(false,false,true,false); } else { int cmd = menuCommand.substring(0,1).toInt(); clearArray(); oledText[0] = ">> " + menuText[cmd][0] + ":"; oledText[6] = menuCommand.substring(1, menuCommand.length()); oledText[5] = menuText[cmd+offset][1]; char currentChar = throttleManager.getCurrentThrottleChar(); int currentIdx = throttleManager.getCurrentThrottleIndex(); switch (soFar.charAt(0)) { case MENU_ITEM_DROP_LOCO: { if (wiThrottleProtocol.getNumberOfLocomotives(currentChar) > 0) { renderAllLocos(false); drawTopLine = true; } } case MENU_ITEM_FUNCTION: case MENU_ITEM_TOGGLE_DIRECTION: { if (wiThrottleProtocol.getNumberOfLocomotives(currentChar) <= 0) { oledText[2] = MSG_THROTTLE_NUMBER + String(currentIdx+1); oledText[3] = MSG_NO_LOCO_SELECTED; setMenuTextForOled(menu_cancel); } break; } } renderArrayInternal(false,false,true,drawTopLine); }
}

void OledRenderer::renderArrayInternal(bool isThreeColumns, bool isPassword, bool sendBuffer, bool drawTopLine) {
	display.clearBuffer(); display.setFont(FONT_DEFAULT); int x = 0; int y = 10; int xInc = 64; int max = 12; if (isThreeColumns) { xInc = 42; max = 18; } for (int i=0; i < max; i++) { const char *cLine = oledText[i].c_str(); if (isPassword && i==2) display.setFont(FONT_PASSWORD); if (oledTextInvert[i]) { display.drawBox(x,y-8,62,10); display.setDrawColor(0); } display.drawUTF8(x,y,cLine); display.setDrawColor(1); if (isPassword && i==2) display.setFont(FONT_DEFAULT); y += 10; if ((i==5) || (i==11)) { x += xInc; y = 10; } } if (drawTopLine) { display.drawHLine(0,11,128); renderBattery(); } display.drawHLine(0,51,128); if (sendBuffer) display.sendBuffer(); }

void OledRenderer::clearArray() { uiState.clearLines(); }

void OledRenderer::renderDirectCommands() { lastOledScreen = last_oled_screen_direct_commands; oledDirectCommandsAreBeingDisplayed = true; clearArray(); oledText[0] = DIRECT_COMMAND_LIST; for (int i=0; i<4; i++) oledText[i+1] = directCommandText[i][0]; int j=0; for (int i=6; i<10; i++) { oledText[i+1] = directCommandText[j][1]; j++; } j=0; for (int i=12; i<16; i++) { oledText[i+1] = directCommandText[j][2]; j++; } renderArray(true,false); menuCommandStarted = false; }

void OledRenderer::renderBattery() {
	if (!(batteryMonitor.enabled() && (batteryMonitor.displayMode()!=NONE) && (batteryMonitor.lastCheckMillis()>0))) return;
	// We will right-align content but enforce a 1-pixel margin so glyphs never hit column 127 (avoid panel wrap quirk).
	const uint8_t screenWidth = 128;
	const uint8_t rightMargin = 2; // keep 2px clear at far right (avoid panel wrap on column 127)
	int pctRaw = batteryMonitor.percent();
	bool showPct = (batteryMonitor.displayMode()==ICON_AND_PERCENT);

	// Measure components
	display.setFont(FONT_GLYPHS);
	const char *icon = "Z";
	int iconWidth = display.getStrWidth(icon); // battery glyph width
	String pctStr;
	int pctWidth = 0;
	if (showPct) {
		display.setFont(FONT_FUNCTION_INDICATORS);
		pctStr = (pctRaw < 5) ? "LOW" : (String(pctRaw) + "%");
		pctWidth = display.getStrWidth(pctStr.c_str());
		display.setFont(FONT_GLYPHS); // restore for icon draw
	}
	int spacing = showPct ? 2 : 0;
	int totalWidth = iconWidth + spacing + pctWidth;
	if (totalWidth > (screenWidth - rightMargin)) {
		// Fallback: drop percent if it can't fit cleanly
		showPct = false;
		totalWidth = iconWidth;
	}
	int leftX = (int)screenWidth - (int)rightMargin - totalWidth; // start position ensuring right margin
	if (leftX < 0) leftX = 0; // safety
	int yIcon = 11;

	// Draw icon
	display.setDrawColor(1);
	display.setFont(FONT_GLYPHS);
	display.drawStr(leftX, yIcon, icon);
	// Battery fill bars (keep within icon region)
	if (pctRaw>10) display.drawLine(leftX+1,yIcon-6,leftX+1,yIcon-3);
	if (pctRaw>25) display.drawLine(leftX+2,yIcon-6,leftX+2,yIcon-3);
	if (pctRaw>50) display.drawLine(leftX+3,yIcon-6,leftX+3,yIcon-3);
	if (pctRaw>75) display.drawLine(leftX+4,yIcon-6,leftX+4,yIcon-3);
	if (pctRaw>90) display.drawLine(leftX+5,yIcon-6,leftX+5,yIcon-3);

	// Draw percent if enabled and still fits
	if (showPct) {
		display.setFont(FONT_FUNCTION_INDICATORS);
		int pctX = leftX + iconWidth + spacing;
		int pctY = 10; // baseline alignment for small font
		// Clamp percent start if rounding produced an out-of-range position
	// Ensure percent text does not draw into last two columns
	int maxRight = screenWidth - rightMargin - 1; // final drawable column for percent
	if (pctX + pctWidth - 1 > maxRight) pctX = maxRight - pctWidth + 1;
	if (pctX < leftX + iconWidth + spacing) pctX = leftX + iconWidth + spacing; // maintain spacing
		display.drawStr(pctX, pctY, pctStr.c_str());
	}
}

void OledRenderer::renderSpeedStepMultiplier() { if (speedStep != throttleManager.getSpeedStep()) { display.setDrawColor(1); display.setFont(FONT_GLYPHS); display.drawGlyph(1,38,glyph_speed_step); display.setFont(FONT_DEFAULT); display.drawStr(9,37,String(throttleManager.getSpeedStep()).c_str()); } }

void OledRenderer::renderSpeed() {
	lastOledScreen = last_oled_screen_speed; menuIsShowing = false; String sLocos=""; String sSpeed=""; String sDirection=""; String sep=" "; bool foundNext=false; String nextNo=""; String nextSpeedDir=""; clearArray(); bool drawTop=false; int currentIdx = throttleManager.getCurrentThrottleIndex(); char currentChar = throttleManager.getCurrentThrottleChar(); if (wiThrottleProtocol.getNumberOfLocomotives(currentChar) > 0) { for (int i=0;i<wiThrottleProtocol.getNumberOfLocomotives(currentChar); i++) { sLocos += sep + getDisplayLocoString(currentIdx,i); sep = CONSIST_SPACE_BETWEEN_LOCOS; } sSpeed = String(throttleManager.getDisplaySpeed(currentIdx)); Direction dir = throttleManager.directions()[currentIdx]; sDirection = (dir==Forward)? DIRECTION_FORWARD_TEXT : DIRECTION_REVERSE_TEXT; if (throttleManager.getMaxThrottles()>1) { int nextIdx = currentIdx+1; for (int i=nextIdx;i<throttleManager.getMaxThrottles();i++) if (wiThrottleProtocol.getNumberOfLocomotives(getMultiThrottleChar(i))>0) { foundNext=true; nextIdx=i; break; } if (!foundNext && currentIdx>0) { for (int i=0;i<currentIdx;i++) if (wiThrottleProtocol.getNumberOfLocomotives(getMultiThrottleChar(i))>0) { foundNext=true; nextIdx=i; break; } } if (foundNext) { nextNo = String(nextIdx+1); int sp = throttleManager.getDisplaySpeed(nextIdx); nextSpeedDir = String(sp); Direction nextDir = throttleManager.directions()[nextIdx]; if (nextDir==Forward) nextSpeedDir += DIRECTION_FORWARD_TEXT_SHORT; else nextSpeedDir = DIRECTION_REVERSE_TEXT_SHORT + nextSpeedDir; nextSpeedDir = String("     ") + nextSpeedDir; nextSpeedDir = nextSpeedDir.substring(nextSpeedDir.length()-5); } } oledText[0] = "   " + sLocos; drawTop=true; } else { setAppnameForOled(); oledText[2] = MSG_THROTTLE_NUMBER + String(currentIdx+1); oledText[3] = MSG_NO_LOCO_SELECTED; drawTop=true; } if (!hashShowsFunctionsInsteadOfKeyDefs) setMenuTextForOled(menu_menu); else setMenuTextForOled(menu_menu_hash_is_functions); renderArrayInternal(false,false,false,drawTop); if (wiThrottleProtocol.getNumberOfLocomotives(currentChar) > 0) { renderFunctions(); display.setDrawColor(0); display.drawBox(0,0,12,16); display.setDrawColor(1); display.setFont(FONT_THROTTLE_NUMBER); display.drawStr(2,15,String(currentIdx+1).c_str()); } renderBattery(); renderSpeedStepMultiplier(); if (trackPower==PowerOn) { display.drawRBox(0,40,9,9,1); display.setDrawColor(0); } display.setFont(FONT_GLYPHS); display.drawGlyph(1,48,glyph_track_power); display.setDrawColor(1); if (!heartbeatMonitor.enabled()) { display.setFont(FONT_GLYPHS); display.drawGlyph(13,49,glyph_heartbeat_off); display.setDrawColor(2); display.drawLine(13,48,20,41); display.setDrawColor(1); } display.setFont(FONT_DIRECTION); display.drawStr(79,36,sDirection.c_str()); const char *cSpeed = sSpeed.c_str(); display.setFont(FONT_SPEED); int width = display.getStrWidth(cSpeed); display.drawStr(22+(55-width),45,cSpeed); if (throttleManager.getMaxThrottles()>1 && foundNext) { display.setFont(FONT_NEXT_THROTTLE); display.drawStr(85+34,38,nextNo.c_str()); display.drawStr(85+12,48,nextSpeedDir.c_str()); } display.sendBuffer(); }

