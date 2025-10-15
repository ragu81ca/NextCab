// UIState.h - single definition
#pragma once
#include <Arduino.h>
#include <WiThrottleProtocol.h>

struct UIState {
	String lines[18];
	bool invert[18];
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
	void clearLines(){ for(int i=0;i<18;i++){ lines[i]=""; invert[i]=false; } }
};

extern UIState uiState;
