#pragma once
#include "PagedListHandler.h"

class FunctionSelectionHandler : public PagedListHandler {
public:
    explicit FunctionSelectionHandler(Renderer &renderer);

protected:
    int  getItemCount() const override;
    int  getItemsPerPage() const override;
    void renderCurrentPage() override;
    void onItemSelected(int index) override;
    void syncPageState(int page) override;
};
