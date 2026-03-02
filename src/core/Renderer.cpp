// Renderer.cpp - display-agnostic renderer (renamed from OledRenderer.cpp)
#include "Renderer.h"
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

Renderer::Renderer(DisplayDriver &d, const DisplayLayout &l, const FontSet &f)
    : display(d), layout(l), fonts(f) {}

void Renderer::renderArray(bool isThreeColumns, bool isPassword, bool sendBuffer, bool drawTopLine) {
	renderArrayInternal(isThreeColumns, isPassword, sendBuffer, drawTopLine);
}

// New wrapper for menu rendering (replaces previous inline logic from sketch)
void Renderer::renderNewMenu(MenuSystem& menuSys) {
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
		oledText[layout.menuTextRow] = currentItem->instructions;
	} else {
		// Display menu items contiguously, respecting layout.
		// Items fill rows 0..n, jumping over menuTextRow (footer) into the
		// second column when necessary (128×64 has 6-row columns, 320×240
		// has 12 — so on the larger display everything stays in one column).
		int row = 0;
		for (uint8_t i = 0; i < menuSize && i < 10; i++) {
			String itemNum = (i == 9) ? "0" : String(i + 1);
			oledText[row] = itemNum + ": " + currentMenu[i].title;
			row++;
			// If we hit the footer row, jump to the second column
			if (row == layout.menuTextRow) {
				row = layout.secondColumnStartRow;
			}
		}
		oledText[layout.menuTextRow] = "* Cancel  # Select";
	}
	
	renderArrayInternal(false,false,true,false);
}

// Public wrapper that records state then calls private renderAllLocos
void Renderer::renderAllLocosScreen(bool hideLeadLoco) {
	clearArray();
	renderAllLocos(hideLeadLoco);
	// Provide a header/menu line for consist editing or loco overview
	oledText[0] = "Add Loco";
	oledText[layout.menuTextRow] = "addr+# Add  * Cancel  # Roster";
	renderArrayInternal(false,false,true,false);
}

// ── Model-based paginated list renderer (domain-free) ──────────────────────
void Renderer::renderPagedList(const PagedListModel &model) {
	clearArray();
	// Use page capacity for layout; fall back to itemCount when capacity not set
	int capacity = (model.pageCapacity > 0) ? model.pageCapacity : model.itemCount;
	int halfPage = capacity / 2;
	// halfPageSplit only makes sense when items overflow into col 2 (OLED).
	// On TFT all items fit in column 0, so disable the split to avoid a
	// mid-screen gap and place the footer at the canonical bottom row.
	bool useSplit = model.halfPageSplit && (layout.secondColumnStartRow <= capacity);
	// Optional header in row 0 — shifts item rows down by 1
	int rowOffset = (model.headerText.length() > 0) ? 1 : 0;
	if (rowOffset) oledText[0] = model.headerText;
	for (int i = 0; i < model.itemCount; i++) {
		const auto &item = model.items[i];
		if (item.label.length() > 0) {
			int row = useSplit ? ((i < halfPage) ? i : i + 1) : i;
			row += rowOffset;
			oledText[row] = String((i + 1) % 10) + ": " + item.label;
			if (item.invert) oledTextInvert[row] = true;
		}
	}
	// Place footer at the split gap when using split, else at the canonical menu row
	int footerRow = useSplit ? (halfPage + rowOffset) : layout.menuTextRow;
	oledText[footerRow] = model.footerText;
	renderArrayInternal(false, false, true, false, false, !useSplit);
}

