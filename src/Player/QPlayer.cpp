#include "Common.h"
#include "QPlayer.h"
#include "../Config.h"
#include "../Local.h"
#include "../Render/ARender.h"

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
		case QVideoFrame::Format_BGR32:
			return "BGRA";
		default:
			return QString();
		}
	}

	class RenderAdapter :public QAbstractVideoSurface
	{
	public:
		RenderAdapter(QObject *parent = 0) :
			QAbstractVideoSurface(parent)
		{
		}

		bool start(const QVideoSurfaceFormat &format)
		{
			QString chroma = getFormat(format.pixelFormat());
			if (chroma.isEmpty()) {
				return false;
			}
			QString buffer(chroma);
			lApp->findObject<ARender>()->setBuffer(buffer, format.frameSize(), 1);
			if (buffer != chroma) {
				return false;
			}
			QSize pixel(format.pixelAspectRatio());
			lApp->findObject<ARender>()->setPixelAspectRatio(pixel.width() / (double)pixel.height());
			return QAbstractVideoSurface::start(format);
		}

		bool present(const QVideoFrame &frame)
		{
			QVideoFrame f(frame);
			if (f.map(QAbstractVideoBuffer::ReadOnly)){
				int len = f.mappedBytes();
				const quint8 *dat = f.bits();
				QList<quint8 *> buffer = lApp->findObject<ARender>()->getBuffer();
				memcpy(buffer[0], dat, len);
				lApp->findObject<ARender>()->releaseBuffer();
				f.unmap();
			}
			else{
				return false;
			}
			emit lApp->findObject<APlayer>()->decode();
			return true;
		}

		QList<QVideoFrame::PixelFormat> supportedPixelFormats(QAbstractVideoBuffer::HandleType handleType) const
		{
			QList<QVideoFrame::PixelFormat> fmt;
			switch (handleType) {
			case QAbstractVideoBuffer::NoHandle:
				fmt << QVideoFrame::Format_BGR32
					<< QVideoFrame::Format_NV12
					<< QVideoFrame::Format_NV21
					<< QVideoFrame::Format_YV12
					<< QVideoFrame::Format_YUV420P;
				break;
			case QAbstractVideoBuffer::GLTextureHandle:
			default:
				break;
			}
			return fmt;
		}
	};

	class QPlayerThread :public QThread
	{
	public:
		explicit QPlayerThread(QObject *parent = 0) :
			QThread(parent), mp(0)
		{
		}

		~QPlayerThread()
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
			mp->setVideoOutput(new RenderAdapter(mp));
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
	mp = (new QPlayerThread(this))->getMediaPlayer();
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
