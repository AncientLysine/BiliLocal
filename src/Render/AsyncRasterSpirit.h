#pragma once

#include "ISpirit.h"
#include <QImage>
#include <QPainter>

class AsyncRasterSpirit :public ISpirit
{
public:
	QImage image;

	explicit AsyncRasterSpirit(const QImage &image) :
		image(image)
	{
	}

	void draw(QPainter *painter, QRectF dest)
	{
		painter->drawImage(dest, image);
	}
};
