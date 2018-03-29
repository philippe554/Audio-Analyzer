#pragma once

#include <dwrite.h>
#include <string>
#include <d2d1.h>

class Writer;

using namespace std;

class Writer
{
public:
	static void init(IDWriteFactory* WF, ID2D1HwndRenderTarget* RT);

	static IDWriteTextFormat* normal();

	static void print(string text, ID2D1SolidColorBrush*color, IDWriteTextFormat*font, D2D1_RECT_F place);
private:
	static IDWriteFactory* WriteFactory;
	static ID2D1HwndRenderTarget* RenderTarget;

	static IDWriteTextFormat* normalData;
};
#pragma once
