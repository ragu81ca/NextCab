// RenderModel.h - single definition
#pragma once
#include <Arduino.h>
struct RenderModel {
	String lines[18];
	bool invert[18];
	bool drawTopLine=false;
	bool sendBuffer=true;
	void clear(){ for(int i=0;i<18;i++){ lines[i]=""; invert[i]=false; } drawTopLine=false; sendBuffer=true; }
};
