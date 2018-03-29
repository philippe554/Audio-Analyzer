#pragma once

class App;

#include <Windows.h> 
#include <d2d1.h>
#include <vector>
#include "View.h"
#include <dwrite.h>

using namespace std;

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

class App
{
public:
	~App();
	HRESULT Initialize(int cmd_show);
	void AskRepaint();
	void RunMessageLoop();

	bool repaint = false;
	bool close = false;
	static bool LMouseButtonPressed;
	static POINT MousePos;
private:
	static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
	HRESULT CreateDeviceIndependentResources();
	HRESULT CreateDeviceResources();
	HRESULT Render();

	HWND hwnd;
	ID2D1Factory* Direct2dFactory;
	IDWriteFactory* WriteFactory;
	ID2D1HwndRenderTarget* RenderTarget;

	vector<View*> views;
};

