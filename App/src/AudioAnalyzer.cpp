#include <mmdeviceapi.h>
#include <audioclient.h>
#include "AudioAnalyzer.h"
#include "Color.h"
#include <ctime>
#include <cstdlib>
#include "Writer.h"
#include <random>

const CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);
const IID IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);
const IID IID_IAudioClient = __uuidof(IAudioClient);
const IID IID_IAudioCaptureClient = __uuidof(IAudioCaptureClient);

double normalCDF(double x)
{
	return std::erfc(-x / std::sqrt(2)) / 2;
}

AudioAnalyzer::AudioAnalyzer(int x, int y, int xSize, int ySize) 
	:View(x, y, xSize, ySize), raw(960000), beatPointPredicted(20000)
{
	plotX = 100;
	plotY = 100;
	plotXSize = 800;
	plotYSize = 500;

	axisXStart = 0;
	axisYStart = -0.1;
	axisXEnd = 10;
	axisYEnd = 2;

	axisXUnit = 5;
	axisYUnit = 0.2;
	axisUnitSize = 5;

	plotTime = 0;
	calcTime = 0;
	captureTime = 0;

	recordReady = false;

	for (int i = 0; i < SERIES::END; i++)
	{
		series.emplace(std::piecewise_construct, std::make_tuple(i), std::make_tuple());
	}

	//series[SERIES::lowSum].plot = true;

	series[SERIES::lowMeanLong].plot = true;
	series[SERIES::lowMeanShort].plot = true;
	series[SERIES::lowMeanLong].offset = 0.5;
	series[SERIES::lowMeanShort].offset = 0.5;

	series[SERIES::lowStdShortHigh].plot = true;
	series[SERIES::lowStdShortHigh].offset = 0.5;
	series[SERIES::lowStdShortHigh].color = Color::green();

	//series[SERIES::lowStdLongLow].plot = true;

	//series[SERIES::lowStdLongLow].color = Color::green();

	//series[SERIES::meanBeat].plot = true;

	/*series[SERIES::meanBeatPoint].plot = true;
	series[SERIES::meanBeatPoint].type = 'd';
	series[SERIES::meanBeatPoint].color = Color::green();*/
	
	/*series[SERIES::predictedBeat].plot = true;
	series[SERIES::predictedBeat].type = 'd';
	series[SERIES::predictedBeat].color = Color::red();*/


	FIRE = false;
}

AudioAnalyzer::~AudioAnalyzer()
{
	record = false;
	recordThread->join();

	calc = false;
	calcThread->join();
}

