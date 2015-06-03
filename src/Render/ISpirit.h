#pragma once

#include <QRectF>
#include <QPainter>

class ISpirit
{
public:
	virtual void draw(QPainter *, QRectF) = 0;
	virtual ~ISpirit() = default;
};
