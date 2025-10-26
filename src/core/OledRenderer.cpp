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
#include "menu/MenuSystem.h"
#include "menu/MenuItem.h"

// u8g2 display instance declared in static.cpp with type depending on OLED configuration (see static.h)
extern BatteryMonitor batteryMonitor; 
extern HeartbeatMonitor heartbeatMonitor;
// Global instance now defined in WiTcontroller.ino to avoid multiple definition.

OledRenderer::OledRenderer(U8G2 &d) : display(d) {}

void OledRenderer::renderArray(bool isThreeColumns, bool isPassword, bool sendBuffer, bool drawTopLine) {
	renderArrayInternal(isThreeColumns, isPassword, sendBuffer, drawTopLine);
}

// New wrapper for menu rendering (replaces previous inline logic from sketch)
void OledRenderer::renderNewMenu(MenuSystem& menuSys) {
	menuIsShowing = true;
	clearArray();
	
	if (!menuSys.isActive()) {
		// Menu not active, shouldn't be called
		return;
	}
	
	const MenuItem* currentMenu = menuSys.getCurrentMenu();
	uint8_t menuSize = menuSys.getCurrentMenuSize();
	const MenuItem* currentItem = menuSys.getCurrentItem();
	String input = menuSys.getAccumulatedInput();
	
	// Check if we're in TEXT_INPUT mode (single item pushed onto stack, depth > 0)
	bool isInputMode = (menuSys.getStackDepth() > 0 && currentItem && currentItem->type == MenuItemType::TEXT_INPUT);
	
	if (isInputMode) {
		// Showing input prompt for TEXT_INPUT item
		oledText[0] = currentItem->title;
		oledText[2] = input + "_";  // Show cursor
		oledText[5] = currentItem->instructions;
	} else {
		// Display menu items (main menu or submenu)
		// Layout: Items 1-5 in lines 0-4, items 6-9 in lines 6-9, item 0 (10th) in line 10
		// Line 5 is for instructions
		for (uint8_t i = 0; i < menuSize && i < 10; i++) {
			int displayLine;
			String itemNum;
			
			if (i < 5) {
				// Items 0-4 → display as "1:" through "5:" in lines 0-4
				displayLine = i;
				itemNum = String(i + 1);
			} else if (i < 9) {
				// Items 5-8 → display as "6:" through "9:" in lines 6-9
				displayLine = i + 1;  // Skip line 5 (instructions)
				itemNum = String(i + 1);
			} else {
				// Item 9 → display as "0:" in line 10
				displayLine = 10;
				itemNum = "0";
			}
			
			String line = itemNum + ": " + currentMenu[i].title;
			oledText[displayLine] = line;
		}
		// Bottom instruction at line 5
		oledText[5] = "* Cancel  # Select";
	}
	
	renderArrayInternal(false,false,true,false);
}

// Public wrapper that records state then calls private renderAllLocos
void OledRenderer::renderAllLocosScreen(bool hideLeadLoco) {
	clearArray();
	renderAllLocos(hideLeadLoco);
	// Provide a header/menu line for consist editing or loco overview
	oledText[0] = "Add Loco";
	oledText[5] = "addr+# Add  * Cancel  # Roster";
	renderArrayInternal(false,false,true,false);
}