// ── ListSelectionScreen-based renderer ─────────────────────────────────
void Renderer::renderListSelection(ListSelectionScreen &screen) {
	if (screen.onBeforeRender) screen.onBeforeRender();
	clearArray();

	int capacity = screen.visibleRows;
	int halfPage = capacity / 2;
	// halfPageSplit only makes sense when items overflow into col 2 (OLED).
	// On TFT all items fit in column 0, so disable the split.
	bool useSplit = screen.halfPageSplit && (layout.secondColumnStartRow <= capacity);

	// Optional header in row 0 — shifts item rows down by 1
	int rowOffset = (screen.headerText.length() > 0) ? 1 : 0;
	if (rowOffset) oledText[0] = screen.headerText;

	for (int i = 0; i < capacity; i++) {
		int gi = screen.globalIndex(i);
		if (gi >= screen.totalItems) break;
		bool invert = false;
		String label = screen.itemLabel(gi, invert);
		if (label.length() > 0) {
			int row = useSplit ? ((i < halfPage) ? i : i + 1) : i;
			row += rowOffset;
			oledText[row] = String((i + 1) % 10) + ": " + label;
			if (invert) oledTextInvert[row] = true;
		}
	}

	// Place footer at the split gap when using split, else at the canonical menu row
	int footerRow = useSplit ? (halfPage + rowOffset) : layout.menuTextRow;
	oledText[footerRow] = screen.footerText();
	renderArrayInternal(false, false, true, false, false, !useSplit);
}

// ── TitleScreen renderer ───────────────────────────────────────────────
// ── Shared helper: populate oledText[] from a TitleScreen ────────────────
// Returns the row index just past the last body line (useful for placing
// a spinner or other decoration below the body).
int Renderer::populateTitleArray(const TitleScreen &screen) {
	clearArray();

	// Row 0: title (always at top)
	oledText[0] = screen.title;

	// Subtitle goes at secondColumnStartRow on small displays (2-column),
	// or row 1 on large single-column displays where secondColumnStartRow
	// would be off-screen for a title layout.
	if (screen.subtitle.length() > 0) {
		int subRow = (layout.menuTextRow <= 5) ? layout.secondColumnStartRow : 1;
		oledText[subRow] = screen.subtitle;
	}

	// Vertically centre the body lines between row 2 and menuTextRow-1.
	// On 128×64 (menuTextRow=5): available rows 2-4 → 3 slots.
	// On 320×240 (menuTextRow=11): available rows 2-10 → 9 slots.
	int firstAvail = 2;
	int lastAvail  = layout.menuTextRow - 1;
	int available  = lastAvail - firstAvail + 1;
	int startRow   = firstAvail;
	if (screen.bodyCount < available) {
		startRow = firstAvail + (available - screen.bodyCount) / 2;
	}
	for (int i = 0; i < screen.bodyCount && (startRow + i) <= lastAvail; i++) {
		oledText[startRow + i] = screen.body[i];
	}

	// Footer at the canonical bottom row
	if (screen.footerText.length() > 0) {
		oledText[layout.menuTextRow] = screen.footerText;
	}

	return startRow + screen.bodyCount;
}

void Renderer::renderTitle(const TitleScreen &screen) {
	populateTitleArray(screen);
	renderArrayInternal(false, false, true, screen.showTopLine, true);
}

// ── Bouncing-dot spinner ────────────────────────────────────────────────
// Draws 5 small squares centred horizontally at `centerY`.  The "active"
// dot (and its immediate neighbours) are drawn larger, creating a wave
// that bounces left ↔ right across the row.
void Renderer::drawSpinner(int centerY, int frame) {
	const int dotCount = 5;
	int maxDot = layout.rowHeight / 3;          // 6 on TFT, 3 on OLED
	if (maxDot < 2) maxDot = 2;
	int minDot = (maxDot < 3) ? 1 : maxDot / 3;
	int midDot = (maxDot + minDot) / 2;
	int gap    = maxDot;                         // space between dot centres

	int cellStep   = maxDot + gap;
	int totalWidth = dotCount * maxDot + (dotCount - 1) * gap;
	int startX     = (layout.screenWidth - totalWidth) / 2;

	// Bounce position: 0→4→0→4→…
	int period = (dotCount - 1) * 2;
	int pos = frame % period;
	if (pos >= dotCount) pos = period - pos;

	display.setDrawColor(1);
	for (int i = 0; i < dotCount; i++) {
		int dist = abs(i - pos);
		int size = (dist == 0) ? maxDot : (dist == 1) ? midDot : minDot;
		int cellCenterX = startX + i * cellStep + maxDot / 2;
		int x = cellCenterX - size / 2;
		int y = centerY - size / 2;
		display.drawBox(x, y, size, size);
	}
}

