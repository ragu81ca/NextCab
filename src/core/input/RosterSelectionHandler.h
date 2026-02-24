#pragma once
#include "PagedListHandler.h"

class RosterSelectionHandler : public PagedListHandler {
public:
    explicit RosterSelectionHandler(Renderer &renderer);

protected:
    int    getItemCount() const override;
    int    getItemsPerPage() const override;
    String getItemLabel(int globalIndex, bool &invert) const override;
    String getFooterText() const override;
    void   onItemSelected(int index) override;
    void   onBeforeRender() override;
};
