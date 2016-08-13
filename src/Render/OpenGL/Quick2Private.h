#pragma once

#include "OpaquePrivate.h"
#include <QtCore>
#include <QtGui>
#include <QtQuick>

class OpenGLQuick2RenderPrivate : public OpenGLOpaqueRenderPrivate
{
public:
	OpenGLQuick2RenderPrivate();

	virtual bool isVisible() override
	{
		return window->isVisible();
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

	virtual void draw(QRect) override
	{
		window->update();
	}

private:
	QPointer<QQuickWindow> window;
	QScopedPointer<QOpenGLPaintDevice> device;
	bool uninitialized;
};
