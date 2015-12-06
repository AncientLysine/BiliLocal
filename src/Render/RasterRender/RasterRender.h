#pragma once

#include "../ARender.h"

class ISprite;
class RasterRenderPrivate;

class RasterRender :public ARender
{
public:
	explicit RasterRender(QObject *parent = 0);

private:
	Q_DECLARE_PRIVATE(RasterRender);

public slots:
	virtual ISprite *getSprite(const Comment &comment) override;
	virtual quintptr getHandle() override;
	virtual void resize(QSize size) override;
	virtual QSize getActualSize() override;
	virtual QSize getBufferSize() override;
	virtual void draw(QRect rect = QRect()) override;
};
