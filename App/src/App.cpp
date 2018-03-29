#include "App.h"
#include <ctime>
#include "MainBar.h"
#include "AudioAnalyzer.h"
#include "Color.h"
#include "Writer.h"

bool App::LMouseButtonPressed = false;
POINT App::MousePos = {0,0};

App::~App()
{
	while (views.size()>0)
	{
		delete views.at(0);
		views.at(0) = nullptr;
		views.erase(views.begin());
	}
}

HRESULT App::Initialize(int nCmdShow)
{
	if (!SUCCEEDED(CreateDeviceIndependentResources()))
	{
		MessageBox(NULL, TEXT("Error Create Device Independent Resources"), TEXT("ERROR"), MB_OK);
		return E_FAIL;
	}

	WNDCLASS wc;

	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hbrBackground = CreateSolidBrush(RGB(255, 255, 255));
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hIcon = NULL;// LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_MYICON));
	wc.hInstance = (HINSTANCE)::GetModuleHandle(NULL);;
	wc.lpfnWndProc = App::WndProc;
	wc.lpszClassName = "WIN_CLASS";
	wc.lpszMenuName = 0;
	wc.style = CS_HREDRAW | CS_VREDRAW;

	if (!RegisterClass(&wc))
	{
		MessageBox(NULL, TEXT("Error registering class"), TEXT("ERROR"), MB_OK);
		return E_FAIL;
	}

	hwnd = CreateWindow("WIN_CLASS",
		"AI Cells 2",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		1000,
		800,
		(HWND)NULL,
		(HMENU)NULL,
		GetModuleHandle(NULL),
		this);

	//delete window border
	LONG lStyle = GetWindowLong(hwnd, GWL_STYLE);
	lStyle &= ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZE | WS_MAXIMIZE | WS_SYSMENU);
	SetWindowLong(hwnd, GWL_STYLE, lStyle);

	LONG lExStyle = GetWindowLong(hwnd, GWL_EXSTYLE);
	lExStyle &= ~(WS_EX_DLGMODALFRAME | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE);
	SetWindowLong(hwnd, GWL_EXSTYLE, lExStyle);

	SetWindowPos(hwnd, NULL, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER);

	if (!hwnd)
	{
		MessageBox(NULL, TEXT("Call to CreateWindow failed!"), TEXT("ERROR"), MB_OK);
		return E_FAIL;
	}

	if (!SUCCEEDED(CreateDeviceResources()))
	{
		MessageBox(NULL, TEXT("Error Create Device Resources"), TEXT("ERROR"), MB_OK);
		return E_FAIL;
	}

	repaint = 1;
	close = 0;

	views.push_back(new MainBar(0, 0, 1000, 100));
	views.push_back(new AudioAnalyzer(0, 100, 1000, 700));

	ShowWindow(hwnd, nCmdShow);

	return S_OK;
}

void	App::AskRepaint()
{
	repaint = 1;
}

HRESULT App::CreateDeviceIndependentResources()
{
	// Create a Direct2D factory.
	HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_MULTI_THREADED, &Direct2dFactory);

	if (SUCCEEDED(hr))
	{
		// Create a DirectWrite factory.
		hr = DWriteCreateFactory(
			DWRITE_FACTORY_TYPE_SHARED,
			__uuidof(WriteFactory),
			reinterpret_cast<IUnknown **>(&WriteFactory)
		);
	}

	return hr;
}
HRESULT App::CreateDeviceResources()
{
	HRESULT hr = S_OK;

	RECT rc;
	GetClientRect(hwnd, &rc);
	D2D1_SIZE_U size = D2D1::SizeU(
		rc.right - rc.left,
		rc.bottom - rc.top
	);

	// Create a Direct2D render target.
	hr = Direct2dFactory->CreateHwndRenderTarget(
		D2D1::RenderTargetProperties(),
		D2D1::HwndRenderTargetProperties(hwnd, size),
		&RenderTarget
	);

	Color::init(RenderTarget);
	Writer::init(WriteFactory, RenderTarget);

	return hr;
}

