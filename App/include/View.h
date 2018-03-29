#pragma once

class View;

#include <d2d1.h>
#include <Windows.h>
#include "App.h"

class View
{
public:
	View(int x,int y,int xSize,int ySize);

	virtual void render(ID2D1HwndRenderTarget* RenderTarget) =0;
	virtual void update() =0;
	virtual void ViewProc(App* app,HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) =0;

	D2D1_RECT_F place;
};