void Renderer::renderWait(const WaitScreen &screen) {
	int nextRow = populateTitleArray(screen);

	// Render text without sending the buffer yet
	renderArrayInternal(false, false, false, screen.showTopLine, true);

	// Place the spinner one row below the last body line, centred vertically
	// in that row.  Clamp so it stays above the status-bar line.
	int spinnerY = (nextRow + 1) * layout.rowHeight;
	int maxY     = layout.statusBarY - layout.rowHeight / 2;
	if (spinnerY > maxY) spinnerY = maxY;

	drawSpinner(spinnerY, screen.frame);

	display.sendBuffer();
}

// ── Text input screen ───────────────────────────────────────────────────
// Draws: optional top-bar, 1-2 centred prompt lines, centred input with
// blinking caret, bottom status-bar, footer text.
void Renderer::renderTextInput(const TextInputScreen &screen) {
	display.clearBuffer();
	display.setDrawColor(1);

	// Top bar separator
	display.drawHLine(0, layout.topBarHeight, layout.screenWidth);
	renderBattery();

	// Bottom status bar separator
	display.drawHLine(0, layout.statusBarY, layout.screenWidth);

	// ── Vertical layout planning ──
	// Available vertical space: between topBarHeight and statusBarY.
	// We'll place prompt lines in the upper portion and the input line
	// in the middle, vertically centred in the available space.
	int contentTop    = layout.topBarHeight + 2;
	int contentBottom = layout.statusBarY;
	int rh            = layout.rowHeight;

	// Count content lines: prompt1 + prompt2 + gap + input = 3 or 4 rows
	int promptLines = 0;
	if (screen.promptLine1.length() > 0) promptLines++;
	if (screen.promptLine2.length() > 0) promptLines++;
	int totalLines = promptLines + 1;  // +1 for the input line
	// Add a gap row between prompt and input if there are prompt lines
	if (promptLines > 0) totalLines++;

	int totalHeight = totalLines * rh;
	int startY = contentTop + (contentBottom - contentTop - totalHeight) / 2 + rh;
	if (startY < contentTop + rh) startY = contentTop + rh;

	display.setFont(fonts.defaultFont);
	int row = 0;

	// ── Prompt lines (centred) ──
	if (screen.promptLine1.length() > 0) {
		const char *p1 = screen.promptLine1.c_str();
		int w = display.getUTF8Width(p1);
		int x = (layout.screenWidth - w) / 2;
		if (x < 0) x = 0;
		display.drawUTF8(x, startY + row * rh, p1);
		row++;
	}
	if (screen.promptLine2.length() > 0) {
		const char *p2 = screen.promptLine2.c_str();
		int w = display.getUTF8Width(p2);
		int x = (layout.screenWidth - w) / 2;
		if (x < 0) x = 0;
		display.drawUTF8(x, startY + row * rh, p2);
		row++;
	}

	// ── Gap between prompt and input ──
	if (promptLines > 0) row++;

	// ── Input line with cursor (centred) ──
	String dispText = screen.displayText();
	int hlPos = screen.highlightPos; // -1 = no highlight, >=0 = invert that char
	int inputY = startY + row * rh;
	if (inputY > layout.screenHeight - 2) inputY = layout.screenHeight - 2;

	if (hlPos >= 0 && hlPos < (int)dispText.length()) {
		// ── Inverted-cursor mode: draw prefix normally, highlight char inverted ──
		String prefix = dispText.substring(0, hlPos);
		String hlChar = dispText.substring(hlPos, hlPos + 1);
		String suffix = (hlPos + 1 < (int)dispText.length())
		                ? dispText.substring(hlPos + 1) : String();

		// Measure widths for centering
		int prefixW = display.getUTF8Width(prefix.c_str());
		int hlCharW = display.getUTF8Width(hlChar.c_str());
		int suffixW = suffix.length() > 0
		              ? display.getUTF8Width(suffix.c_str()) : 0;
		int totalW  = prefixW + hlCharW + suffixW;
		int inputX  = (layout.screenWidth - totalW) / 2;
		if (inputX < 0) inputX = 0;

		int fontAsc = display.getFontAscent();
		int pad     = 1; // horizontal padding around highlighted char

		// Draw prefix text normally
		if (prefix.length() > 0)
			display.drawUTF8(inputX, inputY, prefix.c_str());

		// Draw filled box behind the highlighted character
		int hlX = inputX + prefixW;
		display.setDrawColor(1);
		display.drawBox(hlX - pad, inputY - fontAsc, hlCharW + pad * 2, rh);

		// Draw the highlighted character in inverted colour
		display.setDrawColor(0);
		display.drawUTF8(hlX, inputY, hlChar.c_str());
		display.setDrawColor(1);

		// Draw any suffix text normally
		if (suffix.length() > 0)
			display.drawUTF8(hlX + hlCharW, inputY, suffix.c_str());

	} else {
		// ── Normal text + blinking cursor line ──
		const char *inputStr = dispText.c_str();
		int inputW = display.getUTF8Width(inputStr);
		int inputX = (layout.screenWidth - inputW) / 2;
		if (inputX < 0) inputX = 0;
		display.drawUTF8(inputX, inputY, inputStr);

		// Draw a thin blinking cursor line after the text
		bool caretVisible = ((screen.frame / 4) % 2) == 0;
		if (caretVisible) {
			int cursorX = inputX + inputW + 1;
			int fontAsc = display.getFontAscent();
			// Vertical line from baseline-ascent to baseline
			display.drawLine(cursorX, inputY - fontAsc, cursorX, inputY);
		}
	}

	// ── Footer ──
	if (screen.footerText.length() > 0) {
		const char *ft = screen.footerText.c_str();
		int fw = display.getUTF8Width(ft);
		int fx = (layout.screenWidth - fw) / 2;
		if (fx < 0) fx = 0;
		int fy = layout.menuTextRow * rh + rh;
		if (fy > layout.screenHeight - 2) fy = layout.screenHeight - 2;
		display.drawUTF8(fx, fy, ft);
	}

	display.sendBuffer();
}

