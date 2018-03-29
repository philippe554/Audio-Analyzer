#include "View.h"

View::View(int x, int y,int xSize,int ySize)
{
	place = D2D1::RectF(x, y, x + xSize, y + ySize);
}

