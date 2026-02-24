#pragma once
#include "PagedListHandler.h"

// Forward declarations
class ThrottleManager;

class EditConsistSelectionHandler : public PagedListHandler {
public:
    explicit EditConsistSelectionHandler(Renderer &renderer);

protected:
    int  getItemCount() const override;
    int  getItemsPerPage() const override;
    void renderCurrentPage() override;
    void onItemSelected(int index) override;
};
