// Layout_128x64.h - Layout profile for 128×64 monochrome OLED
// All values extracted from the existing hardcoded Renderer.cpp
#pragma once
#include "../core/DisplayLayout.h"

constexpr DisplayLayout LAYOUT_128x64 = {
    // ── Screen geometry ──
    .screenWidth  = 128,
    .screenHeight = 64,

    // ── Content capacity ──
    .maxTextRows          = 6,    // 6 visible rows per column in current layout
    .maxCharsPerRow       = 21,   // ~21 chars at NokiaSmallPlain on 128px
    .rosterItemsPerPage   = 5,
    .functionItemsPerPage = 10,
    .turnoutItemsPerPage  = 10,
    .routeItemsPerPage    = 10,
    .ssidItemsPerPage     = 5,
    .menuItemsPerPage     = 9,
    .maxLocosDisplayed    = 8,

    // ── Truncation limits ──
    .rosterNameMaxLength    = 0,    // 0 = no truncation (current code doesn't truncate roster names)
    .ssidNameMaxLength      = 13,
    .turnoutNameMaxLength   = 10,
    .routeNameMaxLength     = 10,
    .functionLabelMaxLength = 10,   // substring(0,10) for fn page 0; 7 for pages 1+
    .locoNameMaxLength      = 0,    // 0 = no truncation

    // ── Column layout ──
    .columnCount       = 2,
    .columnWidth       = 64,   // xInc = 64 in 2-column mode
    .threeColumnWidth  = 42,   // xInc = 42 in 3-column mode
    .twoColumnMax      = 12,   // max 12 items in 2-column layout
    .threeColumnMax    = 18,   // max 18 items in 3-column layout

    // ── Row layout ──
    .rowHeight    = 10,
    .topBarHeight = 11,  // separator line y and top-bar content baseline
    .statusBarY   = 51,  // bottom separator y
    .rightMargin  = 2,   // 2px right margin (battery rendering)
    .menuTextRow  = 5,   // last row of first column (6 rows per col)
    .secondColumnStartRow = 6,  // first row of second column

    // ── Speed screen zones ──
    .speedX           = 22,   // drawStr(22+(55-width), 45, ...)
    .speedY           = 45,
    .directionX       = 79,
    .directionY       = 36,
    .throttleNumberX  = 2,
    .throttleNumberY  = 15,
    .throttleNumberBoxW = 12,
    .throttleNumberBoxH = 16,

    // ── Function indicators ──
    .functionIndicatorStartX  = 12,
    .functionIndicatorY       = 13,   // drawRBox(i*4+12, 12+1, 5, 7, 2)
    .functionIndicatorBoxW    = 5,
    .functionIndicatorBoxH    = 7,
    .functionIndicatorSpacing = 4,    // i*4 spacing

    // ── Track power / heartbeat / speed step / momentum / brake ──
    .trackPowerBoxX      = 0,
    .trackPowerBoxY      = 40,
    .trackPowerBoxW      = 9,
    .trackPowerBoxH      = 9,
    .trackPowerGlyphX    = 1,
    .trackPowerGlyphY    = 48,
    .heartbeatGlyphX     = 13,
    .heartbeatGlyphY     = 49,
    .heartbeatStrikeX1   = 13,
    .heartbeatStrikeY1   = 48,
    .heartbeatStrikeX2   = 20,
    .heartbeatStrikeY2   = 41,
    .speedStepGlyphX     = 1,
    .speedStepGlyphY     = 38,
    .speedStepTextX      = 9,
    .speedStepTextY      = 37,
    .momentumTextX       = 13,
    .momentumTextY       = 38,
    .brakeTextX          = 22,
    .brakeTextY          = 38,

    // ── Next throttle info ──
    .nextThrottleNumberX = 119,  // 85+34
    .nextThrottleNumberY = 38,
    .nextThrottleSpeedX  = 97,   // 85+12
    .nextThrottleSpeedY  = 48,

    // ── Wi-Fi signal bars ──
    .wifiBarWidth     = 2,
    .wifiBarGap       = 1,
    .wifiBarMaxHeight = 8,

    // ── Battery ──
    .batteryMinX = 80,   // don't let battery overlap speed area
};
