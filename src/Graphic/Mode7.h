#pragma once

#include "Graphic.h"

class Mode7 :public Graphic
{
public:
	Mode7(const Comment &comment);
	QList<QRectF> locate(){ return QList<QRectF>(); }
	bool move(qint64 time);
	void draw(QPainter *painter);
	uint intersects(Graphic *){ return 0; }

private:
	QPointF bPos;
	QPointF ePos;
	double bAlpha;
	double eAlpha;
	double zRotate;
	double yRotate;
	QImage cache;
	double wait;
	double stay;
	double life;
	double time;
};
