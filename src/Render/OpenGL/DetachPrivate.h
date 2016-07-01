#pragma once

#include "OpaquePrivate.h"
#include "../PFormat.h"
#include "../../Local.h"
#include <QOpenGLWindow>

class OpenGLDetachRenderPrivate :public OpenGLRenderPrivate
{
public:
	OpenGLDetachRenderPrivate();

	virtual ~OpenGLDetachRenderPrivate()
	{
		delete window;
	}

	virtual bool isVisible() override
	{
		return window->isVisible();
	}

	virtual void drawData(QPainter *, QRect) override
	{
	}

	virtual void setFormat(PFormat *format) override
	{
		format->chroma = "NONE";
	}

	virtual void setBuffer(ABuffer *) override
	{
	}

	virtual QObject *getHandle() override
	{
		return window;
	}

	virtual void resize(QSize) override
	{
	}

	virtual QSize getActualSize() override
	{
		return window->size();
	}

	virtual QSize getBufferSize() override
	{
		return QSize();
	}

	virtual void draw(QRect rect) override
	{
		window->update(rect);
	}

private:
	QOpenGLWindow *window;
};
