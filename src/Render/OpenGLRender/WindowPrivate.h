#pragma once

#include "OpaquePrivate.h"
#include "../../Local.h"
#include <QAbstractScrollArea>
#include <QOpenGLWindow>
#include <functional>

namespace
{
	class OWindow :public QOpenGLWindow
	{
	public:
		explicit OWindow(OpenGLOpaqueRenderPrivate *render) :
			render(render)
		{
			window = nullptr;
			QTimer::singleShot(0, [this](){
				window = lApp->mainWidget()->backingStore()->window();
			});
			connect(this, &OWindow::frameSwapped, std::bind(&OpenGLRenderPrivate::onSwapped, render));
		}

	private:
		QWindow *window;
		OpenGLOpaqueRenderPrivate *const render;

		void initializeGL()
		{
			render->initialize();
		}

		void paintGL()
		{
			render->paint(this);
		}

		bool event(QEvent *e)
		{
			switch (e->type()){
			case QEvent::KeyPress:
			case QEvent::KeyRelease:
			case QEvent::Enter:
			case QEvent::Leave:
			case QEvent::MouseMove:
			case QEvent::MouseButtonPress:
			case QEvent::MouseButtonRelease:
			case QEvent::MouseButtonDblClick:
			case QEvent::Wheel:
			case QEvent::DragEnter:
			case QEvent::DragMove:
			case QEvent::DragLeave:
			case QEvent::Drop:
			case QEvent::ContextMenu:
				return window ? qApp->sendEvent(window, e) : false;
			default:
				return QOpenGLWindow::event(e);
			}
		}

	};

	/*	only QMdiSubWindow & QAbstractScrollArea
	will make windowcontainer to create native widgets.
	*/
	class FParent :public QAbstractScrollArea
	{
	public:
		explicit FParent(QWidget *parent) :
			QAbstractScrollArea(parent)
		{
		}
	};
}

class OpenGLWindowRenderPrivate :public OpenGLOpaqueRenderPrivate
{
public:
	OpenGLWindowRenderPrivate();

	virtual bool isVisible() override
	{
		return widget->isVisible();
	}

	virtual quintptr getHandle() override
	{
		return (quintptr)window;
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
	FParent *middle;
	OWindow *window;
};
