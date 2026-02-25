// Layout_320x240.h - Layout profile for 320×240 color TFT display
// Designed to actually use the additional real estate: more items per page,
// longer names, more columns, and proportionally larger UI elements.
#pragma once
#include "../core/DisplayLayout.h"

constexpr DisplayLayout LAYOUT_320x240 = {
    // ── Screen geometry ──
    .screenWidth  = 320,
    .screenHeight = 240,

    // ── Content capacity ── THESE are what make the bigger display useful
    .maxTextRows          = 12,   // double the rows (was 6)
    .maxCharsPerRow       = 40,   // nearly double the width
    .rosterItemsPerPage   = 10,   // was 5 — show 10 roster entries at once
    .functionItemsPerPage = 20,   // was 10 — show 20 functions at once
    .turnoutItemsPerPage  = 20,   // was 10
    .routeItemsPerPage    = 20,   // was 10
    .ssidItemsPerPage     = 10,   // was 5
    .menuItemsPerPage     = 18,   // was 9
    .maxLocosDisplayed    = 16,   // was 8

    // ── Truncation limits ── show full names on larger display
    .rosterNameMaxLength    = 30,  // was 0/untruncated but now fits more
    .ssidNameMaxLength      = 30,  // was 13
    .turnoutNameMaxLength   = 25,  // was 10
    .routeNameMaxLength     = 25,  // was 10
    .functionLabelMaxLength = 20,  // was 10/7
    .locoNameMaxLength      = 25,  // was 0/untruncated

    // ── Column layout ──
    .columnCount       = 4,
    .columnWidth       = 80,   // 320/4 = 80
    .threeColumnWidth  = 106,  // 320/3 ≈ 106
    .twoColumnMax      = 24,   // 12 rows × 2 columns
    .threeColumnMax    = 36,   // 12 rows × 3 columns

    // ── Row layout ──
    .rowHeight    = 20,
    .topBarHeight = 22,
    .statusBarY   = 218,  // near bottom of 240
    .rightMargin  = 4,
    .menuTextRow  = 11,  // last row of first column (12 rows per col)
    .secondColumnStartRow = 12,  // first row of second column

    // ── Speed screen zones ── (scaled proportionally)
    .speedX           = 70,
    .speedY           = 140,
    .directionX       = 200,
    .directionY       = 100,
    .throttleNumberX  = 4,
    .throttleNumberY  = 28,
    .throttleNumberBoxW = 24,
    .throttleNumberBoxH = 30,

    // ── Function indicators ──
    .functionIndicatorStartX  = 24,
    .functionIndicatorY       = 24,
    .functionIndicatorBoxW    = 8,
    .functionIndicatorBoxH    = 12,
    .functionIndicatorSpacing = 9,

    // ── Track power / heartbeat / speed step / momentum / brake ──
    .trackPowerBoxX      = 0,
    .trackPowerBoxY      = 150,
    .trackPowerBoxW      = 18,
    .trackPowerBoxH      = 18,
    .trackPowerGlyphX    = 2,
    .trackPowerGlyphY    = 166,
    .heartbeatGlyphX     = 26,
    .heartbeatGlyphY     = 166,
    .heartbeatStrikeX1   = 26,
    .heartbeatStrikeY1   = 164,
    .heartbeatStrikeX2   = 40,
    .heartbeatStrikeY2   = 150,
    .speedStepGlyphX     = 2,
    .speedStepGlyphY     = 120,
    .speedStepTextX      = 18,
    .speedStepTextY      = 118,
    .momentumTextX       = 26,
    .momentumTextY       = 120,
    .brakeTextX          = 44,
    .brakeTextY          = 120,

    // ── Next throttle info ──
    .nextThrottleNumberX = 290,
    .nextThrottleNumberY = 100,
    .nextThrottleSpeedX  = 260,
    .nextThrottleSpeedY  = 140,

    // ── Wi-Fi signal bars ──
    .wifiBarWidth     = 4,
    .wifiBarGap       = 2,
    .wifiBarMaxHeight = 16,

    // ── Battery ──
    .batteryMinX = 200,
};
