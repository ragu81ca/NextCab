#pragma once
#include "PagedListHandler.h"

class RouteSelectionHandler : public PagedListHandler {
public:
    explicit RouteSelectionHandler(Renderer &renderer);

protected:
    int  getItemCount() const override;
    int  getItemsPerPage() const override;
    void renderCurrentPage() override;
    void onItemSelected(int index) override;
};
