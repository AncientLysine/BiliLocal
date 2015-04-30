#pragma once

#include "ARender.h"

class OpenGLRenderPrivate;

class OpenGLRender :public ARender
{
public:
	explicit OpenGLRender(QObject *parent = 0);

private:
	Q_DECLARE_PRIVATE(OpenGLRender);

public slots:
	ICache *getCache(const QImage &i);
	quintptr getHandle();
	void resize(QSize size);
	QSize getActualSize();
	QSize getBufferSize();
	void draw(QRect rect = QRect());
};
