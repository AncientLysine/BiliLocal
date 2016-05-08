#pragma once

#include "Graphic.h"

class ASprite;

class Mode7 :public Graphic
{
public:
	explicit Mode7(const Comment &comment);
	virtual ~Mode7();
	QList<QRectF> locate(){ return QList<QRectF>(); }
	bool move(double time);
	void draw(QPainter *painter);
	uint intersects(Graphic *){ return 0; }

private:
	QPointF bPos;
	QPointF ePos;
	double bAlpha;
	double eAlpha;
	double zRotate;
	double yRotate;
	ASprite *sprite;
	double wait;
	double stay;
	double life;
	double time;
};
