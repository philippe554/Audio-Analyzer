#include <Windows.h>

#include <mutex>
#include <iostream>

class AudioQueue
{
public:
	AudioQueue(int _size)
	{
		size = _size;
		data = new INT32[size];
		memset(data, 0, size);
		head = 0;
		tail = 0;
	}
	~AudioQueue()
	{
		delete[] data;
	}
	void enqueue(INT32* newData, int length)
	{
		std::lock_guard<std::mutex> guard(m);
		if (tail + length < size)
		{
			memcpy(data + tail, newData, length * 4);

			if (tail < head && head < tail + length)
			{
				std::cout << "Data messed up" << std::endl;
			}

			tail += length;
		}
		else
		{
			int p1 = size - tail;
			int p2 = length - p1;
			memcpy(data + tail, newData, p1 * 4);
			memcpy(data, newData + p1, p2 * 4);

			if (tail < head || head < p2)
			{
				std::cout << "Data messed up" << std::endl;
			}

			tail = p2;
		}
	}
	bool dequeue(INT32* buffer, int length, int reduction = -1)
	{
		if (reduction == -1)
		{
			reduction = length;
		}

		if ((head + length)%size > tail)
		{
			return false;
		}

		std::lock_guard<std::mutex> guard(m);
		if (head + length < size)
		{
			memcpy(buffer, data + head, length * 4);
			head += reduction;
		}
		else
		{
			int p1 = size - head;
			int p2 = length - p1;
			memcpy(buffer, data + head, p1 * 4);
			memcpy(buffer + p1, data, p2 * 4);
			head += reduction;
			head = head%size;
		}
		return true;
	}
	void print(int length)
	{
		for (int i = 0; i < length; i++)
		{
			std::cout << data[i] / 10000000 << " ";
		}
		std::cout << std::endl;
	}
	//private:
	int head;
	int tail;
	int size;
	INT32* data;

	std::mutex m;
};