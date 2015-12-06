#pragma once

#include "Graphic.h"
#include "../Render/ARender.h"
#include "../Render/ISprite.h"

class Plain :public Graphic
{
public:
	void draw(QPainter *painter);

protected:
	ISprite *sprite;

	explicit Plain(const Comment &comment);
	virtual ~Plain();

	double evaluate(QString expression);
};
