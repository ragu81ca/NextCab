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

OledRenderer::OledRenderer(DisplayDriver &d, const DisplayLayout &l, const FontSet &f)
    : display(d), layout(l), fonts(f) {}

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
		int itemsPerPage = layout.ssidItemsPerPage;
		int nameMax = layout.ssidNameMaxLength;
		for (int i=0; i<itemsPerPage && i<foundSsidsCount; i++) {
			int globalIndex = (uiState.page*itemsPerPage)+i;
			if (globalIndex < foundSsidsCount && foundSsids[globalIndex].length()>0) {
				String ssid = foundSsids[globalIndex];
				if (nameMax > 0 && (int)ssid.length()>nameMax) ssid = ssid.substring(0,nameMax);
				oledText[i] = String(i+1) + ": " + ssid + " ";
				oledText[i] += '\x01';
			}
		}
		int totalPages = (foundSsidsCount + itemsPerPage - 1) / itemsPerPage;
		String baseMenu = menu_text[menu_select_ssids_from_found];
		String pageIndicator = " p" + String(uiState.page+1) + "/" + String(totalPages);
		int maxChars = layout.maxCharsPerRow;
		if ((int)baseMenu.length() > (maxChars - (int)pageIndicator.length())) {
			baseMenu = baseMenu.substring(0, maxChars - pageIndicator.length());
		}
		oledText[itemsPerPage] = baseMenu + pageIndicator;
		renderArrayInternal(false,false,true,false);
	}
}

void OledRenderer::renderRoster(const String &soFar) {
	lastOledScreen = last_oled_screen_roster;
	lastOledStringParameter = soFar;
	menuIsShowing = true;
	if (soFar == "") {
	clearArray();
		int itemsPerPage = layout.rosterItemsPerPage;
		int nameMax = layout.rosterNameMaxLength;
		for (int i=0; i<itemsPerPage && ((uiState.page*itemsPerPage)+i<rosterSize); i++) {
			int index = rosterSortedIndex[(uiState.page*itemsPerPage)+i];
			if (rosterAddress[index] != 0) {
				String name = rosterName[index];
				if (nameMax > 0 && (int)name.length() > nameMax) name = name.substring(0, nameMax);
				oledText[i] = String(i) + ": " + name + " (" + rosterAddress[index] + ")" ;
			}
		}
		oledText[itemsPerPage] = "(" + String(uiState.page+1) +  ") " + menu_text[menu_roster];
	renderArrayInternal(false,false,true,false);
	}
}

void OledRenderer::renderTurnoutList(const String &soFar, TurnoutAction action) {
	lastOledScreen = last_oled_screen_turnout_list;
	lastOledStringParameter = soFar;
	menuIsShowing = true;
	if (soFar == "") {
	clearArray();
		int itemsPerPage = layout.turnoutItemsPerPage;
		int nameMax = layout.turnoutNameMaxLength;
		int halfPage = itemsPerPage / 2;
		int j = 0;
		for (int i=0; i<itemsPerPage && i<turnoutListSize; i++) {
			j = (i<halfPage) ? i : i+1;
			if (turnoutListUserName[(uiState.page*itemsPerPage)+i].length()>0) {
				String name = turnoutListUserName[(uiState.page*itemsPerPage)+i];
				if (nameMax > 0 && (int)name.length() > nameMax) name = name.substring(0, nameMax);
				oledText[j] = String(turnoutListIndex[i]) + ": " + name;
			}
		}
		oledText[halfPage] = "(" + String(uiState.page+1) +  ") " + menu_text[menu_turnout_list];
	renderArrayInternal(false,false,true,false);
	}
}

