#include "DetachRender.h"
#include "OpenGLRenderPrivateBase.h"
#include "SyncTextureCache.h"
#include "../Player/APlayer.h"
#include <QApplication>
#include <QDesktopWidget>

namespace
{
	class DWindow;
}

class DetachRenderPrivate :public OpenGLRenderPrivateBase
{
public:
	DWindow *window;

	void drawData(QPainter *, QRect)
	{
	}

	QList<quint8 *> getBuffer()
	{
		return QList<quint8 *>();
	}

	void setBuffer(QString &chroma, QSize, QList<QSize> *)
	{
		chroma = "NONE";
	}

	void releaseBuffer()
	{
	}
};

namespace
{
	class DWindow :public QOpenGLWindow
	{
	public:
		explicit DWindow(DetachRenderPrivate *render) :
			render(render)
		{
			QSurfaceFormat f = format();
			f.setAlphaBufferSize(8);
			setFormat(f);
			setFlags(flags() | Qt::Tool | Qt::FramelessWindowHint | Qt::WindowTransparentForInput | Qt::WindowStaysOnTopHint);
			setGeometry(qApp->desktop()->screenGeometry());
			connect(this, &DWindow::frameSwapped, [this](){
				if (isVisible() && APlayer::instance()->getState() == APlayer::Play){
					QTimer::singleShot(2, ARender::instance(), SLOT(draw()));
				}
			});
		}

	private:
		DetachRenderPrivate *const render;

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
			QRect rect(QPoint(0, 0), ARender::instance()->getActualSize());
			render->drawDanm(&painter, rect);
		}
	};
}

DetachRender::DetachRender(QObject *parent) :
ARender(new DetachRenderPrivate, parent)
{
	ins = this;
	setObjectName("DRender");
	Q_D(DetachRender);
	d->tv.disconnect();
	d->window = new DWindow(d);
	d->window->create();
	connect(APlayer::instance(), &APlayer::begin, d->window, &QWindow::show);
	connect(APlayer::instance(), &APlayer::reach, d->window, &QWindow::hide);
}

DetachRender::~DetachRender()
{
	Q_D(DetachRender);
	if (d->window){
		delete d->window;
	}
}

ARender::ICache *DetachRender::getCache(const QImage &i)
{
	Q_D(DetachRender);
	return new SyncTextureTCache(i, d);
}

quintptr DetachRender::getHandle()
{
	Q_D(DetachRender);
	return (quintptr)d->window;
}

void DetachRender::resize(QSize)
{
}

QSize DetachRender::getActualSize()
{
	Q_D(DetachRender);
	return d->window->size();
}

QSize DetachRender::getBufferSize()
{
	return QSize();
}

void DetachRender::draw(QRect rect)
{
	Q_D(DetachRender);
	d->window->update(rect.isValid() ? rect : QRect(QPoint(0, 0), getActualSize()));
}