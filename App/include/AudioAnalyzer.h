#pragma once
#include <deque>
#include <tuple>
#include <thread>
#include <memory>
#include <mutex>
#include <algorithm>
#include <numeric>
#include <fftw3.h>
#include "TimeQueue.h"

#include "Color.h"

#define PI 3.14159265359

class AudioAnalyzer;

#include "View.h"

class FFTW
{
public:
	FFTW()
	{
		set = false;
	}
	~FFTW()
	{
		if (set)
		{
			delete[] inputBuffer;
			delete[] windowBuffer;
			fftw_free(outputBuffer);
			fftw_destroy_plan(plan);
		}
	}
	void init(int _inputSize)
	{
		if(!set)
		{
			inputSize = _inputSize;
			outputSize = inputSize / 2 + 1;
			inputBuffer = new double[inputSize];
			outputBuffer = fftw_alloc_complex(outputSize * sizeof(fftw_complex));
			plan = fftw_plan_dft_r2c_1d(inputSize, inputBuffer, outputBuffer, FFTW_ESTIMATE);

			windowBuffer = new double[inputSize];
			for (int i = 0; i < inputSize; i++)
			{
				windowBuffer[i] = 0.5 * (1 - cos((2 * PI * i) / (inputSize - 1)));
			}

			set = true;
		}
	}
	bool set;
	int inputSize;
	int outputSize;
	double* inputBuffer;
	double* windowBuffer;
	fftw_complex* outputBuffer;
	fftw_plan plan;
};

const int LOW_FREQ = 3;//6;

enum SERIES
{
	raw, 
	low, lowEnd = low + LOW_FREQ, lowSum,
	lowMeanShort, lowStdShortHigh, lowMeanLong, lowStdLongHigh, lowStdLongLow,
	meanBeat, meanBeatPoint,
	predictedBeat,
	END
};

class TimeVector
{
public:
	TimeVector()
	{
		type = 'l';
		plot = false;
		offset = 0;
		color = Color::black();
	}

	void add(float t, float d)
	{
		std::lock_guard<std::mutex> guard(m);

		data.push_back(std::make_pair(t,d));
	}

	void removeTil(float t)
	{
		std::lock_guard<std::mutex> guard(m);

		if (data.size() > 0)
		{
			auto until = std::find_if(data.begin(), data.end() - 1, [t](std::pair<float, float> d) { return d.first >= t; });

			if (until - data.begin() > 1)
			{
				data.erase(data.begin(), until);
			}
		}
	}

	float getAt(float t)
	{
		std::lock_guard<std::mutex> guard(m);

		if (data.back().first < t)
		{
			return data.back().second;
		}
		if (data.front().first > t)
		{
			return data.front().second;
		}

		for (int i = 1; i < data.size(); i++)
		{
			if ((data.end() - 1 - i)->first < t)
			{
				float t1 = (data.end() - 1 - i)->first;
				float t2 = (data.end() - i)->first;

				if (t < t1 || t2 < t)
				{
					throw "Error";
				}

				float d1 = (data.end() - 1 - i)->second;
				float d2 = (data.end() - i)->second;

				float td = t2 - t1;

				return ((t2 - t) * d1 + (t - t1) * d1) / td;
			}
		}
	}

	std::vector<std::pair<float,float>> data;
	std::mutex m;

	char type;
	bool plot;
	float offset;
	ID2D1SolidColorBrush* color;
};

class WindowFunction
{
public:
	float getHann(int size, int loc)
	{
		if (hann.count(size) == 0)
		{
			vector<float> data(size);

			std::generate(data.begin(), data.end(), [size, i = 0]() mutable {return 0.5 * (1 - cos((2 * PI * i++) / (size - 1))); });
			float sum = std::accumulate(data.begin(), data.end(), 0.0);
			std::transform(data.begin(), data.end(), data.begin(), [sum](float e) { return e / sum; });

			hann.insert(std::make_pair(size, data));
		}

		return hann.at(size)[loc];
	}
	std::map<int, std::vector<float>> hann;
};

class AudioAnalyzer : public View
{
public:
	AudioAnalyzer(int x, int y, int xSize, int ySize);
	~AudioAnalyzer();

	void render(ID2D1HwndRenderTarget* RenderTarget) override;
	void update() override;
	void ViewProc(App* app, HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) override;

private:
	void startRecording();
	bool record;
	bool recordReady;
	std::unique_ptr<std::thread> recordThread;

	void startCalc();
	bool calc;
	std::unique_ptr<std::thread> calcThread;

	int mouseX;
	int mouseY;

	float counter = 0;

	float plotX;
	float plotY;
	float plotXSize;
	float plotYSize;

	float axisXStart;
	float axisYStart;
	float axisXEnd;
	float axisYEnd;

	float axisXUnit;
	float axisYUnit;
	float axisUnitSize;

	int plotTime;
	int calcTime;
	int captureTime;

	std::map<int, TimeVector> series;

	TimeQueue raw;
	FFTW fftwLow;

	TimeQueue low;

	TimeQueue lowMean;
	TimeQueue lowStd;
	TimeQueue lowBeat;

	FFTW fftwBeat;
	TimeQueue beatPhase;
	TimeQueue beatAmplitude;

	TimeQueue beatMean;
	TimeQueue beatStd;

	TimeQueue beatPoint;
	TimeQueue beatPointPredicted;

	TimeQueue energy;

	bool FIRE;
};