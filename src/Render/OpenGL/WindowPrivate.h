#pragma once

#include "OpaquePrivate.h"
#include "../../Local.h"
#include <QWidget>
#include <QOpenGLWindow>

class OpenGLWindowRenderPrivate :public OpenGLOpaqueRenderPrivate
{
public:
	OpenGLWindowRenderPrivate();

	virtual bool isVisible() override
	{
		return widget->isVisible();
	}

	virtual QObject *getHandle() override
	{
		return window;
	}

	virtual void resize(QSize size) override
	{
		middle->resize(size);
		widget->resize(size);
	}

	virtual QSize getActualSize() override
	{
		return widget->size();
	}

	virtual void draw(QRect rect) override
	{
		window->update(rect);
	}

private:
	QWidget *widget;
	QWidget *middle;
	QOpenGLWindow *window;
};
