#pragma once
#include "PagedListHandler.h"

class RouteSelectionHandler : public PagedListHandler {
public:
    explicit RouteSelectionHandler(Renderer &renderer);

protected:
    void configureScreen() override;
};
