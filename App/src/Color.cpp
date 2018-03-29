#include "Color.h"

ID2D1HwndRenderTarget* Color::RenderTarget = nullptr;

ID2D1SolidColorBrush* Color::lightGrayData = nullptr;
ID2D1SolidColorBrush* Color::grayData = nullptr;
ID2D1SolidColorBrush* Color::blackData = nullptr;
ID2D1SolidColorBrush* Color::redData = nullptr;
ID2D1SolidColorBrush* Color::greenData = nullptr;
map<int, ID2D1SolidColorBrush*> Color::heightMapData= map<int, ID2D1SolidColorBrush*>();

void Color::init(ID2D1HwndRenderTarget* RT)
{
	RenderTarget = RT;
}

ID2D1SolidColorBrush* Color::lightGray()
{
	if (RenderTarget == nullptr)throw "No render target set in Color";

	if(lightGrayData == nullptr)
	{
		RenderTarget->CreateSolidColorBrush(
			D2D1::ColorF(D2D1::ColorF::LightGray),
			&lightGrayData);
	}

	return lightGrayData;
}
ID2D1SolidColorBrush* Color::gray()
{
	if (RenderTarget == nullptr)throw "No render target set in Color";

	if (grayData == nullptr)
	{
		RenderTarget->CreateSolidColorBrush(
			D2D1::ColorF(D2D1::ColorF::Gray),
			&grayData);
	}

	return grayData;
}
ID2D1SolidColorBrush* Color::black()
{
	if (RenderTarget == nullptr)throw "No render target set in Color";

	if (blackData == nullptr)
	{
		RenderTarget->CreateSolidColorBrush(
			D2D1::ColorF(D2D1::ColorF::Black),
			&blackData);
	}

	return blackData;
}

ID2D1SolidColorBrush* Color::red()
{
	if (RenderTarget == nullptr)throw "No render target set in Color";

	if (redData == nullptr)
	{
		RenderTarget->CreateSolidColorBrush(
			D2D1::ColorF(D2D1::ColorF::Red),
			&redData);
	}

	return redData;
}

ID2D1SolidColorBrush * Color::green()
{
	if (RenderTarget == nullptr)throw "No render target set in Color";

	if (greenData == nullptr)
	{
		RenderTarget->CreateSolidColorBrush(
			D2D1::ColorF(D2D1::ColorF::Green),
			&greenData);
	}

	return greenData;
}

ID2D1SolidColorBrush* Color::heightMap(double x)
{
	int index = x * 255;
	if (index < 0)index = 0;
	if (index > 255)index = 255;
	double i = index / 255.0;
	if (RenderTarget == nullptr)throw "No render target set in Color";

	if (heightMapData.count(index) == 0)
	{
		ID2D1SolidColorBrush* data;
		RenderTarget->CreateSolidColorBrush(
			D2D1::ColorF(i, i, i, 0.5),
			&data);
		heightMapData[index] = data;
	}

	return heightMapData.at(index);
}