void OledRenderer::renderFoundSsids(const String &soFar) {
	menuIsShowing = true;
	if (soFar == "") {
	clearArray();
		for (int i=0; i<5 && i<foundSsidsCount; i++) {
			int globalIndex = (uiState.page*5)+i;
			if (globalIndex < foundSsidsCount && foundSsids[globalIndex].length()>0) {
				String ssid = foundSsids[globalIndex];
				// int rssi = foundSsidRssis[globalIndex]; // not needed here; glyph resolved during draw
				// Truncate SSID to keep line width stable
				if (ssid.length()>13) ssid = ssid.substring(0,13);
				// Store SSID line; glyph will be drawn inline later (we reserve last char position)
				oledText[i] = String(i) + ": " + ssid + " ";
				// Append placeholder marker we can detect when rendering to draw glyph instead of text character
				// Using '\x01' sentinel (non-printable) to signal glyph draw.
				oledText[i] += '\x01';
				// Overwrite sentinel mapping in uiState invert array if needed (not required now)
				// Store glyph code in a lightweight side channel: reuse foundSsidsOpen boolean array unused during display for that entry.
				// But simpler: keep a parallel array of glyphs; we will just re-derive on draw to avoid extra state.
			}
		}
		int totalPages = (foundSsidsCount + 4) / 5; // ceil
		String baseMenu = menu_text[menu_select_ssids_from_found];
		String pageIndicator = " p" + String(uiState.page+1) + "/" + String(totalPages);
		int maxChars = 24; // conservative width for 128px with small font
		if (baseMenu.length() > (maxChars - pageIndicator.length())) {
			baseMenu = baseMenu.substring(0, maxChars - pageIndicator.length());
		}
		oledText[5] = baseMenu + pageIndicator;
		renderArrayInternal(false,false,true,false);
	}
}

void OledRenderer::renderRoster(const String &soFar) {
	lastOledScreen = last_oled_screen_roster;
	lastOledStringParameter = soFar;
	menuIsShowing = true;
	if (soFar == "") {
	clearArray();
		for (int i=0; i<5 && ((uiState.page*5)+i<rosterSize); i++) {
			int index = rosterSortedIndex[(uiState.page*5)+i];
			if (rosterAddress[index] != 0) {
				oledText[i] = String(i) + ": " + rosterName[index] + " (" + rosterAddress[index] + ")" ;
			}
		}
		oledText[5] = "(" + String(uiState.page+1) +  ") " + menu_text[menu_roster];
	renderArrayInternal(false,false,true,false);
	}
}

void OledRenderer::renderTurnoutList(const String &soFar, TurnoutAction action) {
	lastOledScreen = last_oled_screen_turnout_list;
	lastOledStringParameter = soFar;
	menuIsShowing = true;
	if (soFar == "") {
	clearArray();
		int j = 0;
		for (int i=0; i<10 && i<turnoutListSize; i++) {
			j = (i<5) ? i : i+1;
			if (turnoutListUserName[(uiState.page*10)+i].length()>0) {
				oledText[j] = String(turnoutListIndex[i]) + ": " + turnoutListUserName[(uiState.page*10)+i].substring(0,10);
			}
		}
		oledText[5] = "(" + String(uiState.page+1) +  ") " + menu_text[menu_turnout_list];
	renderArrayInternal(false,false,true,false);
	}
}

void OledRenderer::renderRouteList(const String &soFar) {
	lastOledScreen = last_oled_screen_route_list;
	lastOledStringParameter = soFar;
	menuIsShowing = true;
	if (soFar == "") {
	clearArray();
		int j = 0;
		for (int i=0; i<10 && i<routeListSize; i++) {
			j = (i<5) ? i : i+1;
			if (routeListUserName[(uiState.page*10)+i].length()>0) {
				oledText[j] = String(routeListIndex[i]) + ": " + routeListUserName[(uiState.page*10)+i].substring(0,10);
			}
		}
		oledText[5] =  "(" + String(uiState.page+1) +  ") " + menu_text[menu_route_list];
	renderArrayInternal(false,false,true,false);
	}
}

