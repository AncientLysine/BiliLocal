#include "Common.h"
#include "DetachPrivate.h"
#include <QDesktopWidget>
#include <functional>

namespace
{
	class DWindow :public QOpenGLWindow
	{
	public:
		explicit DWindow(OpenGLDetachRenderPrivate *render) :
			render(render)
		{
			QSurfaceFormat f = format();
			f.setAlphaBufferSize(8);
			setFormat(f);
			setFlags(flags() | Qt::Tool | Qt::FramelessWindowHint | Qt::WindowTransparentForInput | Qt::WindowStaysOnTopHint);
			setGeometry(qApp->desktop()->screenGeometry());
			connect(this, &DWindow::frameSwapped, std::bind(&OpenGLRenderPrivate::onSwapped, render));
		}

	private:
		OpenGLDetachRenderPrivate *const render;

		void initializeGL()
		{
			render->initialize();
		}

		void paintGL()
		{
			render->glClearColor(0, 0, 0, 0);
			render->glClear(GL_COLOR_BUFFER_BIT);
			QPainter painter(this);
			painter.setRenderHints(QPainter::SmoothPixmapTransform);
			QRect rect(QPoint(0, 0), lApp->findObject<ARender>()->getActualSize());
			render->drawDanm(&painter, rect);
		}
	};
}

OpenGLDetachRenderPrivate::OpenGLDetachRenderPrivate()
{
	tv.disconnect();
	window = new DWindow(this);
	window->create();
	QObject::connect(lApp->findObject<APlayer>(), &APlayer::begin, window, &QWindow::show);
	QObject::connect(lApp->findObject<APlayer>(), &APlayer::reach, window, &QWindow::hide);
}
