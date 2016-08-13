#include "Common.h"
#include "QPlayer.h"
#include "../Config.h"
#include "../Local.h"
#include "../Render/ARender.h"
#include "../Render/ABuffer.h"
#include "../Render/PFormat.h"

namespace
{
	QString getFormat(QVideoFrame::PixelFormat format)
	{
		switch (format){
		case QVideoFrame::Format_YUV420P:
			return "I420";
		case QVideoFrame::Format_YV12:
			return "YV12";
		case QVideoFrame::Format_NV12:
			return "NV12";
		case QVideoFrame::Format_NV21:
			return "NV21";
		case QVideoFrame::Format_RGB32:
			return "BGRA";
		case QVideoFrame::Format_BGR32:
			return "ARGB";
		default:
			return QString();
		}
	}

	class FrameBuffer : public ABuffer
	{
	public:
		explicit FrameBuffer(const QVideoFrame &frame)
			: frame(frame)
		{
		}

		virtual bool map() override
		{
			return frame.map(QAbstractVideoBuffer::ReadOnly);
		}

		virtual const uchar *bits() const override
		{
			return frame.bits();
		}

		virtual QList<QSize> size() const override
		{
			QList<QSize> alloc;
			int i;
			int total = frame.mappedBytes();
			int count = frame.planeCount() - 1;
			for (i = 0; i < count; ++i) {
				int plane = frame.bits(i + 1) - frame.bits(i);
				if (plane == 0) {
					continue;
				}
				int width = frame.bytesPerLine(i);
				alloc.append(QSize(width, plane / width));
				total -= plane;
			}
			{
				int width = frame.bytesPerLine(i);
				alloc.append(QSize(width, total / width));
			}
			return alloc;
		}

		virtual void unmap() override
		{
			frame.unmap();
		}

		virtual HandleType handleType() const override
		{
			switch (frame.handleType()) {
			case QAbstractVideoBuffer::GLTextureHandle:
				return GLTextureHandle;
			default:
				return NoHandle;
			}
		}

		virtual QVariant handle() const override
		{
			return frame.handle();
		}

	private:
		QVideoFrame frame;
	};

	class VideoSurface : public QAbstractVideoSurface
	{
	public:
		VideoSurface(QObject *parent = 0) :
			QAbstractVideoSurface(parent)
		{
		}

		bool start(const QVideoSurfaceFormat &format) override
		{
			QString chroma = getFormat(format.pixelFormat());
			if (chroma.isEmpty()) {
				return false;
			}
			PFormat f;
			f.chroma = chroma;
			f.size = format.frameSize();
			lApp->findObject<ARender>()->setFormat(&f);
			if (f.chroma != chroma) {
				return false;
			}
			setNativeResolution(f.size);
			QSize pixel(format.pixelAspectRatio());
			lApp->findObject<ARender>()->setPixelAspectRatio(pixel.width() / (double)pixel.height());
			return QAbstractVideoSurface::start(format);
		}

		bool present(const QVideoFrame &frame) override
		{
			lApp->findObject<ARender>()->setBuffer(new FrameBuffer(frame));
			emit lApp->findObject<APlayer>()->decode();
			return true;
		}

		QList<QVideoFrame::PixelFormat> supportedPixelFormats(QAbstractVideoBuffer::HandleType handleType) const override
		{
			QList<QVideoFrame::PixelFormat> fmt;
			switch (handleType) {
			case QAbstractVideoBuffer::NoHandle:
				fmt << QVideoFrame::Format_RGB32
					<< QVideoFrame::Format_BGR32
					<< QVideoFrame::Format_NV12
					<< QVideoFrame::Format_NV21
					<< QVideoFrame::Format_YV12
					<< QVideoFrame::Format_YUV420P;
				break;
			case QAbstractVideoBuffer::GLTextureHandle:
				/*TODO: dsengine fails to render to texture
				fmt << QVideoFrame::Format_RGB32
					<< QVideoFrame::Format_BGR32;*/
				break;
			default:
				break;
			}
			return fmt;
		}
	};

	class PlayerThread : public QThread
	{
	public:
		explicit PlayerThread(QObject *parent = 0) :
			QThread(parent), mp(0)
		{
		}

		~PlayerThread()
		{
			if (isRunning()){
				quit();
				wait();
			}
		}

		QMediaPlayer *getMediaPlayer()
		{
			if (!isRunning() && mp == 0){
				m.lock();
				start();
				w.wait(&m);
				m.unlock();
			}
			return mp;
		}

