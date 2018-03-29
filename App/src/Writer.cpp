#include "Writer.h"

IDWriteFactory* Writer::WriteFactory = nullptr;
ID2D1HwndRenderTarget* Writer::RenderTarget = nullptr;

IDWriteTextFormat* Writer::normalData = nullptr;

void Writer::init(IDWriteFactory* WF, ID2D1HwndRenderTarget* RT)
{
	WriteFactory = WF;
	RenderTarget = RT;
}

IDWriteTextFormat* Writer::normal()
{
	if (WriteFactory == NULL)throw "No writer factory set in Writer";

	if (normalData == NULL)
	{
		static const WCHAR msc_fontName[] = L"Verdana";
		WriteFactory->CreateTextFormat(
			msc_fontName,
			NULL,
			DWRITE_FONT_WEIGHT_NORMAL,
			DWRITE_FONT_STYLE_NORMAL,
			DWRITE_FONT_STRETCH_NORMAL,
			20,
			L"", //locale
			&normalData
		);
		normalData->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
	}

	return normalData;
}

void Writer::print(string text, ID2D1SolidColorBrush*color, IDWriteTextFormat*font, D2D1_RECT_F place)
{
	ID2D1Layer *pLayerText = NULL;
	RenderTarget->CreateLayer(NULL, &pLayerText);
	RenderTarget->PushLayer(D2D1::LayerParameters(place), pLayerText);

	const char	*temp_char = text.c_str();
	const WCHAR *temp_wchar;
	int j = MultiByteToWideChar(CP_ACP, 0, temp_char, -1, NULL, 0);
	temp_wchar = new WCHAR[j];
	MultiByteToWideChar(CP_ACP, 0, temp_char, -1, (LPWSTR)temp_wchar, j);

	RenderTarget->DrawText(temp_wchar, j,font,place,color);

	delete[] temp_wchar;

	RenderTarget->PopLayer();
	pLayerText->Release();
}

