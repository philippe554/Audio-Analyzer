#include <stdlib.h>
#include <iostream>
#include <math.h>
#include <fftw3.h>
#include <thread>
#include "Recorder.h"

#define M_PI 3.14159265359
#define N 16

float history[100];
int place;

void run(double* input, int inputSize, double time)
{
	static bool cutoff = true;
	static int beatsFound = 0;

	int outputSize = inputSize / 2 + 1;
	fftw_complex* outputBuffer = fftw_alloc_complex(outputSize * sizeof(fftw_complex));

	fftw_plan plan = fftw_plan_dft_r2c_1d(inputSize, input, outputBuffer, FFTW_ESTIMATE);
	fftw_execute(plan);

	float maxA = 0;

	for (int i = 40*time; i < 80*time; i++)
	{
		float a = sqrt(outputBuffer[i][0] * outputBuffer[i][0] + outputBuffer[i][1] * outputBuffer[i][1]);
		if (a > maxA)
		{
			maxA = a;
		}
	}

	history[place++] = maxA;
	place = place % 100;

	float min = 10000;
	float max = 0;
	for (int i = 0; i < 100; i++)
	{
		if (history[i] > max)
		{
			max = history[i];
		}
		if (history[i] < min)
		{
			min = history[i];
		}
	}

	//int beat = 60 * time;
	//float a = sqrt(outputBuffer[beat][0] * outputBuffer[beat][0] + outputBuffer[beat][1] * outputBuffer[beat][1]);
	//float f = atan2(outputBuffer[beat][0], outputBuffer[beat][1]);

	//std::cout << a << std::endl;
	
	float diff = (max - min) / 4.0;

	if (cutoff)
	{
		if (maxA > max - diff)
		{
			std::cout << "Boem: " << beatsFound << " - " << beatsFound*1000*60/clock() << std::endl;
			beatsFound++;
			cutoff = false;
		}
	}
	else
	{
		if (maxA < max - 2*diff)
		{
			cutoff = true;
		}
	}

	fftw_free(outputBuffer);
	fftw_destroy_plan(plan);
}


int main(void)
{
	HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);

	place = 0;
	for (int i = 0; i < 100; i++)
	{
		history[i] = 0.0;
	}

	AudioQueue storage(10000000);

	bool finished = false;

	std::thread t1(RecordAudioStream, &storage, &finished);

	while(!finished)
	{
		INT32* intBuffer = new INT32[9600];
		if (storage.dequeue(intBuffer, 9600, 960))
		{
			double* doubleBuffer = fftw_alloc_real(4800 * sizeof(double));
			for (int j = 0; j < 4800; j++)
			{
				doubleBuffer[j] = ((double)intBuffer[j * 2] + (double)intBuffer[j * 2 + 1]) / MAXINT32;
			}
			run(doubleBuffer, 4800, 0.1);

			fftw_free(doubleBuffer);
		}
		delete[] intBuffer;
	}

	t1.join();

	fftw_cleanup();

	CoUninitialize();
	std::cin.get();
	return 0;
}