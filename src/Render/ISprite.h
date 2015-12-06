#pragma once

#include <QRectF>
#include <QPainter>

class ISprite
{
public:
	virtual void draw(QPainter *, QRectF) = 0;
	virtual QSize getSize() = 0;
	virtual ~ISprite() = default;
};
