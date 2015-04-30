#pragma once

#include "ARender.h"

class RasterRenderPrivate;

class RasterRender :public ARender
{
public:
	explicit RasterRender(QObject *parent = 0);

private:
	Q_DECLARE_PRIVATE(RasterRender);

public slots:
	ICache *getCache(const QImage &i);
	quintptr getHandle();
	void resize(QSize size);
	QSize getActualSize();
	QSize getBufferSize();
	void draw(QRect rect = QRect());
};
