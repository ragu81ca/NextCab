#pragma once
#include "PagedListHandler.h"

class DropLocoSelectionHandler : public PagedListHandler {
public:
    explicit DropLocoSelectionHandler(Renderer &renderer);

protected:
    void configureScreen() override;
    bool handleExtraKey(char key) override;
};
