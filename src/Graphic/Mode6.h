#pragma once

#include "Plain.h"

class Mode6 :public Plain
{
public:
	Mode6(const Comment &comment);
	QList<QRectF> locate();
	bool move(double time);
	uint intersects(Graphic *other);

private:
	double speed;
};
