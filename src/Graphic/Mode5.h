#pragma once

#include "Plain.h"

class Mode5 :public Plain
{
public:
	Mode5(const Comment &comment);
	QList<QRectF> locate();
	bool move(qint64 time);
	uint intersects(Graphic *other);

private:
	double life;
};