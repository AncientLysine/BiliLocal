#pragma once

#include "Plain.h"

class Mode4 :public Plain
{
public:
	Mode4(const Comment &comment);
	QList<QRectF> locate();
	bool move(double time);
	uint intersects(Graphic *other);

private:
	double life;
};
