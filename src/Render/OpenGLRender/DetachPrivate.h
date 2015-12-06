#pragma once

#include "OpaquePrivate.h"
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

	virtual QList<quint8 *> getBuffer() override
	{
		return QList<quint8 *>();
	}

	virtual void setBuffer(QString &chroma, QSize, int, QList<QSize> *) override
	{
		chroma = "NONE";
	}

	virtual void releaseBuffer() override
	{
	}

	virtual quintptr getHandle() override
	{
		return (quintptr)window;
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