void Renderer::renderFunctions() {
	lastOledScreen = last_oled_screen_speed;
	int currentIdx = throttleManager.getCurrentThrottleIndex();
	int startX = layout.functionIndicatorStartX;
	int y = layout.functionIndicatorY;
	int boxW = layout.functionIndicatorBoxW;
	int boxH = layout.functionIndicatorBoxH;
	int spacing = layout.functionIndicatorSpacing;
	for (int i=0; i < MAX_FUNCTIONS; i++) if (throttleManager.getFunctionState(currentIdx, i)) {
		display.drawRBox(i*spacing+startX, y+1, boxW, boxH, 2);
		display.setDrawColor(0);
		display.setFont(fonts.functionIndicators);
		display.drawUTF8(i*spacing+1+startX, y+6+1, String( (i<10) ? i : ((i<20) ? i-10 : i-20)).c_str());
		display.setDrawColor(1);
	}
}

// Helper: Check if we need to show S/L suffixes (only if there are duplicate addresses with different types)
bool Renderer::checkNeedSuffixes(char throttleChar, int numLocos) {
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
String Renderer::formatLocoDisplay(const String &loco, bool needSuffixes) {
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


void Renderer::renderHeartbeatCheck() {
	menuIsShowing = false;
	TitleScreen ts;
	ts.title = "Heartbeat";
	ts.addBody(heartbeatMonitor.enabled() ? MSG_HEARTBEAT_CHECK_ENABLED : MSG_HEARTBEAT_CHECK_DISABLED);
	ts.footerText = "* Close";
	ts.showTopLine = false;
	renderTitle(ts);
}

void Renderer::renderAllLocos(bool hideLeadLoco) {
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
				
				oledText[j+1] = String((i+1) % 10) + ": " + displayLoco; 
				if (wiThrottleProtocol.getDirection(currentChar, loco) == Reverse) 
					oledTextInvert[j+1] = true; 
			} 
			i++; 
		} 
	}
}
// Internal unified array renderer
void Renderer::renderArrayInternal(bool isThreeColumns, bool isPassword, bool sendBuffer, bool drawTopLine, bool centerText, bool fullWidthInvert) {
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
		// Check for wifi signal sentinel (values \x01-\x04 encode strength 0-3)
		int sentinelPos = -1;
		int wifiStrength = 0;
		for (int j = 0; j < (int)lineStr.length(); j++) {
			char c = lineStr.charAt(j);
			if (c >= '\x01' && c <= '\x04') {
				sentinelPos = j;
				wifiStrength = c - '\x01';
				break;
			}
		}
		bool hasWifiGlyph = (sentinelPos >= 0);
		String textPart = hasWifiGlyph ? lineStr.substring(0, sentinelPos) : lineStr;
		const char *cLine = textPart.c_str();
		if (isPassword && i==2) display.setFont(fonts.password);
		// Clamp y so descenders aren't clipped at the screen bottom
		int drawY = (y > layout.screenHeight - 2) ? layout.screenHeight - 2 : y;
		if (oledTextInvert[i]) {
			int boxYOff = layout.rowHeight - 3;  // align box top with font ascender
			int boxW = fullWidthInvert ? (layout.screenWidth - x) : (xInc - 2);
			display.setDrawColor(1);
			display.drawBox(x, drawY - boxYOff, boxW, layout.rowHeight);
			display.setDrawColor(0);
		}
		// Draw base text
		int drawX = x;
		if (centerText && !oledTextInvert[i] && textPart.length() > 0) {
			int textW = display.getUTF8Width(cLine);
			drawX = (layout.screenWidth - textW) / 2;
			if (drawX < 0) drawX = 0;
		}
		display.drawUTF8(drawX, drawY, cLine);
		// Draw signal bars if sentinel present
		if (hasWifiGlyph) {
			int16_t textWidth = display.getUTF8Width(cLine);
			int barsX = x + textWidth + 2;
			
			display.setDrawColor(1);
			
			int barW = layout.wifiBarWidth;
			int barGap = layout.wifiBarGap;
			for (int bar = 0; bar < 4; bar++) {
				int barX = barsX + (bar * (barW + barGap));
				int barHeight = (bar + 1) * (layout.wifiBarMaxHeight / 4);
				int barY = drawY - barHeight + 1;
				if (bar <= wifiStrength) {
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

void Renderer::clearArray() { uiState.clearLines(); }

void Renderer::renderDirectCommands() { lastOledScreen = last_oled_screen_direct_commands; oledDirectCommandsAreBeingDisplayed = true; clearArray(); oledText[0] = DIRECT_COMMAND_LIST; for (int i=0; i<4; i++) oledText[i+1] = directCommandText[i][0]; int j=0; for (int i=6; i<10; i++) { oledText[i+1] = directCommandText[j][1]; j++; } j=0; for (int i=12; i<16; i++) { oledText[i+1] = directCommandText[j][2]; j++; } renderArray(true,false); menuCommandStarted = false; }

void Renderer::renderBattery() {
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

void Renderer::renderSpeedStepMultiplier() { if (speedStep != throttleManager.getSpeedStep()) { display.setDrawColor(1); display.setFont(fonts.glyphs); display.drawGlyph(layout.speedStepGlyphX, layout.speedStepGlyphY, glyph_speed_step); display.setFont(fonts.defaultFont); display.drawStr(layout.speedStepTextX, layout.speedStepTextY, String(throttleManager.getSpeedStep()).c_str()); } }

void Renderer::renderMomentumIndicator() {
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

void Renderer::renderBrakeIndicator() {
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

void Renderer::renderSpeed() {
	// Delegate to the model-based rendering path.
	// buildThrottleScreen() populates a ThrottleScreen from global state;
	// renderThrottleScreen() draws it without touching any globals.
	ThrottleScreen screen;
	buildThrottleScreen(screen);
	renderThrottleScreen(screen);
}

// ── ThrottleScreen-based rendering ──────────────────────────────────────────
// Self-contained operating screen renderer driven entirely by the
// ThrottleScreen model.  No globals are read — all data comes from `screen`.

void Renderer::renderThrottleScreen(const ThrottleScreen &screen) {
	lastOledScreen = last_oled_screen_speed;
	menuIsShowing = false;
	clearArray();
	bool drawTop = false;
	String noLocoMsg1;
	String noLocoMsg2;

	if (screen.hasLoco()) {
		oledText[0] = "   " + screen.locoDisplayString;
		drawTop = true;
	} else {
		setAppnameForOled();
		noLocoMsg1 = MSG_THROTTLE_NUMBER + String(screen.throttleNumber);
		noLocoMsg2 = MSG_NO_LOCO_SELECTED;
		drawTop = true;
	}

	// Populate menu/key-label lines from the model
	for (int i = 0; i < ThrottleScreen::MAX_MENU_LINES; i++) {
		oledText[i] = screen.menuLines[i].length() > 0 ? screen.menuLines[i] : oledText[i];
		oledTextInvert[i] = screen.menuInvert[i];
	}

	renderArrayInternal(false, false, false, drawTop);

	// Centred "Throttle #N" / "No Loco Selected" when idle
	if (noLocoMsg1.length() > 0) {
		display.setFont(fonts.defaultFont);
		int contentTop    = layout.topBarHeight + 4;
		int contentBottom = layout.statusBarY - 4;
		int areaHeight    = contentBottom - contentTop;
		int lineSpacing   = layout.rowHeight + 4;
		int blockHeight   = lineSpacing;
		int startY        = contentTop + (areaHeight - blockHeight) / 2;

		int w1 = display.getUTF8Width(noLocoMsg1.c_str());
		display.drawUTF8((layout.screenWidth - w1) / 2, startY, noLocoMsg1.c_str());
		int w2 = display.getUTF8Width(noLocoMsg2.c_str());
		display.drawUTF8((layout.screenWidth - w2) / 2, startY + lineSpacing, noLocoMsg2.c_str());
	}

	// Function state indicators
	if (screen.hasLoco()) {
		int startX  = layout.functionIndicatorStartX;
		int y       = layout.functionIndicatorY;
		int boxW    = layout.functionIndicatorBoxW;
		int boxH    = layout.functionIndicatorBoxH;
		int spacing = layout.functionIndicatorSpacing;
		for (int i = 0; i < MAX_FUNCTIONS; i++) {
			if (screen.functionActive[i]) {
				display.drawRBox(i * spacing + startX, y + 1, boxW, boxH, 2);
				display.setDrawColor(0);
				display.setFont(fonts.functionIndicators);
				display.drawUTF8(i * spacing + 1 + startX, y + 6 + 1,
				                 String(i < 10 ? i : (i < 20 ? i - 10 : i - 20)).c_str());
				display.setDrawColor(1);
			}
		}

		// Throttle number badge
		display.setDrawColor(0);
		display.drawBox(0, 0, layout.throttleNumberBoxW, layout.throttleNumberBoxH);
		display.setDrawColor(1);
		display.setFont(fonts.throttleNumber);
		int tnY = (layout.throttleNumberBoxH + display.getFontAscent()) / 2;
		display.drawStr(layout.throttleNumberX, tnY, String(screen.throttleNumber).c_str());
	}

	// Battery (already abstracted via batteryMonitor)
	renderBattery();

	// Speed step multiplier indicator (shown when different from compile-time default)
	if (screen.currentSpeedStep != speedStep) {
		display.setDrawColor(1);
		display.setFont(fonts.glyphs);
		display.drawGlyph(layout.speedStepGlyphX, layout.speedStepGlyphY, glyph_speed_step);
		display.setFont(fonts.defaultFont);
		display.drawStr(layout.speedStepTextX, layout.speedStepTextY,
		                String(screen.currentSpeedStep).c_str());
	}

	// Track power indicator
	if (screen.trackPower == PowerOn) {
		display.drawRBox(layout.trackPowerBoxX, layout.trackPowerBoxY,
		                 layout.trackPowerBoxW, layout.trackPowerBoxH, 1);
		display.setDrawColor(0);
	}
	display.setFont(fonts.glyphs);
	display.drawGlyph(layout.trackPowerGlyphX, layout.trackPowerGlyphY, glyph_track_power);
	display.setDrawColor(1);

	// Heartbeat disabled indicator
	if (!screen.heartbeatEnabled) {
		display.setFont(fonts.glyphs);
		display.drawGlyph(layout.heartbeatGlyphX, layout.heartbeatGlyphY, glyph_heartbeat_off);
		display.setDrawColor(2);
		display.drawLine(layout.heartbeatStrikeX1, layout.heartbeatStrikeY1,
		                 layout.heartbeatStrikeX2, layout.heartbeatStrikeY2);
		display.setDrawColor(1);
	}

	// Momentum indicator (L/M/H)
	if (screen.momentumLevel > 0) {
		const char *labels[] = {"L", "M", "H"};
		int idx = constrain(screen.momentumLevel - 1, 0, 2);
		display.setFont(fonts.defaultFont);
		display.drawStr(layout.momentumTextX, layout.momentumTextY, labels[idx]);
	}

	// Brake / ramp indicator
	if (screen.brakeState == 1) {
		display.setFont(fonts.defaultFont);
		display.drawStr(layout.brakeTextX, layout.brakeTextY, "B");
	} else if (screen.brakeState == 2) {
		display.setFont(fonts.glyphs);
		display.drawGlyph(layout.brakeTextX, layout.brakeTextY, glyph_arrow_up);
	} else if (screen.brakeState == 3) {
		display.setFont(fonts.glyphs);
		display.drawGlyph(layout.brakeTextX, layout.brakeTextY, glyph_arrow_down);
	}

	// Direction and speed (large font)
	display.setFont(fonts.direction);
	display.drawStr(layout.directionX, layout.directionY, screen.directionDisplay.c_str());

	const char *cSpeed = screen.speedDisplay.c_str();
	display.setFont(fonts.speed);
	int width = display.getStrWidth(cSpeed);
	int speedAreaWidth = layout.directionX - layout.speedX;
	display.drawStr(layout.speedX + (speedAreaWidth - width), layout.speedY, cSpeed);

	// Next throttle preview
	if (screen.hasNextThrottle) {
		display.setFont(fonts.nextThrottle);
		display.drawStr(layout.nextThrottleNumberX, layout.nextThrottleNumberY,
		                screen.nextThrottleNumber.c_str());
		display.drawStr(layout.nextThrottleSpeedX, layout.nextThrottleSpeedY,
		                screen.nextThrottleSpeedDir.c_str());
	}

	display.sendBuffer();
}
