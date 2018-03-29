#include <Windows.h> 

#include "App.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, int CmdShow)
{
	//Heap error --> close
	HeapSetInformation(NULL, HeapEnableTerminationOnCorruption, NULL, 0);

	if (SUCCEEDED(CoInitialize(NULL)))
	{
		App app;
		if (SUCCEEDED(app.Initialize(CmdShow)))
		{
			app.RunMessageLoop();
		}
		CoUninitialize();
	}
}