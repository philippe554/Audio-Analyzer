#pragma once

#include <cstdint>
#include <mutex>
#include <algorithm>

class TimeQueue;

struct Point
{
	float value;
	float time;
};

class TimeQueue
{
public:
	TimeQueue();
	TimeQueue(int _size);
	TimeQueue(const TimeQueue& other) = delete;
	~TimeQueue();

	bool enqueue(float* buffer, int length, float t);

	void apply(std::function<void(TimeQueue*)> f);

	bool getLatest(Point* buffer, int length);

	void removeUntil(float t);

	int getSize();
	int getSizeUnsave();

	float getAt(float t, int& help);
	float getAt(float t);
	Point* operator[](int i);

private:
	Point* data;

	int size;
	int head; //Including
	int tail; //Excluding

	std::mutex m;
};