void AudioAnalyzer::render(ID2D1HwndRenderTarget* RenderTarget)
{
	int start = clock();
	RenderTarget->DrawLine({ plotX, plotY }, { plotX, plotY + plotYSize }, Color::black());
	float xUnitAmount = (axisXEnd - axisXStart) / axisXUnit;
	float xUnitSpace = plotXSize * axisXUnit / (axisXEnd - axisXStart);
	for (int i = 0; i <= xUnitAmount; i++)
	{
		RenderTarget->DrawLine({ plotX + xUnitSpace*i, plotY + plotYSize - axisUnitSize }, { plotX + xUnitSpace*i, plotY + plotYSize + axisUnitSize }, Color::black());

		Writer::print(to_string((int)(axisXStart + i*axisXUnit)), Color::black(), Writer::normal(), { plotX + xUnitSpace*i - 10, plotY + plotYSize + axisUnitSize, plotX + xUnitSpace*i + 50,  plotY + plotYSize + axisUnitSize + 50 });
	}

	RenderTarget->DrawLine({ plotX, plotY + plotYSize }, { plotX + plotXSize, plotY + plotYSize }, Color::black());
	float yUnitAmount = (axisYEnd - axisYStart) / axisYUnit;
	float yUnitSpace = plotYSize * axisYUnit / (axisYEnd - axisYStart);
	for (int i = 0; i <= yUnitAmount; i++)
	{
		RenderTarget->DrawLine({ plotX - axisUnitSize, plotY + plotYSize - yUnitSpace*i }, { plotX + axisUnitSize, plotY + plotYSize  - yUnitSpace*i }, Color::black());

		Writer::print(to_string(((int)((axisYStart + i*axisYUnit)*10))), Color::black(), Writer::normal(), { plotX - 50, plotY + plotYSize - yUnitSpace*i - 50, plotX, plotY + plotYSize - yUnitSpace*i + 50});
	}

	Writer::print("Plot Time: " + to_string(plotTime), Color::black(), Writer::normal(), { 120,20,400,60 });
	Writer::print("Calc Time: " + to_string(calcTime), Color::black(), Writer::normal(), { 120,100,400,60 });
	Writer::print("Capture Time: " + to_string(captureTime), Color::black(), Writer::normal(), { 120,180,400,60 });

	float xResolution = plotXSize / (axisXEnd - axisXStart);
	float yResolution = plotYSize / (axisYEnd - axisYStart);
	float time = clock() / 1000.0f;

	RenderTarget->DrawLine({ plotX + xResolution*(time - axisXStart) , plotY }, { plotX + xResolution*(time - axisXStart) , plotY + plotYSize }, Color::red());
	//RenderTarget->DrawLine({ plotX + xResolution*(time - 1 - axisXStart) , plotY }, { plotX + xResolution*(time - 1 - axisXStart) , plotY + plotYSize }, Color::red());

	for (auto& s : series)
	{
		if (s.second.plot)
		{
			std::lock_guard<std::mutex> guard(s.second.m);

			auto& data = s.second.data;
			if (s.second.type == 'l')
			{
				if (data.size() >= 2)
				{
					for (auto it = data.begin(); it != data.end()-1; ++it)
					{
						RenderTarget->DrawLine({ plotX + xResolution*(it->first - axisXStart) , plotY + plotYSize - yResolution*(it->second + s.second.offset - axisYStart) },
						{ plotX + xResolution*((it + 1)->first - axisXStart) , plotY + plotYSize - yResolution*((it + 1)->second + s.second.offset - axisYStart) }, s.second.color);
					}
				}
			}
			else if (s.second.type == 'd')
			{
				if (data.size() >= 1)
				{
					for (auto it = data.begin(); it != data.end(); ++it)
					{
						D2D1_ELLIPSE ellipse = D2D1::Ellipse({ plotX + xResolution*(it->first - axisXStart) , plotY + plotYSize - yResolution*(it->second + s.second.offset - axisYStart) }, 5, 5);
						RenderTarget->DrawEllipse(ellipse, s.second.color);
					}
				}
			}
		}
	}

	if (FIRE)
	{
		RenderTarget->FillRectangle({ 0,0,1000,50 }, Color::green());
	}

	plotTime = clock() - start;
}

void AudioAnalyzer::update()
{
	if (!recordThread)
	{
		recordThread = std::make_unique<std::thread>(&AudioAnalyzer::startRecording, this);
	}

	if (!calcThread)
	{
		calcThread = std::make_unique<std::thread>(&AudioAnalyzer::startCalc, this);
	}

	axisXStart = max(clock()/1000.0f - 9.0f, 0.0f);
	axisXEnd = max(clock()/1000.0f + 1.0, 10.0f);

	Sleep(1);
}

void AudioAnalyzer::ViewProc(App*app, HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (message == WM_MOUSEMOVE)
	{
		POINT pt = { LOWORD(lParam), HIWORD(lParam) };
		mouseX = pt.x - place.left;
		mouseY = pt.y - place.top;
	}
	if (message == WM_LBUTTONDOWN)
	{
		POINT pt = { LOWORD(lParam), HIWORD(lParam) };
		pt.x = (pt.x - place.left);
		pt.y = (pt.y - place.top);
		
	}
	if (message == WM_KEYDOWN)
	{
		if (wParam == 'Q')
		{
			
		}
	}
}

