#include "MainBar.h"
#include "Color.h"

MainBar::MainBar(int x, int y, int xSize, int ySize):View(x, y, xSize, ySize)
{
	int border = ySize*0.1;
	closeButton = D2D1::RectF(x + xSize - ySize + border, y + border, x + xSize - border, y + ySize - border);
}

void MainBar::render(ID2D1HwndRenderTarget* RenderTarget)
{
	RenderTarget->FillRectangle(place,Color::lightGray());
	RenderTarget->FillRectangle(closeButton, Color::gray());
}

void MainBar::update()
{
	
}

void MainBar::ViewProc(App*app,HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (message == WM_LBUTTONDOWN)
	{
		POINT pt = { LOWORD(lParam), HIWORD(lParam)};
		pt.x -= place.left;
		pt.y -= place.top;
		if (pt.x >= closeButton.left&&pt.x <= closeButton.right
			&&pt.y >= closeButton.top&&pt.y <= closeButton.bottom)
		{
			app->close = true;
		}else if (pt.x >= place.left&&pt.x <= place.right
			&&pt.y >= place.top&&pt.y <= place.bottom)
		{
			app->LMouseButtonPressed = false;
			SendMessage(hwnd, WM_NCLBUTTONDOWN, HTCAPTION, lParam);
		}
	}
}
