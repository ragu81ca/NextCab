// HeartbeatPresenter.h - single definition
#pragma once
#include "RenderModel.h"
#include "UIState.h"
#include "../../static.h" // ensure menu/message macros are defined
inline void buildHeartbeatRenderModel(RenderModel &model, const UIState &state, bool heartbeatEnabled) {
	model.clear();
	model.lines[0] = MENU_ITEM_TEXT_TITLE_HEARTBEAT;
	model.lines[1] = heartbeatEnabled ? MSG_HEARTBEAT_CHECK_ENABLED : MSG_HEARTBEAT_CHECK_DISABLED;
	model.lines[5] = MENU_ITEM_TEXT_MENU_HEARTBEAT;
	model.drawTopLine = false;
}