HRESULT App::Render()
{
	HRESULT hr = S_OK;

	RenderTarget->BeginDraw();
	RenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
	RenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::WhiteSmoke));

	for(int i=0;i<views.size();i++)
	{
		ID2D1Layer *pLayer = nullptr;
		RenderTarget->CreateLayer(nullptr, &pLayer);
		RenderTarget->PushLayer(D2D1::LayerParameters(views.at(i)->place), pLayer);

		RenderTarget->SetTransform(D2D1::Matrix3x2F::Translation(views.at(i)->place.left, views.at(i)->place.top));

		views.at(i)->render(RenderTarget);

		RenderTarget->PopLayer();
		pLayer->Release();
		pLayer = nullptr;
	}

	/*for (int k = 0; k < AreaProc.size(); k++)
	{
		if (AreaProc[k] != NULL)
		{
			if (AreaProc[k]->Activated)
			{
				ID2D1Layer *pLayer = NULL;
				RenderTarget->CreateLayer(NULL, &pLayer);
				RenderTarget->PushLayer(D2D1::LayerParameters(AreaProc[k]->MainRect), pLayer);

				for (int i = 0; i < AreaProc[k]->Area.size(); i++)
				{
					int colorID = AreaProc[k]->Area[i]->colorID;
					if (AreaProc[k]->Area[i]->areaButton)
					{
						colorID = AreaProc[k]->Area[i]->areaButton->fillData[AreaProc[k]->Area[i]->areaButton->state];
					}
					if (AreaProc[k]->Area[i]->form == AREA::f_box)
					{
						if (((AREABOX*)(AreaProc[k]->Area[i]))->bitmap)
						{
							RenderTarget->DrawBitmap(Bitmap[colorID], &RectRelativeToRect(AreaProc[k]->MainRect, ((AREABOX*)(AreaProc[k]->Area[i]))->rect));
						}
						else
						{
							RenderTarget->FillRectangle(&RectRelativeToRect(AreaProc[k]->MainRect, ((AREABOX*)(AreaProc[k]->Area[i]))->rect), color[colorID]);
						}
					}
					else if (AreaProc[k]->Area[i]->form == AREA::f_circel)
					{
						RenderTarget->FillEllipse(D2D1::Ellipse(D2D1::Point2F(AreaProc[k]->MainRect.left + ((AREACIRCEL*)(AreaProc[k]->Area[i]))->x,
							AreaProc[k]->MainRect.top + ((AREACIRCEL*)(AreaProc[k]->Area[i]))->y), ((AREACIRCEL*)(AreaProc[k]->Area[i]))->xRadius, ((AREACIRCEL*)(AreaProc[k]->Area[i]))->yRadius),
							color[colorID]);
					}
					else if (AreaProc[k]->Area[i]->form == AREA::f_line)
					{
						RenderTarget->DrawLine(((AREALINE*)(AreaProc[k]->Area[i]))->p1, ((AREALINE*)(AreaProc[k]->Area[i]))->p2, color[colorID]);
					}
					else if (AreaProc[k]->Area[i]->form == AREA::f_path)
					{
						RenderTarget->FillGeometry(((AREAPATH*)(AreaProc[k]->Area[i]))->path, color[colorID]);
					}
					if (AreaProc[k]->Area[i]->areaText)
					{
						ID2D1Layer *pLayerText = NULL;
						RenderTarget->CreateLayer(NULL, &pLayerText);
						RenderTarget->PushLayer(D2D1::LayerParameters(RectRelativeToRect(AreaProc[k]->MainRect, AreaProc[k]->Area[i]->areaText->rect)), pLayerText);

						const char	*temp_char = AreaProc[k]->Area[i]->areaText->text.c_str();
						const WCHAR *temp_wchar;
						int j = MultiByteToWideChar(CP_ACP, 0, temp_char, -1, NULL, 0);
						temp_wchar = new WCHAR[j];
						MultiByteToWideChar(CP_ACP, 0, temp_char, -1, (LPWSTR)temp_wchar, j);

						RenderTarget->DrawText(temp_wchar, j,
							TextFormat[AreaProc[k]->Area[i]->areaText->formatID],
							RectRelativeToRect(AreaProc[k]->MainRect, AreaProc[k]->Area[i]->areaText->rect),
							color[AreaProc[k]->Area[i]->areaText->colorID]);

						RenderTarget->PopLayer();
						SafeRelease(&pLayerText);
					}
				}

				RenderTarget->PopLayer();
				SafeRelease(&pLayer);
			}
		}
	}*/

	RenderTarget->EndDraw();

	return hr;
}

void	App::RunMessageLoop()
{
	MSG msg;

	long time = clock();

	while (true)
	{
		//standard messages
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)>0) //TODO: Check PM_REMOVE and ">0"
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		//cooldown proces
		if (GetForegroundWindow() != hwnd)
		{
			Sleep(10);
		}
		//repaint
		if (repaint == 1)
		{
			InvalidateRect(hwnd, NULL, FALSE);
			//repaint = 0;
		}
		for (int i = 0; i < views.size(); i++)
		{
			views.at(i)->update();
		}
		//close
		if (close == 1)
		{
			break;
		}
	}
}
LRESULT CALLBACK App::WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (message == WM_CREATE)
	{
		LPCREATESTRUCT pcs = (LPCREATESTRUCT)lParam;
		App *app = (App *)pcs->lpCreateParams;
		SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)app);

		app->LMouseButtonPressed = 0;

		return DefWindowProc(hwnd, message, wParam, lParam);
	}

	App *app = (App*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
	if (app == NULL)
	{
		return DefWindowProc(hwnd, message, wParam, lParam);
	}

	if (message == WM_PAINT) {
		app->Render();
		ValidateRect(hwnd, NULL);
	}
	else if (message == WM_CLOSE)
	{
		app->close = 1;
	}
	else if (message == WM_LBUTTONDOWN)
	{
		app->LMouseButtonPressed = true;
		MousePos = { LOWORD(lParam), HIWORD(lParam) };
	}
	else if (message == WM_LBUTTONUP)
	{
		app->LMouseButtonPressed = false;
		MousePos = { LOWORD(lParam), HIWORD(lParam) };
	}
	else if (message == WM_MOUSEMOVE)
	{
		MousePos = { LOWORD(lParam), HIWORD(lParam) };
	}

	for (int i = 0; i < app->views.size(); i++)
	{
		app->views.at(i)->ViewProc(app, hwnd, message, wParam, lParam);
	}

	return DefWindowProc(hwnd, message, wParam, lParam);
}
