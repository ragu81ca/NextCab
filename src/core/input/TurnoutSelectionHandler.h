#pragma once
#include "PagedListHandler.h"
#include <WiThrottleProtocol.h>

class TurnoutSelectionHandler : public PagedListHandler {
public:
    explicit TurnoutSelectionHandler(Renderer &renderer);

    void setAction(TurnoutAction action) { action_ = action; }
    TurnoutAction getAction() const { return action_; }

protected:
    int  getItemCount() const override;
    int  getItemsPerPage() const override;
    void renderCurrentPage() override;
    void onItemSelected(int index) override;

private:
    TurnoutAction action_;
};
