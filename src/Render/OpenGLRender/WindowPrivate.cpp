#include "Common.h"
#include "WindowPrivate.h"
#include <QAbstractScrollArea>
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
			QTimer::singleShot(0, [this]() {
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
			switch (e->type()) {
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

OpenGLWindowRenderPrivate::OpenGLWindowRenderPrivate()
{
	middle = new FParent(lApp->mainWidget());
	middle->lower();
	window = new OWindow(this);
	widget = QWidget::createWindowContainer(window, middle);
	middle->setAcceptDrops(false);
	widget->setAcceptDrops(false);
}
