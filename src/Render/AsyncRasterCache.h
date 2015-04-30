#pragma once

#include "ARender.h"

class AsyncRasterCache :public ARender::ICache
{
public:
	QImage i;

	explicit AsyncRasterCache(const QImage &i) :i(i){}

	void draw(QPainter *p, QRectF r)
	{
		p->drawImage(r, i);
	}
};
