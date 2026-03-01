#pragma once
#include "PagedListHandler.h"

class FunctionSelectionHandler : public PagedListHandler {
public:
    explicit FunctionSelectionHandler(Renderer &renderer);

    void onEnter() override;  // override for "no loco" edge case

protected:
    void configureScreen() override;
};
