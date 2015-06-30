#include "QPlayer.h"
#include "../Config.h"
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
			if (chroma.isEmpty())
				return false;
			QString buffer(chroma);
			ARender::instance()->setBuffer(buffer, format.frameSize());
			if (buffer != chroma)
				return false;
			QSize pixel(format.pixelAspectRatio());
			ARender::instance()->setPixelAspectRatio(pixel.width() / (double)pixel.height());
			return true;
		}

		bool present(const QVideoFrame &frame)
		{
			QVideoFrame f(frame);
			if (f.map(QAbstractVideoBuffer::ReadOnly)){
				int len = f.mappedBytes();
				const quint8 *dat = f.bits();
				QList<quint8 *> buffer = ARender::instance()->getBuffer();
				memcpy(buffer[0], dat, len);
				ARender::instance()->releaseBuffer();
				f.unmap();
			}
			else{
				return false;
			}
			emit APlayer::instance()->decode();
			return true;
		}

		QList<QVideoFrame::PixelFormat> supportedPixelFormats(QAbstractVideoBuffer::HandleType handleType) const
		{
			QList<QVideoFrame::PixelFormat> f;
			if (QAbstractVideoBuffer::NoHandle == handleType){
				f << QVideoFrame::Format_NV12 <<
					QVideoFrame::Format_NV21 <<
					QVideoFrame::Format_YV12 <<
					QVideoFrame::Format_YUV420P;
			}
			return f;
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


QPlayer::QPlayer(QObject *parent) :
APlayer(parent)
{
	ins = this;
	setObjectName("QPlayer");
	state = Stop;
	manuallyStopped = false;
	waitingForBegin = false;
	skipTimeChanged = false;

	mp = (new QPlayerThread(this))->getMediaPlayer();
	mp->setVolume(Config::getValue("/Playing/Volume", 50));

	connect<void(QMediaPlayer::*)(QMediaPlayer::Error)>(mp, &QMediaPlayer::error, this, [this](int error){
		if ((State)mp->state() == Play){
			manuallyStopped = true;
		}
		emit errorOccurred(error);
	});

	connect(mp, &QMediaPlayer::volumeChanged, this, &QPlayer::volumeChanged);

	connect(mp, &QMediaPlayer::stateChanged, this, [this](int _state){
		if (_state == Stop){
			if (!manuallyStopped&&Config::getValue("/Playing/Loop", false)){
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
			ARender::instance()->setMusic(!mp->isVideoAvailable());
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

QList<QAction *> QPlayer::getTracks(int)
{
	return QList<QAction *>();
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

void QPlayer::setTime(qint64 _time)
{
	QMetaObject::invokeMethod(mp, "setPosition",
		Qt::BlockingQueuedConnection,
		Q_ARG(qint64, _time));
	skipTimeChanged = true;
	emit jumped(_time);
}

qint64 QPlayer::getTime()
{
	return mp->position();
}

void QPlayer::setMedia(QString _file, bool manually)
{
	stop(manually);
	QMetaObject::invokeMethod(mp, "setMedia",
		Qt::BlockingQueuedConnection,
		Q_ARG(QMediaContent, QUrl::fromLocalFile(_file)));
	QMetaObject::invokeMethod(mp, "setPlaybackRate",
		Qt::BlockingQueuedConnection,
		Q_ARG(qreal, 1.0));
	if (Config::getValue("/Playing/Immediate", false)){
		play();
	}
}

QString QPlayer::getMedia()
{
	QUrl u = mp->media().canonicalUrl();
	return u.isLocalFile() ? u.toLocalFile() : QString();
}

qint64 QPlayer::getDuration()
{
	return mp->duration();
}

void QPlayer::setVolume(int _volume)
{
	_volume = qBound(0, _volume, 100);
	mp->setVolume(_volume);
	Config::setValue("/Playing/Volume", _volume);
}

int QPlayer::getVolume()
{
	return mp->volume();
}

void QPlayer::setRate(double _rate)
{
	mp->setPlaybackRate(_rate);
}

double QPlayer::getRate()
{
	return mp->playbackRate();
}
