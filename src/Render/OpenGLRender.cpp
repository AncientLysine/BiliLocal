#include "OpenGLRender.h"
#include "OpenGLRenderPrivateBase.h"
#include "SyncTextureCache.h"
#include "../Config.h"
#include "../Local.h"
#include "../Player/APlayer.h"
#include <QOpenGLWidget>
#include <QOpenGLWindow>

class OpenGLRenderPrivate :public OpenGLRenderPrivateBase
{
public:
	QSize inner;
	int format;
	GLuint frame[3];
	QMutex dataLock;
	QList<quint8 *> buffer;

	void initialize()
	{
		OpenGLRenderPrivateBase::initialize();
		glGenTextures(3, frame);
	}

	void uploadTexture(int i, int c, int w, int h)
	{
		OpenGLRenderPrivateBase::uploadTexture(frame[i], c, w, h, buffer[i]);
	}

	void drawData(QPainter *painter, QRect rect)
	{
		if (inner.isEmpty()){
			return;
		}
		painter->beginNativePainting();
		if (dirty){
			int w = inner.width(), h = inner.height();
			dataLock.lock();
			switch (format){
			case 0:
			case 1:
				uploadTexture(0, 1, w, h);
				uploadTexture(1, 1, w / 2, h / 2);
				uploadTexture(2, 1, w / 2, h / 2);
				break;
			case 2:
			case 3:
				uploadTexture(0, 1, w, h);
				uploadTexture(1, 2, w / 2, h / 2);
				break;
			case 4:
				uploadTexture(0, 4, w, h);
				break;
			}
			dirty = false;
			dataLock.unlock();
		}
		QRect dest = fitRect(ARender::instance()->getPreferSize(), rect);
		drawTexture(frame, format, dest, rect);
		painter->endNativePainting();
	}

	void paint(QPaintDevice *device)
	{
		QPainter painter(device);
		painter.setRenderHints(QPainter::SmoothPixmapTransform);
		QRect rect(QPoint(0, 0), ARender::instance()->getActualSize());
		if (APlayer::instance()->getState() == APlayer::Stop){
			drawStop(&painter, rect);
		}
		else{
			drawPlay(&painter, rect);
			drawTime(&painter, rect);
		}
	}

	QList<quint8 *> getBuffer()
	{
		dataLock.lock();
		return buffer;
	}

	void releaseBuffer()
	{
		dirty = true;
		dataLock.unlock();
	}

	void setBuffer(QString &chroma, QSize size, QList<QSize> *bufferSize)
	{
		if (chroma == "YV12"){
			format = 1;
		}
		else if (chroma == "NV12"){
			format = 2;
		}
		else if (chroma == "NV21"){
			format = 3;
		}
		else{
			format = 0;
			chroma = "I420";
		}
		inner = size;
		int s = size.width()*size.height();
		quint8 *alloc = nullptr;
		QList<QSize> plane;
		switch (format){
		case 0:
		case 1:
			alloc = new quint8[s * 3 / 2];
			plane.append(size);
			size /= 2;
			plane.append(size);
			plane.append(size);
			break;
		case 2:
		case 3:
			alloc = new quint8[s * 3 / 2];
			plane.append(size);
			size.rheight() /= 2;
			plane.append(size);
			break;
		}
		if (!buffer.isEmpty()){
			delete[]buffer[0];
		}
		buffer.clear();
		for (const QSize &s : plane){
			buffer.append(alloc);
			alloc += s.width()*s.height();
		}
		if (bufferSize)
			bufferSize->swap(plane);
	}

	virtual quintptr getHandle() = 0;
	virtual void resize(QSize) = 0;
	virtual QSize getActualSize() = 0;
	virtual void draw(QRect) = 0;

	~OpenGLRenderPrivate()
	{
		if (!buffer.isEmpty()){
			delete[]buffer[0];
		}
	}
};

