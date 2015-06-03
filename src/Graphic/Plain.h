#pragma once

#include "Graphic.h"
#include "../Render/ARender.h"
#include "../Render/ISpirit.h"

class Plain :public Graphic
{
public:
	void draw(QPainter *painter);

protected:
	ISpirit *spirit;

	explicit Plain(const Comment &comment);
	virtual ~Plain();

	double evaluate(QString expression);
};
