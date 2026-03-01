#pragma once
#include "PagedListHandler.h"

class RosterSelectionHandler : public PagedListHandler {
public:
    explicit RosterSelectionHandler(Renderer &renderer);

protected:
    void configureScreen() override;
};