void OledRenderer::renderFunctionList(const String &soFar) {
	lastOledScreen = last_oled_screen_function_list;
	lastOledStringParameter = soFar;
	menuIsShowing = true;
	uiState.functionHasBeenSelected = false;
	if (soFar == "") {
		clearArray();
		char currentChar = throttleManager.getCurrentThrottleChar();
		int currentIdx = throttleManager.getCurrentThrottleIndex();
		if (wiThrottleProtocol.getNumberOfLocomotives(currentChar) > 0 ) {
			int j = 0; int k = 0;
			for (int i=0; i<10; i++) {
				k = (uiState.functionPage*10) + i;
				if (k < MAX_FUNCTIONS) {
					j = (i<5) ? i : i+1;
					oledText[j] = String(i) + ": " + ((k<10) ? functionLabels[currentIdx][k].substring(0,10) : String(k) + "-" + functionLabels[currentIdx][k].substring(0,7));
					if (functionStates[currentIdx][k]) {
						oledTextInvert[j] = true;
					}
				}
			}
			oledText[5] = "(" + String(uiState.functionPage) +  ") " + menu_text[menu_function_list];
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

// Helper: Check if we need to show S/L suffixes (only if there are duplicate addresses with different types)
bool OledRenderer::checkNeedSuffixes(char throttleChar, int numLocos) {
	if (numLocos <= 1) return false;
	
	for (int idx1 = 0; idx1 < numLocos; idx1++) {
		String loco1 = wiThrottleProtocol.getLocomotiveAtPosition(throttleChar, idx1);
		if (loco1.length() > 1 && (loco1.charAt(0) == 'S' || loco1.charAt(0) == 'L')) {
			String addr1 = loco1.substring(1);
			for (int idx2 = idx1 + 1; idx2 < numLocos; idx2++) {
				String loco2 = wiThrottleProtocol.getLocomotiveAtPosition(throttleChar, idx2);
				if (loco2.length() > 1 && (loco2.charAt(0) == 'S' || loco2.charAt(0) == 'L')) {
					String addr2 = loco2.substring(1);
					if (addr1.equals(addr2) && loco1.charAt(0) != loco2.charAt(0)) {
						return true; // Found duplicate addresses with different types
					}
				}
			}
		}
	}
	return false;
}

// Helper: Format locomotive display string (strip S/L prefix, optionally add suffix)
String OledRenderer::formatLocoDisplay(const String &loco, bool needSuffixes) {
	String displayLoco = loco;
	if (displayLoco.length() > 0 && (displayLoco.charAt(0) == 'S' || displayLoco.charAt(0) == 'L')) {
		char prefix = displayLoco.charAt(0);
		displayLoco = displayLoco.substring(1);
		if (needSuffixes) {
			displayLoco += " (" + String(prefix) + ")";
		}
	}
	return displayLoco;
}

void OledRenderer::renderEditConsist() {
	lastOledScreen = last_oled_screen_edit_consist; 
	menuIsShowing = false; 
	clearArray(); 
	renderAllLocos(false); // Show ALL locos including lead loco (was true, hiding lead)
	oledText[0] = "Edit Consist Facing"; 
	oledText[5] = "no Chng Facing   * Close"; 
	renderArrayInternal(false,false,false,false);
	u8g2.sendBuffer();
}

void OledRenderer::renderDropLocoList() {
	menuIsShowing = true;
	clearArray();
	
	char currentChar = throttleManager.getCurrentThrottleChar();
	int numLocos = wiThrottleProtocol.getNumberOfLocomotives(currentChar);
	
	// Check if we need to show S/L suffixes
	bool needSuffixes = checkNeedSuffixes(currentChar, numLocos);
	
	// Render the locos
	if (numLocos > 0) {
		// Display locos with 1-based numbering for user-friendliness
		for (int index = 0; index < numLocos && index < 9; index++) {
			String loco = wiThrottleProtocol.getLocomotiveAtPosition(currentChar, index);
			int displayNum = index + 1;  // Convert to 1-based
			int linePos = (index < 4) ? index : index + 2;  // Skip line 4 (instructions)
			
			String displayLoco = formatLocoDisplay(loco, needSuffixes);
			
			oledText[linePos + 1] = String(displayNum) + ": " + displayLoco;
			if (wiThrottleProtocol.getDirection(currentChar, loco) == Reverse) {
				oledTextInvert[linePos + 1] = true;
			}
		}
	}
	
	oledText[0] = "Drop Loco";
	oledText[5] = "1-9 Select 0 All * Cancel";
	renderArrayInternal(false, false, true, false);
}


void OledRenderer::renderHeartbeatCheck() {
	menuIsShowing = false; RenderModel model; buildHeartbeatRenderModel(model, uiState, heartbeatMonitor.enabled()); for (int i=0;i<18;i++){ oledText[i] = model.lines[i]; oledTextInvert[i] = model.invert[i]; } renderArrayInternal(false,false,model.sendBuffer,model.drawTopLine);
}

void OledRenderer::renderAllLocos(bool hideLeadLoco) {
	lastOledScreen = last_oled_screen_all_locos; 
	lastOledBoolParameter = hideLeadLoco; 
	int startAt = (hideLeadLoco) ? 1 : 0; 
	String loco; 
	int j=0; 
	int i=0; 
	char currentChar = throttleManager.getCurrentThrottleChar(); 
	int numLocos = wiThrottleProtocol.getNumberOfLocomotives(currentChar);
	
	// Check if we need to show S/L suffixes
	bool needSuffixes = checkNeedSuffixes(currentChar, numLocos);
	
	// Render the locos
	if (numLocos > 0) { 
		for (int index=0; ((index < numLocos) && (i < 8)); index++) { 
			j = (i<4) ? i : i+2; 
			loco = wiThrottleProtocol.getLocomotiveAtPosition(currentChar, index); 
			
			if (i>=startAt) {
				String displayLoco = formatLocoDisplay(loco, needSuffixes);
				
				oledText[j+1] = String(i) + ": " + displayLoco; 
				if (wiThrottleProtocol.getDirection(currentChar, loco) == Reverse) 
					oledTextInvert[j+1] = true; 
			} 
			i++; 
		} 
	}
}
// Internal unified array renderer
void OledRenderer::renderArrayInternal(bool isThreeColumns, bool isPassword, bool sendBuffer, bool drawTopLine) {
	display.clearBuffer();
	display.setDrawColor(1);
	display.setFont(FONT_DEFAULT);
	int x = 0;
	int y = 10;
	int xInc = 64;
	int max = 12;
	if (isThreeColumns) { xInc = 42; max = 18; }
	for (int i=0; i < max; i++) {
		String lineStr = oledText[i];
		int sentinelPos = lineStr.indexOf('\x01');
		bool hasWifiGlyph = (sentinelPos >= 0);
		String textPart = hasWifiGlyph ? lineStr.substring(0, sentinelPos) : lineStr;
		const char *cLine = textPart.c_str();
		if (isPassword && i==2) display.setFont(FONT_PASSWORD);
		if (oledTextInvert[i]) {
			display.setDrawColor(1);
			display.drawBox(x, y-8, 62, 10);
			display.setDrawColor(0);
		}
		// Draw base text
		display.drawUTF8(x, y, cLine);
		// Draw signal bars if sentinel present
		if (hasWifiGlyph) {
			// Determine signal strength from page-adjusted RSSI global index
			int globalIndex = (uiState.page * 5) + i;
			int rssi = (globalIndex < foundSsidsCount) ? foundSsidRssis[globalIndex] : -100;
			int strength; // 0=weak, 1=low, 2=medium, 3=strong
			if (rssi >= -50) strength = 3;
			else if (rssi >= -65) strength = 2;
			else if (rssi >= -80) strength = 1;
			else strength = 0;
			
			int16_t textWidth = display.getUTF8Width(cLine);
			int barsX = x + textWidth + 2; // gap after text
			
			// Ensure proper draw color (1=black on white, or white on inverted background)
			display.setDrawColor(1);
			
			// Draw Wi-Fi signal bars (4 bars, each 2px wide with 1px gap)
			for (int bar = 0; bar < 4; bar++) {
				int barX = barsX + (bar * 3); // 2px bar + 1px gap
				int barHeight = (bar + 1) * 2; // heights: 2, 4, 6, 8 pixels
				int barY = y - barHeight + 1; // align bottom to baseline
				if (bar <= strength) {
					// Filled bar for active strength
					display.drawBox(barX, barY, 2, barHeight);
				}
				// Don't draw inactive bars - leave them blank for clarity on small monochrome displays
			}
		}
		display.setDrawColor(1);
		if (isPassword && i==2) display.setFont(FONT_DEFAULT);
		y += 10;
		if ((i==5) || (i==11)) { x += xInc; y = 10; }
	}
	// Draw separator/top line if requested
	if (drawTopLine) { display.drawHLine(0,11,128); renderBattery(); }
	// Bottom menu/status bar: fixed y baseline at 51
	display.setDrawColor(1);
	display.drawHLine(0,51,128);
	if (sendBuffer) display.sendBuffer();
}

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