void AudioAnalyzer::startRecording()
{
	record = true;

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

	CoCreateInstance(CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL, IID_IMMDeviceEnumerator, (void**)&pEnumerator);

	pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pDevice);

	pDevice->Activate(IID_IAudioClient, CLSCTX_ALL, NULL, (void**)&pAudioClient);

	pAudioClient->GetMixFormat(&pwfx);

	pAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_LOOPBACK, hnsRequestedDuration, 0, pwfx, NULL);

	pAudioClient->GetBufferSize(&bufferFrameCount);

	pAudioClient->GetService(IID_IAudioCaptureClient, (void**)&pCaptureClient);

	float* compressed = new float[bufferFrameCount];

	pAudioClient->Start();

	fftwLow.init(pwfx->nSamplesPerSec / 40);
	fftwBeat.init(600);

	recordReady = true;

	while (record)
	{
		pCaptureClient->GetNextPacketSize(&packetLength);

		if (packetLength > 0)
		{
			int start = clock();

			pCaptureClient->GetBuffer(&pData, &numFramesAvailable, &flags, NULL, NULL);

			INT32* ptr = (INT32*)pData;

			for (int i = 0; i < numFramesAvailable; i++)
			{
				compressed[i] = ((float)ptr[i * 2] + (float)ptr[i * 2 + 1]) / (2.0f * MAXINT32);
			}

			raw.enqueue(compressed, numFramesAvailable, (float)clock() / 1000.0);

			pCaptureClient->ReleaseBuffer(numFramesAvailable);

			captureTime = clock() - start;
		}
		Sleep(10);
	}

	pAudioClient->Stop();

	delete[] compressed;

	CoTaskMemFree(pwfx);
	pEnumerator->Release();
	pDevice->Release();
	pAudioClient->Release();
	pCaptureClient->Release();
}

