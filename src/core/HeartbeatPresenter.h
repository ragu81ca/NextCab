// HeartbeatPresenter.h - single definition
#pragma once
#include "RenderModel.h"
#include "UIState.h"
#include "../../static.h" // ensure menu/message macros are defined
inline void buildHeartbeatRenderModel(RenderModel &model, const UIState &state, bool heartbeatEnabled) {
	model.clear();
	model.lines[0] = "Heartbeat";
	model.lines[1] = heartbeatEnabled ? MSG_HEARTBEAT_CHECK_ENABLED : MSG_HEARTBEAT_CHECK_DISABLED;
	model.lines[5] = "* Close";
	model.drawTopLine = false;
}
