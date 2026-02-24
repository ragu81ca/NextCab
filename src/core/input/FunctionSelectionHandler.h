#pragma once
#include "PagedListHandler.h"

class FunctionSelectionHandler : public PagedListHandler {
public:
    explicit FunctionSelectionHandler(Renderer &renderer);

    void onEnter() override;  // override for "no loco" edge case

protected:
    int    getItemCount() const override;
    int    getItemsPerPage() const override;
    String getItemLabel(int globalIndex, bool &invert) const override;
    String getFooterText() const override;
    void   onItemSelected(int index) override;
    bool   useHalfPageSplit() const override { return true; }
    void   onBeforeRender() override;
    void   syncPageState(int page) override;
};
