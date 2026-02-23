// RenderModel.h - single definition
#pragma once
#include <Arduino.h>

// Maximum rows any supported display can hold.
// 128×64 uses up to 18 (6 rows × 3 columns); 320×240 could use up to 36 (12 × 3).
#define RENDER_MODEL_MAX_ROWS 36

struct RenderModel {
	String lines[RENDER_MODEL_MAX_ROWS];
	bool invert[RENDER_MODEL_MAX_ROWS];
	bool drawTopLine=false;
	bool sendBuffer=true;
	void clear(){ for(int i=0;i<RENDER_MODEL_MAX_ROWS;i++){ lines[i]=""; invert[i]=false; } drawTopLine=false; sendBuffer=true; }
};
