#pragma once

#include "Plain.h"

class Mode1 :public Plain
{
public:
	Mode1(const Comment &comment);
	QList<QRectF> locate();
	bool move(qint64 time);
	uint intersects(Graphic *other);

private:
	double speed;
};