	private:
		QMediaPlayer *mp;
		QMutex m;
		QWaitCondition w;

		void run()
		{
			m.lock();
			mp = new QMediaPlayer;
			mp->setNotifyInterval(300);
			mp->setVideoOutput(new VideoSurface(mp));
			m.unlock();
			w.wakeAll();
			exec();
			delete mp;
		}
	};
}


QPlayer::QPlayer(QObject *parent)
	: APlayer(parent)
{
	setObjectName("QPlayer");

	state = Stop;
	manuallyStopped = false;
	waitingForBegin = false;
	skipTimeChanged = false;
	auto thread = new PlayerThread(this);
	pt = thread;
	mp = thread->getMediaPlayer();
	mp->setVolume(Config::getValue("/Player/Volume", 50));

	connect<void(QMediaPlayer::*)(QMediaPlayer::Error)>(mp, &QMediaPlayer::error, this, [this](int error){
		if ((State)mp->state() == Play){
			manuallyStopped = true;
		}
		emit errorOccurred(error);
	});

	connect(mp, &QMediaPlayer::volumeChanged, this, &QPlayer::volumeChanged);

	connect(mp, &QMediaPlayer::stateChanged, this, [this](int _state){
		if (_state == Stop){
			if (!manuallyStopped&&Config::getValue("/Player/Loop", false)){
				stateChanged(state = Loop);
				play();
				emit jumped(0);
			}
			else{
				stateChanged(state = Stop);
				emit reach(manuallyStopped);
			}
			manuallyStopped = false;
		}
		else{
			manuallyStopped = false;
			if (_state == Play&&state == Stop){
				waitingForBegin = true;
			}
			else{
				emit stateChanged(state = _state);
			}
		}
	});

	connect(mp, &QMediaPlayer::positionChanged, this, [this](qint64 time){
		if (waitingForBegin&&time > 0){
			waitingForBegin = false;
			lApp->findObject<ARender>()->setMusic(!mp->isVideoAvailable());
			emit stateChanged(state = Play);
			emit begin();
		}
		if (!skipTimeChanged){
			emit timeChanged(time);
		}
		else{
			skipTimeChanged = false;
		}
	});

	connect(mp, &QMediaPlayer::mediaChanged, this, [this](){
		emit mediaChanged(getMedia());
	});

	connect(mp, &QMediaPlayer::playbackRateChanged, this, &QPlayer::rateChanged);
}

QPlayer::~QPlayer()
{
	pt->exit();
	pt->wait();
}

void QPlayer::play()
{
	QMetaObject::invokeMethod(mp, getState() == Play ? "pause" : "play",
		Qt::BlockingQueuedConnection);
}

void QPlayer::stop(bool manually)
{
	manuallyStopped = manually;
	QMetaObject::invokeMethod(mp, "stop",
		Qt::BlockingQueuedConnection);
}

void QPlayer::setTime(qint64 time)
{
	QMetaObject::invokeMethod(mp, "setPosition",
		Qt::BlockingQueuedConnection,
		Q_ARG(qint64, time));
	skipTimeChanged = true;
	emit jumped(time);
}

qint64 QPlayer::getTime()
{
	return mp->position();
}

void QPlayer::setMedia(QString file)
{
	QMetaObject::invokeMethod(mp, "setMedia",
		Qt::BlockingQueuedConnection,
		Q_ARG(QMediaContent, QUrl::fromUserInput(file)));
	QMetaObject::invokeMethod(mp, "setPlaybackRate",
		Qt::BlockingQueuedConnection,
		Q_ARG(qreal, 1.0));
	if (Config::getValue("/Player/Immediate", false)){
		play();
	}
}

QString QPlayer::getMedia()
{
	const QUrl &u = mp->media().canonicalUrl();
	return u.isLocalFile() ? u.toLocalFile() : u.toString();
}

qint64 QPlayer::getDuration()
{
	return mp->duration();
}

void QPlayer::setVolume(int volume)
{
	volume = qBound(0, volume, 100);
	mp->setVolume(volume);
	Config::setValue("/Player/Volume", volume);
}

int QPlayer::getVolume()
{
	return mp->volume();
}

void QPlayer::setRate(double rate)
{
	mp->setPlaybackRate(rate);
}

double QPlayer::getRate()
{
	return mp->playbackRate();
}