void OledRenderer::renderRouteList(const String &soFar) {
	lastOledScreen = last_oled_screen_route_list;
	lastOledStringParameter = soFar;
	menuIsShowing = true;
	if (soFar == "") {
	clearArray();
		int itemsPerPage = layout.routeItemsPerPage;
		int nameMax = layout.routeNameMaxLength;
		int halfPage = itemsPerPage / 2;
		int j = 0;
		for (int i=0; i<itemsPerPage && i<routeListSize; i++) {
			j = (i<halfPage) ? i : i+1;
			if (routeListUserName[(uiState.page*itemsPerPage)+i].length()>0) {
				String name = routeListUserName[(uiState.page*itemsPerPage)+i];
				if (nameMax > 0 && (int)name.length() > nameMax) name = name.substring(0, nameMax);
				oledText[j] = String(routeListIndex[i]) + ": " + name;
			}
		}
		oledText[halfPage] =  "(" + String(uiState.page+1) +  ") " + menu_text[menu_route_list];
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
			int itemsPerPage = layout.functionItemsPerPage;
			int labelMax = layout.functionLabelMaxLength;
			int halfPage = itemsPerPage / 2;
			int j = 0; int k = 0;
			for (int i=0; i<itemsPerPage; i++) {
				k = (uiState.functionPage*itemsPerPage) + i;
				if (k < MAX_FUNCTIONS) {
					j = (i<halfPage) ? i : i+1;
					String label = functionLabels[currentIdx][k];
					if (labelMax > 0 && (int)label.length() > labelMax) label = label.substring(0, labelMax);
					oledText[j] = String(i) + ": " + ((k<10) ? label : String(k) + "-" + label);
					if (functionStates[currentIdx][k]) {
						oledTextInvert[j] = true;
					}
				}
			}
			oledText[halfPage] = "(" + String(uiState.functionPage) +  ") " + menu_text[menu_function_list];
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
	int startX = layout.functionIndicatorStartX;
	int y = layout.functionIndicatorY;
	int boxW = layout.functionIndicatorBoxW;
	int boxH = layout.functionIndicatorBoxH;
	int spacing = layout.functionIndicatorSpacing;
	for (int i=0; i < MAX_FUNCTIONS; i++) if (functionStates[currentIdx][i]) {
		display.drawRBox(i*spacing+startX, y+1, boxW, boxH, 2);
		display.setDrawColor(0);
		display.setFont(fonts.functionIndicators);
		display.drawUTF8(i*spacing+1+startX, y+6+1, String( (i<10) ? i : ((i<20) ? i-10 : i-20)).c_str());
		display.setDrawColor(1);
	}
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
	renderAllLocos(false);
	oledText[0] = "Edit Consist Facing"; 
	oledText[5] = "no Chng Facing   * Close"; 
	renderArrayInternal(false,false,false,false);
	display.sendBuffer();
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
		int maxLocos = layout.maxLocosDisplayed;
		int halfPage = maxLocos / 2;
		for (int index = 0; index < numLocos && index < maxLocos; index++) {
			String loco = wiThrottleProtocol.getLocomotiveAtPosition(currentChar, index);
			int displayNum = index + 1;
			int linePos = (index < halfPage) ? index : index + 2;
			
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
	menuIsShowing = false;
	RenderModel model;
	buildHeartbeatRenderModel(model, uiState, heartbeatMonitor.enabled());
	int max = layout.twoColumnMax;
	for (int i=0; i<max; i++) {
		oledText[i] = model.lines[i];
		oledTextInvert[i] = model.invert[i];
	}
	renderArrayInternal(false, false, model.sendBuffer, model.drawTopLine);
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
	int maxLocos = layout.maxLocosDisplayed;
	int halfPage = maxLocos / 2;
	
	// Check if we need to show S/L suffixes
	bool needSuffixes = checkNeedSuffixes(currentChar, numLocos);
	
	// Render the locos
	if (numLocos > 0) { 
		for (int index=0; ((index < numLocos) && (i < maxLocos)); index++) { 
			j = (i<halfPage) ? i : i+2; 
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
	display.setFont(fonts.defaultFont);
	int x = 0;
	int y = layout.rowHeight;
	int xInc = layout.columnWidth;
	int max = layout.twoColumnMax;
	if (isThreeColumns) { xInc = layout.threeColumnWidth; max = layout.threeColumnMax; }
	int rowsPerColumn = max / (isThreeColumns ? 3 : 2);
	for (int i=0; i < max; i++) {
		String lineStr = oledText[i];
		int sentinelPos = lineStr.indexOf('\x01');
		bool hasWifiGlyph = (sentinelPos >= 0);
		String textPart = hasWifiGlyph ? lineStr.substring(0, sentinelPos) : lineStr;
		const char *cLine = textPart.c_str();
		if (isPassword && i==2) display.setFont(fonts.password);
		if (oledTextInvert[i]) {
			display.setDrawColor(1);
			display.drawBox(x, y-8, xInc-2, layout.rowHeight);
			display.setDrawColor(0);
		}
		// Draw base text
		display.drawUTF8(x, y, cLine);
		// Draw signal bars if sentinel present
		if (hasWifiGlyph) {
			int itemsPerPage = layout.ssidItemsPerPage;
			int globalIndex = (uiState.page * itemsPerPage) + i;
			int rssi = (globalIndex < foundSsidsCount) ? foundSsidRssis[globalIndex] : -100;
			int strength;
			if (rssi >= -50) strength = 3;
			else if (rssi >= -65) strength = 2;
			else if (rssi >= -80) strength = 1;
			else strength = 0;
			
			int16_t textWidth = display.getUTF8Width(cLine);
			int barsX = x + textWidth + 2;
			
			display.setDrawColor(1);
			
			int barW = layout.wifiBarWidth;
			int barGap = layout.wifiBarGap;
			for (int bar = 0; bar < 4; bar++) {
				int barX = barsX + (bar * (barW + barGap));
				int barHeight = (bar + 1) * (layout.wifiBarMaxHeight / 4);
				int barY = y - barHeight + 1;
				if (bar <= strength) {
					display.drawBox(barX, barY, barW, barHeight);
				}
			}
		}
		display.setDrawColor(1);
		if (isPassword && i==2) display.setFont(fonts.defaultFont);
		y += layout.rowHeight;
		if (((i+1) % rowsPerColumn) == 0) { x += xInc; y = layout.rowHeight; }
	}
	// Draw separator/top line if requested
	if (drawTopLine) { display.drawHLine(0, layout.topBarHeight, layout.screenWidth); renderBattery(); }
	// Bottom menu/status bar separator
	display.setDrawColor(1);
	display.drawHLine(0, layout.statusBarY, layout.screenWidth);
	if (sendBuffer) display.sendBuffer();
}

void OledRenderer::clearArray() { uiState.clearLines(); }

void OledRenderer::renderDirectCommands() { lastOledScreen = last_oled_screen_direct_commands; oledDirectCommandsAreBeingDisplayed = true; clearArray(); oledText[0] = DIRECT_COMMAND_LIST; for (int i=0; i<4; i++) oledText[i+1] = directCommandText[i][0]; int j=0; for (int i=6; i<10; i++) { oledText[i+1] = directCommandText[j][1]; j++; } j=0; for (int i=12; i<16; i++) { oledText[i+1] = directCommandText[j][2]; j++; } renderArray(true,false); menuCommandStarted = false; }

void OledRenderer::renderBattery() {
	if (!(batteryMonitor.enabled() && (batteryMonitor.displayMode()!=NONE) && (batteryMonitor.lastCheckMillis()>0))) return;
	const int screenWidth = layout.screenWidth;
	const int rightMargin = layout.rightMargin;
	int pctRaw = batteryMonitor.percent();
	bool showPct = (batteryMonitor.displayMode()==ICON_AND_PERCENT);

	// Measure components
	display.setFont(fonts.glyphs);
	const char *icon = "Z";
	int iconWidth = display.getStrWidth(icon);
	String pctStr;
	int pctWidth = 0;
	if (showPct) {
		display.setFont(fonts.functionIndicators);
		pctStr = (pctRaw < 5) ? "LOW" : (String(pctRaw) + "%");
		pctWidth = display.getStrWidth(pctStr.c_str());
		display.setFont(fonts.glyphs);
	}
	int spacing = showPct ? 2 : 0;
	int totalWidth = iconWidth + spacing + pctWidth;
	if (totalWidth > (screenWidth - rightMargin)) {
		showPct = false;
		totalWidth = iconWidth;
	}
	int leftX = screenWidth - rightMargin - totalWidth;
	if (leftX < layout.batteryMinX) leftX = layout.batteryMinX;
	int yIcon = layout.topBarHeight;

	// Draw icon
	display.setDrawColor(1);
	display.setFont(fonts.glyphs);
	display.drawStr(leftX, yIcon, icon);
	// Battery fill bars (keep within icon region)
	if (pctRaw>10) display.drawLine(leftX+1,yIcon-6,leftX+1,yIcon-3);
	if (pctRaw>25) display.drawLine(leftX+2,yIcon-6,leftX+2,yIcon-3);
	if (pctRaw>50) display.drawLine(leftX+3,yIcon-6,leftX+3,yIcon-3);
	if (pctRaw>75) display.drawLine(leftX+4,yIcon-6,leftX+4,yIcon-3);
	if (pctRaw>90) display.drawLine(leftX+5,yIcon-6,leftX+5,yIcon-3);

	// Draw percent if enabled and still fits
	if (showPct) {
		display.setFont(fonts.functionIndicators);
		int pctX = leftX + iconWidth + spacing;
		int pctY = yIcon - 1;
		int maxRight = screenWidth - rightMargin - 1;
		if (pctX + pctWidth - 1 > maxRight) pctX = maxRight - pctWidth + 1;
		if (pctX < leftX + iconWidth + spacing) pctX = leftX + iconWidth + spacing;
		display.drawStr(pctX, pctY, pctStr.c_str());
	}
}

void OledRenderer::renderSpeedStepMultiplier() { if (speedStep != throttleManager.getSpeedStep()) { display.setDrawColor(1); display.setFont(fonts.glyphs); display.drawGlyph(layout.speedStepGlyphX, layout.speedStepGlyphY, glyph_speed_step); display.setFont(fonts.defaultFont); display.drawStr(layout.speedStepTextX, layout.speedStepTextY, String(throttleManager.getSpeedStep()).c_str()); } }

void OledRenderer::renderMomentumIndicator() {
    int currentIdx = throttleManager.getCurrentThrottleIndex();
    if (!throttleManager.momentum().isActive(currentIdx)) return;
    
    const char* levelChar = "";
    switch (throttleManager.momentum().getMomentumLevel(currentIdx)) {
        case MomentumLevel::Low:    levelChar = "L"; break;
        case MomentumLevel::Medium: levelChar = "M"; break;
        case MomentumLevel::High:   levelChar = "H"; break;
        default: return;
    }
    
    display.setDrawColor(1);
    display.setFont(fonts.defaultFont);
    display.drawStr(layout.momentumTextX, layout.momentumTextY, levelChar);
}

void OledRenderer::renderBrakeIndicator() {
    int currentIdx = throttleManager.getCurrentThrottleIndex();
    
    if (throttleManager.momentum().isBraking(currentIdx)) {
        display.setDrawColor(1);
        display.setFont(fonts.defaultFont);
        display.drawStr(layout.brakeTextX, layout.brakeTextY, "B");
        return;
    }
    
    int targetSpeed = throttleManager.momentum().getTargetSpeed(currentIdx);
    int actualSpeed = throttleManager.momentum().getActualSpeed(currentIdx);
    
    if (targetSpeed > actualSpeed + 1) {
        display.setDrawColor(1);
        display.setFont(fonts.glyphs);
        display.drawGlyph(layout.brakeTextX, layout.brakeTextY, glyph_arrow_up);
    } else if (targetSpeed + 1 < actualSpeed) {
        display.setDrawColor(1);
        display.setFont(fonts.glyphs);
        display.drawGlyph(layout.brakeTextX, layout.brakeTextY, glyph_arrow_down);
    }
}

void OledRenderer::renderSpeed() {
	lastOledScreen = last_oled_screen_speed;
	menuIsShowing = false;
	String sLocos = "";
	String sSpeed = "";
	String sDirection = "";
	String sep = " ";
	bool foundNext = false;
	String nextNo = "";
	String nextSpeedDir = "";
	clearArray();
	bool drawTop = false;
	int currentIdx = throttleManager.getCurrentThrottleIndex();
	char currentChar = throttleManager.getCurrentThrottleChar();

	if (wiThrottleProtocol.getNumberOfLocomotives(currentChar) > 0) {
		for (int i = 0; i < wiThrottleProtocol.getNumberOfLocomotives(currentChar); i++) {
			sLocos += sep + getDisplayLocoString(currentIdx, i);
			sep = CONSIST_SPACE_BETWEEN_LOCOS;
		}
		sSpeed = String(throttleManager.getDisplaySpeed(currentIdx));
		Direction dir = throttleManager.directions()[currentIdx];
		sDirection = (dir == Forward) ? DIRECTION_FORWARD_TEXT : DIRECTION_REVERSE_TEXT;

		if (throttleManager.getMaxThrottles() > 1) {
			int nextIdx = currentIdx + 1;
			for (int i = nextIdx; i < throttleManager.getMaxThrottles(); i++)
				if (wiThrottleProtocol.getNumberOfLocomotives(getMultiThrottleChar(i)) > 0) { foundNext = true; nextIdx = i; break; }
			if (!foundNext && currentIdx > 0) {
				for (int i = 0; i < currentIdx; i++)
					if (wiThrottleProtocol.getNumberOfLocomotives(getMultiThrottleChar(i)) > 0) { foundNext = true; nextIdx = i; break; }
			}
			if (foundNext) {
				nextNo = String(nextIdx + 1);
				int sp = throttleManager.getDisplaySpeed(nextIdx);
				nextSpeedDir = String(sp);
				Direction nextDir = throttleManager.directions()[nextIdx];
				if (nextDir == Forward) nextSpeedDir += DIRECTION_FORWARD_TEXT_SHORT;
				else nextSpeedDir = DIRECTION_REVERSE_TEXT_SHORT + nextSpeedDir;
				nextSpeedDir = String("     ") + nextSpeedDir;
				nextSpeedDir = nextSpeedDir.substring(nextSpeedDir.length() - 5);
			}
		}
		oledText[0] = "   " + sLocos;
		drawTop = true;
	} else {
		setAppnameForOled();
		oledText[2] = MSG_THROTTLE_NUMBER + String(currentIdx + 1);
		oledText[3] = MSG_NO_LOCO_SELECTED;
		drawTop = true;
	}

	if (!hashShowsFunctionsInsteadOfKeyDefs) setMenuTextForOled(menu_menu);
	else setMenuTextForOled(menu_menu_hash_is_functions);

	renderArrayInternal(false, false, false, drawTop);

	if (wiThrottleProtocol.getNumberOfLocomotives(currentChar) > 0) {
		renderFunctions();
		display.setDrawColor(0);
		display.drawBox(0, 0, layout.throttleNumberBoxW, layout.throttleNumberBoxH);
		display.setDrawColor(1);
		display.setFont(fonts.throttleNumber);
		display.drawStr(layout.throttleNumberX, layout.throttleNumberY, String(currentIdx + 1).c_str());
	}

	renderBattery();
	renderSpeedStepMultiplier();

	// Track power indicator
	if (trackPower == PowerOn) {
		display.drawRBox(layout.trackPowerBoxX, layout.trackPowerBoxY,
		                 layout.trackPowerBoxW, layout.trackPowerBoxH, 1);
		display.setDrawColor(0);
	}
	display.setFont(fonts.glyphs);
	display.drawGlyph(layout.trackPowerGlyphX, layout.trackPowerGlyphY, glyph_track_power);
	display.setDrawColor(1);

	// Heartbeat disabled indicator
	if (!heartbeatMonitor.enabled()) {
		display.setFont(fonts.glyphs);
		display.drawGlyph(layout.heartbeatGlyphX, layout.heartbeatGlyphY, glyph_heartbeat_off);
		display.setDrawColor(2);
		display.drawLine(layout.heartbeatStrikeX1, layout.heartbeatStrikeY1,
		                 layout.heartbeatStrikeX2, layout.heartbeatStrikeY2);
		display.setDrawColor(1);
	}

	renderMomentumIndicator();
	renderBrakeIndicator();

	// Direction and speed
	display.setFont(fonts.direction);
	display.drawStr(layout.directionX, layout.directionY, sDirection.c_str());

	const char *cSpeed = sSpeed.c_str();
	display.setFont(fonts.speed);
	int width = display.getStrWidth(cSpeed);
	// Center speed text in the speed area
	int speedAreaWidth = layout.directionX - layout.speedX;
	display.drawStr(layout.speedX + (speedAreaWidth - width), layout.speedY, cSpeed);

	// Next throttle info
	if (throttleManager.getMaxThrottles() > 1 && foundNext) {
		display.setFont(fonts.nextThrottle);
		display.drawStr(layout.nextThrottleNumberX, layout.nextThrottleNumberY, nextNo.c_str());
		display.drawStr(layout.nextThrottleSpeedX, layout.nextThrottleSpeedY, nextSpeedDir.c_str());
	}

	display.sendBuffer();
}

