#pragma once

#include "Graphic.h"
#include "../Render/ARender.h"

class Plain :public Graphic
{
public:
	void draw(QPainter *painter);

protected:
	ARender::ICache *cache;

	explicit Plain(const Comment &comment);
	~Plain(){ delete cache; }
};
