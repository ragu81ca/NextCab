#pragma once
#include "PagedListHandler.h"

class RosterSelectionHandler : public PagedListHandler {
public:
    explicit RosterSelectionHandler(Renderer &renderer);

protected:
    int  getItemCount() const override;
    int  getItemsPerPage() const override;
    void renderCurrentPage() override;
    void onItemSelected(int index) override;
};
