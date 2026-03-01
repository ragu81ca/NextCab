#pragma once
#include "PagedListHandler.h"

class EditConsistSelectionHandler : public PagedListHandler {
public:
    explicit EditConsistSelectionHandler(Renderer &renderer);

protected:
    void configureScreen() override;
};
