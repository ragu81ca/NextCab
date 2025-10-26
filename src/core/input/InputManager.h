#pragma once
#include <Arduino.h>
#include "IModeHandler.h"
#include "IInputDevice.h"

// Simple mode id for now; can expand to enum class later.
enum class InputMode : uint8_t {
    Operation,
    PasswordEntry,
    RosterSelection,
    TurnoutSelection,
    RouteSelection,
    FunctionSelection,
    DropLocoSelection,
    EditConsist
};

class InputManager {
public:
    InputManager() = default;

    void setOperationHandler(IModeHandler *h) { opHandler_ = h; }
    void setPasswordHandler(IModeHandler *h) { pwHandler_ = h; }
    void setRosterSelectionHandler(IModeHandler *h) { rosterHandler_ = h; }
    void setTurnoutSelectionHandler(IModeHandler *h) { turnoutHandler_ = h; }
    void setRouteSelectionHandler(IModeHandler *h) { routeHandler_ = h; }
    void setFunctionSelectionHandler(IModeHandler *h) { functionHandler_ = h; }
    void setDropLocoSelectionHandler(IModeHandler *h) { dropLocoHandler_ = h; }
    void setEditConsistHandler(IModeHandler *h) { editConsistHandler_ = h; }
    void setActionFallbackHandler(IModeHandler *h) { actionHandler_ = h; }

    void setMode(InputMode mode);
    void forceMode(InputMode mode); // always rebind active handler & call onEnter
    InputMode getMode() const { return mode_; }

    // Dispatch event to active mode; if not consumed can implement fallbacks.
    void dispatch(const InputEvent &ev);

    // Register an input device (ownership managed externally; lifetime must exceed manager).
    bool registerDevice(IInputDevice* dev) {
        if (!dev || deviceCount_ >= MaxDevices) return false;
        devices_[deviceCount_++] = dev; return true;
    }
    void beginAllDevices() {
        for (size_t i=0;i<deviceCount_;++i) devices_[i]->begin();
    }
    void pollAllDevices() {
        for (size_t i=0;i<deviceCount_;++i) devices_[i]->poll();
    }
    size_t deviceCount() const { return deviceCount_; }
    IInputDevice* deviceAt(size_t idx) const { return (idx<deviceCount_) ? devices_[idx] : nullptr; }

private:
    InputMode mode_ { InputMode::Operation };
    IModeHandler *opHandler_ { nullptr };
    IModeHandler *pwHandler_ { nullptr };
    IModeHandler *rosterHandler_ { nullptr };
    IModeHandler *turnoutHandler_ { nullptr };
    IModeHandler *routeHandler_ { nullptr };
    IModeHandler *functionHandler_ { nullptr };
    IModeHandler *dropLocoHandler_ { nullptr };
    IModeHandler *editConsistHandler_ { nullptr };
    IModeHandler *actionHandler_ { nullptr }; // used for Action events if active handler doesn't consume
    IModeHandler *active_ { nullptr };
    static constexpr size_t MaxDevices = 8;
    IInputDevice* devices_[MaxDevices] { nullptr }; // non-owning pointers
    size_t deviceCount_ { 0 };
    // Centralize mode->handler binding to remove duplicated switch logic.
    IModeHandler* resolveHandlerForMode(InputMode m) const {
        switch (m) {
            case InputMode::Operation: return opHandler_;
            case InputMode::PasswordEntry: return pwHandler_;
            case InputMode::RosterSelection: return rosterHandler_;
            case InputMode::TurnoutSelection: return turnoutHandler_;
            case InputMode::RouteSelection: return routeHandler_;
            case InputMode::FunctionSelection: return functionHandler_;
            case InputMode::DropLocoSelection: return dropLocoHandler_;
            case InputMode::EditConsist: return editConsistHandler_;
            default: return nullptr;
        }
    }
};
