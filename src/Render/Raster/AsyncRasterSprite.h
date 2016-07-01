#pragma once

#include "../ASprite.h"
#include "../ARender.h"
#include <QImage>

class AsyncRasterSprite : public ASprite
{
public:
	virtual void prepare() override;
	virtual void draw(QPainter *painter) override;
	virtual QSize getSize() override;

private:
	QImage preImage;
	double preAlpha;
};
