// UIState.h - single definition
#pragma once
#include <Arduino.h>
#include <WiThrottleProtocol.h>
#include "RenderModel.h"  // for RENDER_MODEL_MAX_ROWS

struct UIState {
	String lines[RENDER_MODEL_MAX_ROWS];
	bool invert[RENDER_MODEL_MAX_ROWS];
	bool menuIsShowing=false;
	int page=0;
	int functionPage=0;
	bool functionHasBeenSelected=false;
	int lastScreen=0;
	String lastStringParam="";
	bool lastBoolParam=false;
	TurnoutAction lastTurnoutParam=TurnoutToggle;
	bool directCommandsDisplayed=false;
	bool hashShowsFunctions=false;

	// Broadcast / server-info overlay state (formerly .ino globals)
	String broadcastMessageText = "";
	long   broadcastMessageTime = 0;
	long   lastReceivingServerDetailsTime = 0;

	void clearLines(){ for(int i=0;i<RENDER_MODEL_MAX_ROWS;i++){ lines[i]=""; invert[i]=false; } }
};

extern UIState uiState;