void AudioAnalyzer::startCalc()
{
	calc = true;
	WindowFunction wf;

	auto getLow = [&series = series, &fftw = fftwLow](TimeQueue* tq)
	{
		int s = tq->getSizeUnsave();
		if (s >= fftw.inputSize)
		{
			float time = 0;

			for (int i = 0; i < fftw.inputSize; i++)
			{
				int pos = s - fftw.inputSize + i;
				fftw.inputBuffer[i] = (*tq)[pos]->value; // *fftw.windowBuffer[i];
				time += (*tq)[pos]->time;
			}
			time /= fftw.inputSize;

			fftw_execute(fftw.plan);

			float sum = 0;

			for (int i = 0; i < LOW_FREQ; i++)
			{
				int offset = 0;
				float value = sqrt(fftw.outputBuffer[i + offset][0] * fftw.outputBuffer[i + offset][0] + fftw.outputBuffer[i + offset][1] * fftw.outputBuffer[i + offset][1]) / 5000.0;
				sum += value;
				series[SERIES::low + i].add(time, value);
			}

			series[SERIES::lowSum].add(time, sum);
		}
	};

	auto proccesLow = [&wf, &series = series]()
	{
		std::lock_guard<std::mutex> guard(series[SERIES::lowSum].m);

		auto& lowSum = series[SERIES::lowSum].data;

		int lengthLong = 200;
		int lengthShort = lengthLong / 20;

		if (lowSum.size() >= lengthLong)
		{
			float shortMean = 0;
			float shortTime = 0;
			float longMean = 0;
			float longTime = 0;
			for (int i = 0; i < lengthLong; i++)
			{
				longMean += lowSum[lowSum.size() - 1 - i].second;// *wf.getHann(lengthLong, i);
				longTime += lowSum[lowSum.size() - 1 - i].first;
				if (i < lengthShort)
				{
					shortMean += lowSum[lowSum.size() - 1 - i].second;// *wf.getHann(lengthShort, i);
					shortTime += lowSum[lowSum.size() - 1 - i].first;
				}
			}
			longMean /= lengthLong;
			shortMean /= lengthShort;
			shortTime /= lengthShort;
			longTime /= lengthLong;

			series[SERIES::lowMeanShort].add(shortTime, shortMean);
			series[SERIES::lowMeanLong].add(shortTime, longMean);

			float longStd = 0;
			for (int i = 0; i < lengthLong; i++)
			{
				float diff = lowSum[lowSum.size() - 1 - i].second - longMean;
				longStd += diff*diff;
			}
			longStd = sqrt(longStd / lengthLong);

			series[SERIES::lowStdShortHigh].add(shortTime, longMean + longStd*0.5);
			//series[SERIES::lowStdLongLow].add(longTime, longSum - longStd*0.5);

			if (longStd > 0.02 && longMean + longStd*0.5 < shortMean)
			{
				series[SERIES::meanBeat].add(shortTime, 0.5);
			}
			else
			{
				series[SERIES::meanBeat].add(shortTime, 0);
			}
		}
	};
	auto correctJumps = [](TimeVector& tv, int correctLength)
	{
		std::lock_guard<std::mutex> guard(tv.m);

		correctLength = min(correctLength, tv.data.size());

		if (tv.data.size() > 2)
		{
			float last = tv.data.back().second;

			bool found = false;
			int upto;
			for (int i = 1; i < correctLength; i++)
			{
				if ((tv.data.end() - 1 - i)->second == last)
				{
					found = true;
					upto = i;
				}
			}
			if (found)
			{
				for (int i = 1; i <= upto; i++)
				{
					(tv.data.end() - 1 - i)->second = last;
				}
			}
		}
	};
	auto getRise = [](TimeVector& in, TimeVector& out, int correctLength)
	{
		std::lock_guard<std::mutex> guard(in.m);

		if (in.data.size() > correctLength + 3)
		{
			float p1 = (in.data.end() - correctLength - 3)->second;
			float p2 = (in.data.end() - correctLength - 2)->second;

			if (p1 < p2)
			{
				out.add((in.data.end() - correctLength - 2)->first, (in.data.end() - correctLength - 2)->second);
			}
		}
	};
	auto predict = [](TimeVector& in, TimeVector& out, int amount)
	{
		std::lock_guard<std::mutex> guard(in.m);

		if (in.data.size() > amount)
		{
			float mean = 0;
			for (int i = 1; i < amount; i++)
			{
				mean += (in.data.end() - i)->first - (in.data.end() - i - 1)->first;
			}
			mean /= amount-1;

			float std = 0;
			for (int i = 1; i < amount; i++)
			{
				float diff = (in.data.end() - i)->first - (in.data.end() - i - 1)->first - mean;
				std += diff * diff;
			}
			std = sqrt(std / (amount - 1));

			float lastIn = in.data.back().first;
			float now = clock() / 1000.0;
			if (mean < 1 && std < 0.1 && now - lastIn < 1)
			{		
				float lastOut = 0;
				if (out.data.size() > 0) // Thread save??
				{
					lastOut = out.data.back().first;
				}

				float next = lastIn;
				while (next < now)
				{
					next += mean;
					if (next > lastOut + mean/2.0)
					{
						out.add(next, 0.5);
					}
				}
			}

		}
	};
	auto calcFire = [&FIRE = FIRE](TimeVector& in, int amount)
	{
		std::lock_guard<std::mutex> guard(in.m);

		if (in.data.size() > amount)
		{
			FIRE = false;
			for (int i = 0; i < amount; i++)
			{
				if ((in.data.end() - 1 - i)->second > 0.25)
				{
					FIRE = true;
				}
			}
		}
	};
	/*
	auto getEnergy = [&energy = energy](TimeQueue* tq)
	{
		int s = tq->getSizeUnsave();
		int length = 4800;
		if (s >= 4800)
		{
			float sum = 0.0;
			float time = 0;
			for (int i = 1; i < length; i++)
			{
				float diff = (*tq)[-i]->value - (*tq)[-i - 1]->value;
				sum += diff*diff;
				time += (*tq)[-i]->time;
			}

			sum /= length;
			sum += 0.5;

			time /= length;

			energy.enqueue(&sum, 1, time);
		}
	};

	

	auto getLowMeanStd = [&lowMean = lowMean, &lowStd = lowStd, &lowBeat = lowBeat](TimeQueue* tq)
	{
		int s = tq->getSizeUnsave();
		float steps = 50;
		float seconds = 0.5;
		float end = (*tq)[s - 1]->time;
		float start = end - seconds;
		if ((*tq)[0]->time < start)
		{
			float stepSize = (end - start) / steps;
			float time = (start + end) / 2.0;
			float mean = 0;
			float std = 0;

			int timePosHelper = 1;
			for (int i = 0; i < steps; i++)
			{
				mean += tq->getAt(start + i*stepSize, timePosHelper);
			}
			mean /= steps;

			timePosHelper = 1;
			for (int i = 0; i < steps; i++)
			{
				float diff = mean - tq->getAt(start + i*stepSize, timePosHelper);
				std += diff * diff;
			}
			std = sqrt(std / steps);

			lowMean.enqueue(&mean, 1, time);
			lowStd.enqueue(&std, 1, time);

			float last = tq->getAt(time);
			if (last > mean && std > 0.005)
			{
				float v = 0.3;
				lowBeat.enqueue(&v, 1, time);
			}
			else
			{
				float v = 0;
				lowBeat.enqueue(&v, 1, time);
			}
			int correctLength = 6;
			bool found = false;
			int upto = -1;
			for (int i = 2; i <= correctLength; i++)
			{
				if (lowBeat[-i]->value == lowBeat[-1]->value)
				{
					found = true;
					upto = i;
				}
			}
			if (found)
			{
				for (int i = 2; i <= upto; i++)
				{
					lowBeat[-i]->value = lowBeat[-1]->value;
				}
			}
		}
	};

	auto getBeat = [&fftw = fftwBeat, &beatPhase = beatPhase, &beatAmplitude = beatAmplitude, &lowStd = lowStd](TimeQueue* tq)
	{
		int s = tq->getSizeUnsave();
		float end = (*tq)[s - 1]->time;
		float start = end - fftw.inputSize / 1000.0;
		if ((*tq)[0]->time < start)
		{
			float time = (start + end) / 2.0;
			float std = lowStd.getAt(time);
			float phase;

			float stepSize = (end - start) / fftw.inputSize;

			int timePosHelper = 1;
			
			float min = 999;
			float max = 0;
			for (int i = 0; i < fftw.inputSize; i++)
			{
				fftw.inputBuffer[i] = tq->getAt(start + i*stepSize, timePosHelper);//* fftw.windowBuffer[i];
				if (fftw.inputBuffer[i] < min)
				{
					min = fftw.inputBuffer[i];
				}
				if (fftw.inputBuffer[i] > max)
				{
					max = fftw.inputBuffer[i];
				}
			}
			float mid = (max - min) / 2.0;

			for (int i = 0; i < fftw.inputSize; i++)
			{
				fftw.inputBuffer[i] -= mid;
			}

			fftw_execute(fftw.plan);

			int n = 1;
			phase = atan2(fftw.outputBuffer[n][0], fftw.outputBuffer[n][1]) / 20 + 0.5;
			beatPhase.enqueue(&phase, 1, time);

			//float amplitude = sqrt(fftw.outputBuffer[n][0] * fftw.outputBuffer[n][0] + fftw.outputBuffer[n][1] * fftw.outputBuffer[n][1]) / 200 + 1;
			//beatAmplitude.enqueue(&amplitude, 1, time);
		}
	};

	int doubleBeat = 0;
	auto getBeatPoint = [&doubleBeat, &beatPoint = beatPoint](TimeQueue* tq)
	{
		int s = tq->getSizeUnsave();
		int meanSteps = 4;
		if (s >= meanSteps+1)
		{
			if ((*tq)[-2]->value < 0.5 && (*tq)[-1]->value > 0.5)
			{
				float h = 0.8;

				float newTime = (*tq)[-1]->time;
				if (beatPoint.getSizeUnsave()==0)
				{
					beatPoint.enqueue(&h, 1, newTime);
				}
				else
				{
					float diff = newTime - beatPoint[-1]->time;
					if (diff > 3)
					{
						beatPoint.enqueue(&h, 1, newTime);
					}
					else
					{
						if (beatPoint.getSizeUnsave() <= meanSteps)
						{
							beatPoint.enqueue(&h, 1, newTime);
						}
						else
						{
							float mean = 0;
							for (int i = 0; i < meanSteps; i++)
							{
								mean += beatPoint[-i - 1]->time - beatPoint[-i - 2]->time;
							}
							mean /= meanSteps;

							float std = 0;
							for (int i = 0; i < meanSteps; i++)
							{
								float d = beatPoint[-i - 1]->time - beatPoint[-i - 2]->time - mean;
								std += d * d;
							}
							std = max(sqrt(std / meanSteps), 0.05);

							float min = 1;
							float minIndex=0;
							for (int i = 0; i < 4; i++)
							{
								float error = abs(diff - i*mean);
								float p = normalCDF(error / std);
								if (p < min)
								{
									min = p;
									minIndex = i;
								}
							}
							if (minIndex == 2)
							{
								doubleBeat++;
							}
							else
							{
								doubleBeat = 0;
							}
							if (doubleBeat == 3)
							{
								minIndex = 1;
							}
							if (minIndex > 0)
							{
								float step = diff / minIndex;
								float start = beatPoint[-1]->time;
								for (int i = 1; i <= minIndex; i++)
								{
									beatPoint.enqueue(&h, 1, start + i*step);
								}
							}
						}
					}
				}
			}
		}
	};

	auto getBeatPointPredicted = [&beatPointPredicted = beatPointPredicted](TimeQueue* tq)
	{
		int s = tq->getSizeUnsave();
		float time = clock() / 1000.0;
		int frameSize = 4;
		if (s >= frameSize+1)
		{
			float first = (*tq)[-frameSize]->time;
			if (first + frameSize*0.75 > time)
			{
				float avgDistance = 0;
				for (int i = 0; i < frameSize; i++)
				{
					avgDistance += (*tq)[-i-1]->time - (*tq)[-i - 2]->time;
				}
				avgDistance /= frameSize;

				float value = 0.9;
				float next = (*tq)[-1]->time;

				while (next < time + 0.2)
				{
					if (beatPointPredicted.getSizeUnsave() > 0)
					{
						if (next > beatPointPredicted[-1]->time + avgDistance/2.0)
						{
							beatPointPredicted.enqueue(&value, 1, next);
						}
					}
					else
					{
						beatPointPredicted.enqueue(&value, 1, next);
					}
					next += avgDistance;
				}
			}
		}
	};

	auto fire = [&FIRE = FIRE](TimeQueue* tq)
	{
		float now = clock() / 1000.0;
		FIRE = false;
		for (int i = 0; i < min(5, tq->getSizeUnsave()); i++)
		{
			if (abs((*tq)[-i - 1]->time+0.1 - now) <= 0.1)
			{
				FIRE = true;
			}
		}
	};
	*/
	while (calc)
	{
		if (recordReady)
		{
			int start = clock();

			raw.apply(getLow);

			proccesLow();

			calcFire(series[SERIES::meanBeat], 20);

			//correctJumps(series[SERIES::meanBeat], 6);
			//getRise(series[SERIES::meanBeat], series[SERIES::meanBeatPoint], 6);
			//predict(series[SERIES::meanBeatPoint], series[SERIES::predictedBeat], 6);

			for (auto& s : series)
			{
				s.second.removeTil((float)clock() / 1000.0 - 9.0);
			}

			calcTime = clock() - start;
		}
		Sleep(1);
	}
}
