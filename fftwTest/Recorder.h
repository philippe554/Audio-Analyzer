#include <Windows.h>
#include <mmsystem.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <avrt.h>
#include <functiondiscoverykeys_devpkey.h>
#include <time.h>
#include <iostream>

#include "AudioQueue.h"

const CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);
const IID IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);
const IID IID_IAudioClient = __uuidof(IAudioClient);
const IID IID_IAudioCaptureClient = __uuidof(IAudioCaptureClient);

#define SAFE_RELEASE(punk)  \
              if ((punk) != NULL)  \
                { (punk)->Release(); (punk) = NULL; }


HRESULT RecordAudioStream(AudioQueue *storage, bool* finished)
{
	HRESULT hr;
	REFERENCE_TIME hnsRequestedDuration = 10000000;
	REFERENCE_TIME hnsActualDuration;
	UINT32 bufferFrameCount;
	UINT32 numFramesAvailable;
	IMMDeviceEnumerator *pEnumerator = NULL;
	IMMDevice *pDevice = NULL;
	IAudioClient *pAudioClient = NULL;
	IAudioCaptureClient *pCaptureClient = NULL;
	WAVEFORMATEX *pwfx = NULL;
	UINT32 packetLength = 0;
	BOOL bDone = FALSE;
	BYTE *pData;
	DWORD flags;

	std::cout << CoCreateInstance(CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL, IID_IMMDeviceEnumerator, (void**)&pEnumerator);

	std::cout << pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pDevice);

	std::cout << pDevice->Activate(IID_IAudioClient, CLSCTX_ALL, NULL, (void**)&pAudioClient);

	std::cout << pAudioClient->GetMixFormat(&pwfx);

	std::cout << pAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_LOOPBACK, hnsRequestedDuration, 0, pwfx, NULL);

	std::cout << pAudioClient->GetBufferSize(&bufferFrameCount);

	std::cout << pAudioClient->GetService(IID_IAudioCaptureClient, (void**)&pCaptureClient);

	std::cout << pAudioClient->Start();

	std::cout << std::endl;

	std::cout << "Buffer Size: " << bufferFrameCount << std::endl;
	std::cout << "Sample per sec: " << pwfx->nSamplesPerSec << std::endl;
	std::cout << "Channels: " << pwfx->nChannels << std::endl;
	std::cout << "Bits per sample: " << pwfx->wBitsPerSample << std::endl;
	std::cout << "cbSize: " << pwfx->cbSize << std::endl;
	std::cout << "nBlockAlign: " << pwfx->nBlockAlign << std::endl;
	std::cout << "nAvgBytesPerSec: " << pwfx->nAvgBytesPerSec << std::endl;
	std::cout << "wFormatTag: ";

	if (pwfx->wFormatTag == WAVE_FORMAT_EXTENSIBLE) std::cout << "WAVE_FORMAT_EXTENSIBLE";
	if (pwfx->wFormatTag == WAVE_FORMAT_MPEG) std::cout << "WAVE_FORMAT_MPEG";
	if (pwfx->wFormatTag == WAVE_FORMAT_MPEGLAYER3) std::cout << "WAVE_FORMAT_MPEGLAYER3";

	//byte* data = new byte[pwfx->nSamplesPerSec * 6 * pwfx->nBlockAlign];
	int loc = 0;
	int amountOfCaptures = 0;

	while (true)
	{
		hr = pCaptureClient->GetNextPacketSize(&packetLength);

		if (packetLength > 0)
		{
			amountOfCaptures++;
			hr = pCaptureClient->GetBuffer(&pData, &numFramesAvailable, &flags, NULL, NULL);

			if (flags & AUDCLNT_BUFFERFLAGS_SILENT)
			{
				pData = NULL;  // Tell CopyData to write silence.
				std::cout << "When does this happen?" << std::endl;
			}

			//memcpy(data + loc, pData, numFramesAvailable * pwfx->nBlockAlign);
			loc += numFramesAvailable * pwfx->nBlockAlign;

			storage->enqueue((INT32*)pData, numFramesAvailable * 2);

			hr = pCaptureClient->ReleaseBuffer(numFramesAvailable);
		}
	}
	std::cout << std::endl;
	std::cout << "Samples captured: " << loc << std::endl;
	std::cout << "avg samples per capture: " << (double)loc / (double)amountOfCaptures / (double)pwfx->nBlockAlign << std::endl;

	//cout << "Sample of samples captures: " << endl;
	/*for (int i = 0; i < 1000; i+= pwfx->nBlockAlign)
	{
	unsigned int a = unsigned int((unsigned char)(data[i]) << 8 | (unsigned char)(data[i + 1]));
	unsigned int b = unsigned int((unsigned char)(data[i + 2]) << 8 | (unsigned char)(data[i + 3]));

	a /= 1000;
	b /= 1000;

	cout << "(" << a << "," << b << ")  ";
	}*/
	//INT32* ptr = (INT32*)data;
	//for (int i = 0; i < 100; i++)
	//{
		//int a = ptr[i] / 10000000;
		//std::cout << a << " ";
		//int b = ptr[i*20+1] / 10000000;

		//cout << "(" << a << "," << b << ")  ";
	//}
	//std::cout << std::endl;

	pAudioClient->Stop();

	//delete[] data;
	CoTaskMemFree(pwfx);
	SAFE_RELEASE(pEnumerator)
		SAFE_RELEASE(pDevice)
		SAFE_RELEASE(pAudioClient)
		SAFE_RELEASE(pCaptureClient)

		*finished = true;

	return hr;
}

