// DisplayLayout.h - Display layout profile
// Defines both pixel positions AND content capacity for a given display size.
// Content capacity values (items per page, max name lengths, column counts) are what
// actually let a larger display SHOW MORE INFORMATION rather than just bigger pixels.
#pragma once

struct DisplayLayout {
    // ── Screen geometry ──
    int screenWidth;
    int screenHeight;

    // ── Content capacity (the key to actually using bigger displays) ──
    int maxTextRows;              // total text rows in the array (6 on small, 12+ on large)
    int maxCharsPerRow;           // characters per row at default font size
    int rosterItemsPerPage;       // roster entries per page
    int functionItemsPerPage;     // function items per page
    int turnoutItemsPerPage;      // turnout items per page
    int routeItemsPerPage;        // route items per page
    int ssidItemsPerPage;         // SSIDs per page
    int menuItemsPerPage;         // menu items per page
    int maxLocosDisplayed;        // max locos in consist list

    // ── Truncation limits ──
    int rosterNameMaxLength;      // max chars for roster names
    int ssidNameMaxLength;        // max chars for SSID names
    int turnoutNameMaxLength;     // max chars for turnout names
    int routeNameMaxLength;       // max chars for route names
    int functionLabelMaxLength;   // max chars for function labels
    int locoNameMaxLength;        // max chars for loco display names

    // ── Column layout (for renderArray 2-col / 3-col modes) ──
    int columnCount;              // 2 for 128×64, 3-4 for 320×240
    int columnWidth;              // pixels per column
    int threeColumnWidth;         // pixel width for 3-column mode
    int twoColumnMax;             // max items in 2-column mode
    int threeColumnMax;           // max items in 3-column mode

    // ── Row layout ──
    int rowHeight;                // pixels per text row
    int topBarHeight;             // y of separator below top bar (also baseline for row 0)
    int statusBarY;               // y of the bottom status/menu separator
    int rightMargin;              // pixels to keep clear at right edge

    // ── Logical row indices in the oledText[] array ──
    int menuTextRow;              // row index for bottom menu/status text (5 on 128×64, 11 on 320×240)
    int secondColumnStartRow;     // first row index of the second column (6 on 128×64, 12 on 320×240)

    // ── Speed screen zones ──
    int speedX;                   // x of speed text
    int speedY;                   // baseline y of speed text
    int directionX;               // x of direction text
    int directionY;               // baseline y of direction text
    int throttleNumberX;          // x of throttle number
    // throttleNumberY removed — now computed dynamically from box height + font ascent
    int throttleNumberBoxW;       // width of throttle number background box
    int throttleNumberBoxH;       // height of throttle number background box

    // ── Function indicators ──
    int functionIndicatorStartX;  // x start for function indicator boxes
    int functionIndicatorY;       // y of function indicator row
    int functionIndicatorBoxW;    // width per function indicator box
    int functionIndicatorBoxH;    // height per function indicator box
    int functionIndicatorSpacing; // horizontal spacing between indicators
    int functionIndicatorTextYOffset; // y offset from box top to text baseline

    // ── Track power / heartbeat / speed step / momentum / brake indicators ──
    int trackPowerBoxX;
    int trackPowerBoxY;
    int trackPowerBoxW;
    int trackPowerBoxH;
    int trackPowerGlyphX;
    int trackPowerGlyphY;
    int heartbeatGlyphX;
    int heartbeatGlyphY;
    int heartbeatStrikeX1;
    int heartbeatStrikeY1;
    int heartbeatStrikeX2;
    int heartbeatStrikeY2;
    int speedStepGlyphX;
    int speedStepGlyphY;
    int speedStepTextX;
    int speedStepTextY;

    // ── Momentum train indicator (loco + 0/1/2 cars) ──
    int momentumX;             // left edge of locomotive
    int momentumY;             // top edge of locomotive
    int momentumScale;         // pixel scale: 1 for OLED (8px loco), 2 for TFT (16px)

    // ── Next throttle info ──
    int nextThrottleNumberX;
    int nextThrottleNumberY;
    int nextThrottleSpeedX;
    int nextThrottleSpeedY;

    // ── Wi-Fi signal bars ──
    int wifiBarWidth;             // pixel width of each signal bar
    int wifiBarGap;               // gap between bars
    int wifiBarMaxHeight;         // max height of tallest bar

    // ── Battery ──
    int batteryMinX;              // leftmost x battery can start at (avoids overlap with content)
};
