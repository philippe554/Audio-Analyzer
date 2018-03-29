#include <TimeQueue.h>

TimeQueue::TimeQueue()
{
	size = 2000;
	head = 0;
	tail = 0;

	data = new Point[size];
}

TimeQueue::TimeQueue(int _size)
{
	size = _size;
	head = 0;
	tail = 0;

	data = new Point[size];
}

TimeQueue::~TimeQueue()
{
	delete[] data;
}

bool TimeQueue::enqueue(float* buffer, int length, float t)
{
	std::lock_guard<std::mutex> guard(m);

	if (tail + length < size)
	{
		if (tail < head && head < tail + length)
		{
			return false;
		}
		for (int i = 0; i < length; i++)
		{
			data[tail + i].value = buffer[i];
			data[tail + i].time = t;
		}
		tail = tail + length;
	}
	else
	{
		int p1 = size - tail;
		int p2 = length - p1;
		if (tail <= head && head < size)
		{
			return false;
		}
		if (0 <= head && head < p2)
		{
			return false;
		}
		for (int i = 0; i < p1; i++)
		{
			data[tail + i].value = buffer[i];
			data[tail + i].time = t;
		}
		for (int i = 0; i < p2; i++)
		{
			data[i].value = buffer[p1 + i];
			data[i].time = t;
		}
		tail = p2;
	}
	return true;
}

void TimeQueue::apply(std::function<void(TimeQueue*)> f)
{
	std::lock_guard<std::mutex> guard(m);

	f(this);
}

bool TimeQueue::getLatest(Point* buffer, int length)
{
	std::lock_guard<std::mutex> guard(m);

	if (length < getSizeUnsave())
	{
		return false;
	}

	for (int i = 0; i < length; i++)
	{
		int pos = (tail - length + i + size) % size;
		buffer[i] = data[pos];
	}
	
	return true;
}

void TimeQueue::removeUntil(float t)
{
	std::lock_guard<std::mutex> guard(m);

	while (data[head].time < t && getSizeUnsave() > 0)
	{
		head++;
		if (head == size)
		{
			head = 0;
		}
	}
}

int TimeQueue::getSize()
{
	std::lock_guard<std::mutex> guard(m);

	return getSizeUnsave();
}

float TimeQueue::getAt(float t, int & help)
{
	while (data[(head + help) % size].time < t)
	{
		help++;
	}

	if (help >= getSizeUnsave())
	{
		help = getSizeUnsave() - 1;
	}

	float diff = data[(head + help) % size].time - data[(head + help - 1) % size].time;
	float d1 = (data[(head + help) % size].time - t) / diff;
	float d2 = (t - data[(head + help - 1) % size].time) / diff;

	return d1 * data[(head + help - 1) % size].value + d2 * data[(head + help) % size].value;
}

float TimeQueue::getAt(float t)
{
	int help = getSizeUnsave() - 2;

	while (data[(head + help) % size].time > t)
	{
		help--;
	}

	if (help < 0)
	{
		help = 0;
	}

	float diff = data[(head + help + 1) % size].time - data[(head + help) % size].time;
	float d1 = (data[(head + help + 1) % size].time - t) / diff;
	float d2 = (t - data[(head + help) % size].time) / diff;

	return d1 * data[(head + help) % size].value + d2 * data[(head + help + 1) % size].value;
}

Point * TimeQueue::operator[](int i)
{
	if (i < 0)
	{
		i = getSizeUnsave() + i;
	}
	return data + (head + i) % size;
}

int TimeQueue::getSizeUnsave()
{
	if (head <= tail)
	{
		return tail - head;
	}
	else
	{
		return (size - head) + tail ;
	}
}