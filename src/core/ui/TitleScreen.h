// TitleScreen.h — Data-only model for status / info / splash screens.
//
// Title screens show a small number of text lines — a header, 1-4 body
// lines, and an optional footer.  The Renderer vertically centres the body
// content between the top bar and the status bar so it looks balanced on
// any display size.
//
// Key principle: the screen knows NOTHING about pixels or fonts.
#pragma once

#include "IScreen.h"

class TitleScreen : public IScreen {
public:
    static constexpr int MAX_BODY_LINES = 4;

    // ── IScreen ─────────────────────────────────────────────────────────
    ScreenType type() const override { return ScreenType::Title; }

    // ── Content ─────────────────────────────────────────────────────────
    String title;                           // Row 0 — e.g. app name, "Disconnected"
    String subtitle;                        // Drawn at secondColumnStartRow (small OLED)
                                            // or row 1 on large displays when present
    String body[MAX_BODY_LINES];            // Vertically centred in available space
    int    bodyCount     = 0;

    String footerText;                      // Displayed at menuTextRow

    bool   showTopLine   = true;            // Draw the top-bar separator

    // ── Convenience builders ────────────────────────────────────────────
    void addBody(const String &text) {
        if (bodyCount < MAX_BODY_LINES) body[bodyCount++] = text;
    }

    void clear() {
        title = "";
        subtitle = "";
        for (int i = 0; i < MAX_BODY_LINES; i++) body[i] = "";
        bodyCount   = 0;
        footerText  = "";
        showTopLine = true;
    }

    /// Convenience: set title + subtitle to app name / version.
    void setAppHeader(const String &appName, const String &appVersion) {
        title    = appName;
        subtitle = appVersion;
    }
};
