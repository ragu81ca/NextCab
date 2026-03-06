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
    .functionItemsPerPage = 10,   // capped at 10 — keypad keys 1-9,0 limit
    .turnoutItemsPerPage  = 10,   // capped at 10 — keypad keys 1-9,0 limit
    .routeItemsPerPage    = 10,   // capped at 10 — keypad keys 1-9,0 limit
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

    // ── Speed screen zones ── (scaled for 78px speed font)
    .speedX           = 60,
    .speedY           = 160,
    .directionX       = 228,
    .directionY       = 100,   // approximate; renderer computes exact top-alignment
    .throttleNumberX  = 4,
    .throttleNumberBoxW = 24,
    .throttleNumberBoxH = 30,

    // ── Function indicators (50% larger than OLED) ──
    .functionIndicatorStartX  = 26,
    .functionIndicatorY       = 24,
    .functionIndicatorBoxW    = 12,
    .functionIndicatorBoxH    = 18,
    .functionIndicatorSpacing = 14,
    .functionIndicatorTextYOffset = 14, // vertically centred in 18px box with 10px font

    // ── Track power / heartbeat / speed step (32px glyphs, hugging status bar) ──
    //    Status bar at y=218. Track power box 34px tall, 2px gap above bar.
    .trackPowerBoxX      = 0,
    .trackPowerBoxY      = 182,    // 218 - 2 - 34
    .trackPowerBoxW      = 34,
    .trackPowerBoxH      = 34,
    .trackPowerGlyphX    = 2,
    .trackPowerGlyphY    = 214,    // baseline inside box
    .heartbeatGlyphX     = 40,
    .heartbeatGlyphY     = 214,
    .heartbeatStrikeX1   = 40,
    .heartbeatStrikeY1   = 212,
    .heartbeatStrikeX2   = 68,
    .heartbeatStrikeY2   = 186,
    .speedStepGlyphX     = 2,
    .speedStepGlyphY     = 178,    // just above track power box
    .speedStepTextX      = 34,
    .speedStepTextY      = 174,

    // ── Momentum train indicator (loco + 0/1/2 cars, below function indicators) ──
    //    Function indicators end at y ≈ 43 (y=24+1+18). Train at +2px gap.
    .momentumX     = 2,
    .momentumY     = 45,
    .momentumScale = 2,

    // ── Next throttle info ──
    .nextThrottleNumberX = 290,
    .nextThrottleNumberY = 120,
    .nextThrottleSpeedX  = 260,
    .nextThrottleSpeedY  = 180,

    // ── Wi-Fi signal bars ──
    .wifiBarWidth     = 4,
    .wifiBarGap       = 2,
    .wifiBarMaxHeight = 16,

    // ── Battery ──
    .batteryMinX = 200,
};
