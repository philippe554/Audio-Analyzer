#pragma once

class MainBar;

#include <d2d1.h>
#include "View.h"

class MainBar : public View
{
public:
	MainBar(int x, int y, int xSize, int ySize);
	void render(ID2D1HwndRenderTarget* RenderTarget) override;
	void update() override;
	void ViewProc(App* app,HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) override;

private:
	D2D1_RECT_F closeButton;
};