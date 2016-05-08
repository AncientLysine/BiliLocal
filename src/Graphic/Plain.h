#pragma once

#include "Graphic.h"
#include "../Render/ARender.h"
#include "../Render/ASprite.h"

class Plain :public Graphic
{
public:
	void draw(QPainter *painter);

protected:
	ASprite *sprite;

	explicit Plain(const Comment &comment);
	virtual ~Plain();

	double evaluate(QString expression);
};
