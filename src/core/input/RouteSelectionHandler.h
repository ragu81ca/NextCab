#pragma once
#include "PagedListHandler.h"

class RouteSelectionHandler : public PagedListHandler {
public:
    explicit RouteSelectionHandler(Renderer &renderer);

protected:
    int    getItemCount() const override;
    int    getItemsPerPage() const override;
    String getItemLabel(int globalIndex, bool &invert) const override;
    String getFooterText() const override;
    void   onItemSelected(int index) override;
    bool   useHalfPageSplit() const override { return true; }
    void   onBeforeRender() override;
};
