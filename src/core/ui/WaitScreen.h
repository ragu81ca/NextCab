// WaitScreen.h — A TitleScreen that carries animation state for "please wait"
// screens (browsing, connecting, etc.).
//
// The Renderer draws the same header / body / footer as TitleScreen but adds
// a graphical bouncing-dot spinner below the body text.  Callers just call
// advance() each iteration and re-render — no getDots(), no manual body-array
// manipulation.
#pragma once

#include "TitleScreen.h"

class WaitScreen : public TitleScreen {
public:
    ScreenType type() const override { return ScreenType::Wait; }

    int frame = 0;          // current animation frame (caller increments)

    /// Advance the animation by one step.
    void advance() { frame++; }
};