namespace
{
	class OWidget :public QOpenGLWidget
	{
	public:
		explicit OWidget(OpenGLRenderPrivate *render) :
			QOpenGLWidget(lApp->mainWidget()), render(render)
		{
			setAttribute(Qt::WA_TransparentForMouseEvents);
			lower();
			connect(this, &OWidget::frameSwapped, [this](){
				if (isVisible() && APlayer::instance()->getState() == APlayer::Play){
					QTimer::singleShot(2, ARender::instance(), SLOT(draw()));
				}
			});
		}

	private:
		OpenGLRenderPrivate *const render;

		void initializeGL()
		{
			render->initialize();
		}

		void paintGL()
		{
			render->paint(this);
		}
	};
}

class OpenGLWidgetRenderPrivate :public OpenGLRenderPrivate
{
public:
	OpenGLWidgetRenderPrivate()
	{
		widget = new OWidget(this);
	}

	quintptr getHandle()
	{
		return (quintptr)widget;
	}

	void resize(QSize size)
	{
		widget->resize(size);
	}

	QSize getActualSize()
	{
		return widget->size();
	}

	void draw(QRect rect)
	{
		widget->update(rect);
	}

private:
	OWidget *widget;
};

namespace
{
	class OWindow :public QOpenGLWindow
	{
	public:
		explicit OWindow(OpenGLRenderPrivate *render) :
			render(render)
		{
			window = nullptr;
			QTimer::singleShot(0, [this](){
				window = lApp->mainWidget()->backingStore()->window();
			});
			connect(this, &OWindow::frameSwapped, [this](){
				if (isVisible() && APlayer::instance()->getState() == APlayer::Play){
					QTimer::singleShot(2, ARender::instance(), SLOT(draw()));
				}
			});
		}

	private:
		QWindow *window;
		OpenGLRenderPrivate *const render;

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

class OpenGLWindowRenderPrivate :public OpenGLRenderPrivate
{
public:
	OpenGLWindowRenderPrivate()
	{
		widget = lApp->mainWidget();
		middle = new FParent(widget);
		middle->lower();
		window = new OWindow(this);
		widget = QWidget::createWindowContainer(window, middle);
		middle->setAcceptDrops(false);
		widget->setAcceptDrops(false);
	}

	quintptr getHandle()
	{
		return (quintptr)window;
	}

	void resize(QSize size)
	{
		middle->resize(size);
		widget->resize(size);
	}

	QSize getActualSize()
	{
		return widget->size();
	}

	void draw(QRect rect)
	{
		window->update(rect);
	}

private:
	QWidget *widget;
	FParent *middle;
	OWindow *window;
};

namespace
{
	OpenGLRenderPrivate *choose()
	{
		if (Config::getValue("/Performance/Option/OpenGL/FBO", true))
		{
			return new OpenGLWidgetRenderPrivate;
		}
		else
		{
			return new OpenGLWindowRenderPrivate;
		}
	}
}

OpenGLRender::OpenGLRender(QObject *parent) :
ARender(choose(), parent)
{
	ins = this;
	setObjectName("ORender");
	connect(APlayer::instance(), SIGNAL(stateChanged(int)), this, SLOT(draw()));
}

ARender::ICache *OpenGLRender::getCache(const QImage &i)
{
	Q_D(OpenGLRender);
	return new SyncTextureTCache(i, d);
}

quintptr OpenGLRender::getHandle()
{
	Q_D(OpenGLRender);
	return d->getHandle();
}

void OpenGLRender::resize(QSize size)
{
	Q_D(OpenGLRender);
	d->resize(size);
}

QSize OpenGLRender::getActualSize()
{
	Q_D(OpenGLRender);
	return d->getActualSize();
}

QSize OpenGLRender::getBufferSize()
{
	Q_D(OpenGLRender);
	return d->inner;
}

void OpenGLRender::draw(QRect rect)
{
	Q_D(OpenGLRender);
	d->draw(rect.isValid() ? rect : QRect(QPoint(0, 0), getActualSize()));
}
