/*=======================================================================
*
*   Copyright (C) 2013 Lysine.
*
*   Filename:    VPlayer.cpp
*   Time:        2013/03/18
*   Author:      Lysine
*
*   Lysine is a student majoring in Software Engineering
*   from the School of Software, SUN YAT-SEN UNIVERSITY.
*
*   This program is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option) any later version.
*
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.

*   You should have received a copy of the GNU General Public License
*   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
=========================================================================*/

#include "APlayer.h"
#include "Config.h"
#include "Local.h"
#include "Render.h"
#include "Utils.h"

#ifdef BACKEND_VLC
extern "C"
{
#include <vlc/vlc.h>
}

class VPlayer:public APlayer
{
public:
	enum Event
	{
		Init,
		Wait,
		Free,
		Fail
	};

	explicit VPlayer(QObject *parent=0);
	~VPlayer();
	static QMutex time;
	QList<QAction *> getTracks(int type);

private:
	int state;
	QActionGroup *tracks[3];
	libvlc_instance_t *vlc;
	libvlc_media_player_t *mp;

	void	init();
	void	wait();
	void	free();

public slots:
	void	play();
	void	stop(bool manually=true);
	int 	getState(){return state;}

	void	setTime(qint64 _time);
	qint64	getTime();

	void	setMedia(QString _file,bool manually=true);
	QString getMedia();

	qint64	getDuration();
	void	addSubtitle(QString _file);

	void	setVolume(int _volume);
	int 	getVolume();

	QSize   getSize();

	void    setRate(double _rate);
	double  getRate();

	void	event(int type);

};

QMutex VPlayer::time;

namespace
{
class Buffer
{
public:
	explicit Buffer(const QList<QSize> &planeSize)
	{
		size=0;
		QList<int> planeLength;
		for(const QSize &s:planeSize){
			int length=s.width()*s.height();
			planeLength.append(length);
			size+=length;
		}
		quint8 *alloc=new quint8[size];
		for(int length:planeLength){
			data.append(alloc);
			alloc+=length;
		}
	}

	~Buffer()
	{
		delete []data[0];
	}

	void flush()
	{
		memcpy(Render::instance()->getBuffer()[0],data[0],size);
		Render::instance()->releaseBuffer();
	}

	QList<quint8 *> getBuffer()
	{
		return data;
	}

private:
	QList<quint8 *> data;
	int size;
};

unsigned fmt(void **opaque,char *chroma,
					unsigned *width,unsigned *height,
					unsigned *p,unsigned *l)
{
	QString c(chroma);
	QList<QSize> b;
	Render::instance()->setBuffer(c,QSize(*width,*height),&b);
	if (b.isEmpty()){
		return 0;
	}
	memcpy(chroma,c.toUtf8(),4);
	for(int i=0;i<b.size();++i){
		const QSize &s=b[i];
		p[i]=s.width();l[i]=s.height();
	}
	*opaque=(void *)new Buffer(b);
	return 1;
}

void *lck(void *opaque,void **planes)
{
	int i=0;
	for(quint8 *p:((Buffer *)opaque)->getBuffer()){
		planes[i++]=(void *)p;
	}
	return nullptr;
}

void dsp(void *opaque,void *)
{
	((Buffer *)opaque)->flush();
	emit APlayer::instance()->decode();
}

void clr(void *opaque)
{
	delete (Buffer *)opaque;
}

void sta(const libvlc_event_t *,void *)
{
	QMetaObject::invokeMethod(APlayer::instance(),"event",Q_ARG(int,VPlayer::Init));
}

void mid(const libvlc_event_t *,void *)
{
	if (VPlayer::time.tryLock()) {
		QMetaObject::invokeMethod(APlayer::instance(),
								  "timeChanged",
								  Q_ARG(qint64,APlayer::instance()->getTime()));
		VPlayer::time.unlock();
	}
}

void hal(const libvlc_event_t *,void *)
{
	QMetaObject::invokeMethod(APlayer::instance(),"event",Q_ARG(int,VPlayer::Wait));
}

void end(const libvlc_event_t *,void *)
{
	QMetaObject::invokeMethod(APlayer::instance(),"event",Q_ARG(int,VPlayer::Free));
}

void err(const libvlc_event_t *,void *)
{
	QMetaObject::invokeMethod(APlayer::instance(),"event",Q_ARG(int,VPlayer::Fail));
}
}

VPlayer::VPlayer(QObject *parent):
	APlayer(parent)
{
	ins=this;
	setObjectName("VPlayer");
	QList<QByteArray> args;
	for(QJsonValue arg:Config::getValue<QJsonArray>("/Playing/Arguments")){
		args.append(arg.toString().toUtf8());
	}
	const char **argv=args.isEmpty()?nullptr:new const char *[args.size()];
	for(int i=0;i<args.size();++i){
		argv[i]=args[i];
	}
	vlc=libvlc_new(args.size(),argv);
#ifdef Q_OS_WIN
	libvlc_add_intf(vlc,"bililocal");
#endif
	mp=nullptr;
	state=Stop;
	for(auto &iter:tracks){
		iter=new QActionGroup(this);
		iter->setExclusive(true);
	}
}

VPlayer::~VPlayer()
{
	if(mp){
		libvlc_media_player_release(mp);
	}
	libvlc_release(vlc);
}

QList<QAction *> VPlayer::getTracks(int type)
{
	QList<QAction *> track;
	if(type&Utils::Video){
		track+=tracks[0]->actions();
	}
	if(type&Utils::Audio){
		track+=tracks[1]->actions();
	}
	if(type&Utils::Subtitle){
		track+=tracks[2]->actions();
	}
	return track;
}

namespace
{
void copyTracks(libvlc_track_description_t *head,QActionGroup *group)
{
	qDeleteAll(group->actions());
	libvlc_track_description_t *iter=head;
	while(iter){
		QAction *action=group->addAction(iter->psz_name);
		action->setCheckable(true);
		action->setData(iter->i_id);
		iter=iter->p_next;
	}
	libvlc_track_description_list_release(head);
}
}

void VPlayer::init()
{
	if(mp){
		auto *connection=new QMetaObject::Connection;
		*connection=connect(this,&VPlayer::timeChanged,this,[=](){
			int last=state;
			emit stateChanged(state=Play);
			disconnect(*connection);
			delete connection;
			switch(last){
			case Stop:
			{
				bool music=true;
				libvlc_media_track_t **info;
				int n=libvlc_media_tracks_get(libvlc_media_player_get_media(mp),&info);
				for(int i=0;i<n;++i){
					if(info[i]->i_type==libvlc_track_video){
						libvlc_video_track_t *v=info[i]->video;
						double r=v->i_sar_den==0?1:(double)v->i_sar_num/v->i_sar_den;
						Render::instance()->setPixelAspectRatio(r);
						music=false;
						break;
					}
				}
				libvlc_media_tracks_release(info,n);
				Render::instance()->setMusic(music);
				if(!Config::getValue("/Playing/Subtitle",true)){
					libvlc_video_set_spu(mp,-1);
				}
				copyTracks(libvlc_video_get_spu_description(mp),tracks[2]);
				copyTracks(libvlc_video_get_track_description(mp),tracks[0]);
				copyTracks(libvlc_audio_get_track_description(mp),tracks[1]);
				for(QAction *i:tracks[0]->actions()){
					int t=i->data().toInt();
					connect(i,&QAction::triggered,[=](){
						libvlc_video_set_track(mp,t);
						Render::instance()->setMusic(t==-1);
					});
					i->setChecked(t==libvlc_video_get_track(mp));
				}
				for(QAction *i:tracks[1]->actions()){
					connect(i,&QAction::triggered,[=](){libvlc_audio_set_track(mp,i->data().toInt());});
					i->setChecked(i->data().toInt()==libvlc_audio_get_track(mp));
				}
				for(QAction *i:tracks[2]->actions()){
					connect(i,&QAction::triggered,[=](){libvlc_video_set_spu(mp,i->data().toInt());});
					i->setChecked(i->data().toInt()==libvlc_video_get_spu(mp));
				}
				emit begin();
				break;
			}
			case Loop:
			{
				for(auto *g:tracks){
					for(QAction *i:g->actions()){
						if(i->isChecked()){
							i->trigger();
						}
					}
				}
				break;
			}
			default:
				return;
			}
			setVolume(Config::getValue("/Playing/Volume",50));
		});
	}
}

void VPlayer::wait()
{
	emit stateChanged(state=Pause);
}

void VPlayer::free()
{
	if(state==Play&&Config::getValue("/Playing/Loop",false)){
		libvlc_media_player_stop(mp);
		emit stateChanged(state=Loop);
		libvlc_media_player_play(mp);
		emit jumped(0);
	}
	else{
		stop(false);
	}
}

void VPlayer::play()
{
	if(mp){
		if(state==Stop){
			libvlc_video_set_format_callbacks(mp,fmt,clr);
			libvlc_video_set_callbacks(mp,lck,nullptr,dsp,nullptr);
			libvlc_media_player_play(mp);
		}
		else{
			libvlc_media_player_pause(mp);
		}
	}
}

void VPlayer::stop(bool manually)
{
	if(mp&&state!=Stop){
		libvlc_media_player_stop(mp);
		emit stateChanged(state=Stop);
		for(auto g:tracks){
			qDeleteAll(g->actions());
		}
		emit reach(manually);
	}
}

void VPlayer::setTime(qint64 _time)
{
	if(mp&&state!=Stop){
		if(getDuration()==_time){
			if(Config::getValue("/Playing/Loop",false)){
				setTime(0);
			}
			else{
				stop();
			}
		}
		else{
			time.lock();
			qApp->processEvents();
			emit jumped(_time);
			libvlc_media_player_set_time(mp,qBound<qint64>(0,_time,getDuration()));
			time.unlock();
		}
	}
}

qint64 VPlayer::getTime()
{
	return state==Stop?-1:libvlc_media_player_get_time(mp);
}

void VPlayer::setMedia(QString _file,bool manually)
{
	stop(manually);
	libvlc_media_t *m=libvlc_media_new_path(vlc,QDir::toNativeSeparators(_file).toUtf8());
	if(!m){
		return;
	}
	if(mp){
		libvlc_media_player_release(mp);
	}
	mp=libvlc_media_player_new_from_media(m);
	libvlc_media_release(m);
	if(!mp){
		return;
	}
	libvlc_event_manager_t *man=libvlc_media_player_event_manager(mp);
	libvlc_event_attach(man,
						libvlc_MediaPlayerPlaying,
						sta,nullptr);
	libvlc_event_attach(man,
						libvlc_MediaPlayerTimeChanged,
						mid,nullptr);
	libvlc_event_attach(man,
						libvlc_MediaPlayerPaused,
						hal,nullptr);
	libvlc_event_attach(man,
						libvlc_MediaPlayerEndReached,
						end,nullptr);
	libvlc_event_attach(man,
						libvlc_MediaPlayerEncounteredError,
						err,nullptr);
	emit mediaChanged(getMedia());
	if (Config::getValue("/Playing/Immediate",false)){
		play();
	}
}

QString VPlayer::getMedia()
{
	if(mp){
		libvlc_media_t *m=libvlc_media_player_get_media(mp);
		char *s=libvlc_media_get_mrl(m);
		QUrl u(s);
		libvlc_free(s);
		libvlc_media_release(m);
		return u.isLocalFile()?u.toLocalFile():u.url();
	}
	return QString();
}

qint64 VPlayer::getDuration()
{
	return mp?libvlc_media_player_get_length(mp):-1;
}

void VPlayer::addSubtitle(QString _file)
{
	if(mp){
		if(tracks[2]->actions().isEmpty()){
			QAction *action=tracks[2]->addAction(APlayer::tr("Disable"));
			action->setCheckable(true);
			connect(action,&QAction::triggered,[this](){
				libvlc_video_set_spu(mp,-1);
			});
		}
		QAction *outside=new QAction(tracks[2]);
		QFileInfo info(_file);
		outside->setCheckable(true);
		outside->setText(qApp->fontMetrics().elidedText(info.fileName(),Qt::ElideMiddle,200));
		outside->setData(QDir::toNativeSeparators(info.absoluteFilePath()));
		connect(outside,&QAction::triggered,[=](){
			libvlc_video_set_subtitle_file(mp,outside->data().toString().toUtf8());
		});
		outside->trigger();
	}
}

void VPlayer::setVolume(int _volume)
{
	_volume=qBound(0,_volume,100);
	Config::setValue("/Playing/Volume",_volume);
	if(mp){
		libvlc_audio_set_volume(mp,_volume);
	}
	emit volumeChanged(_volume);
}

int VPlayer::getVolume()
{
	return mp?libvlc_audio_get_volume(mp):0;
}

QSize VPlayer::getSize()
{
	unsigned w=0,h=0;
	if (mp){
		libvlc_video_get_size(mp,0,&w,&h);
	}
	return QSize(w,h);
}

void VPlayer::setRate(double _rate)
{
	if (mp){
		libvlc_media_player_set_rate(mp,_rate);
	}
}

double VPlayer::getRate()
{
	return mp?libvlc_media_player_get_rate(mp):0;
}

void VPlayer::event(int type)
{
	switch(type){
	case Init:
		init();
		break;
	case Wait:
		wait();
		break;
	case Free:
		free();
		break;
	case Fail:
		emit errorOccurred(UnknownError);
		break;
	}
}
#endif

#ifdef BACKEND_QMM
#include <QtMultimedia>

namespace
{
QString getFormat(QVideoFrame::PixelFormat format)
{
	switch(format){
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

class RenderAdapter:public QAbstractVideoSurface
{
public:
	RenderAdapter(QObject *parent=0):
		QAbstractVideoSurface(parent)
	{
	}

	bool start(const QVideoSurfaceFormat &format)
	{
		QString chroma=getFormat(format.pixelFormat());
		if (chroma.isEmpty())
			return false;
		QString buffer(chroma);
		Render::instance()->setBuffer(buffer,format.frameSize());
		if (buffer!=chroma)
			return false;
		QSize pixel(format.pixelAspectRatio());
		Render::instance()->setPixelAspectRatio(pixel.width()/(double)pixel.height());
		return true;
	}

	bool present(const QVideoFrame &frame)
	{
		QVideoFrame f(frame);
		if (f.map(QAbstractVideoBuffer::ReadOnly)){
			int len=f.mappedBytes();
			const quint8 *dat=f.bits();
			QList<quint8 *> buffer=Render::instance()->getBuffer();
			memcpy(buffer[0],dat,len);
			Render::instance()->releaseBuffer();
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
		if (QAbstractVideoBuffer::NoHandle==handleType){
			f<<QVideoFrame::Format_NV12<<
			   QVideoFrame::Format_NV21<<
			   QVideoFrame::Format_YV12<<
			   QVideoFrame::Format_YUV420P;
		}
		return f;
	}
};

class QPlayerThread:public QThread
{
public:
	explicit QPlayerThread(QObject *parent=0):
		QThread(parent),mp(0)
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
		if(!isRunning()&&mp==0){
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
		mp=new QMediaPlayer;
		mp->setNotifyInterval(300);
		mp->setVideoOutput(new RenderAdapter(mp));
		m.unlock();
		w.wakeAll();
		exec();
		delete mp;
	}
};
}

class QPlayer:public APlayer
{
public:
	explicit QPlayer(QObject *parent=0);
	QList<QAction *> getTracks(int type);

private:
	QMediaPlayer *mp;
	int state;
	bool manuallyStopped;
	bool waitingForBegin;
	bool skipTimeChanged;

public slots:
	void	play();
	void	stop(bool manually=true);
	int 	getState(){return state;}

	void	setTime(qint64 _time);
	qint64	getTime();

	void	setMedia(QString _file,bool manually=true);
	QString getMedia();

	qint64	getDuration();
	void	addSubtitle(QString _file);

	void	setVolume(int _volume);
	int 	getVolume();

	QSize   getSize();

	void    setRate(double _rate);
	double  getRate();

	void	event(int type);

};

QPlayer::QPlayer(QObject *parent):
	APlayer(parent)
{
	ins=this;
	setObjectName("QPlayer");
	state=Stop;
	manuallyStopped=false;
	waitingForBegin=false;
	skipTimeChanged=false;

	mp=(new QPlayerThread(this))->getMediaPlayer();
	mp->setVolume(Config::getValue("/Playing/Volume",50));

	connect<void(QMediaPlayer::*)(QMediaPlayer::Error)>(mp,&QMediaPlayer::error,this,[this](int error){
		if ((State)mp->state()==Play){
			manuallyStopped=true;
		}
		emit errorOccurred(error);
	});
	
	connect(mp,&QMediaPlayer::volumeChanged,this,&QPlayer::volumeChanged);

	connect(mp,&QMediaPlayer::stateChanged,this,[this](int _state){
		if(_state==Stop){
			if(!manuallyStopped&&Config::getValue("/Playing/Loop",false)){
				stateChanged(state=Loop);
				play();
				emit jumped(0);
			}
			else{
				stateChanged(state=Stop);
				emit reach(manuallyStopped);
			}
			manuallyStopped=false;
		}
		else{
			manuallyStopped=false;
			if(_state==Play&&state==Stop){
				waitingForBegin=true;
			}
			else{
				emit stateChanged(state=_state);
			}
		}
	});

	connect(mp,&QMediaPlayer::positionChanged,this,[this](qint64 time){
		if (waitingForBegin&&time>0){
			waitingForBegin=false;
			Render::instance()->setMusic(!mp->isVideoAvailable());
			emit stateChanged(state=Play);
			emit begin();
		}
		if(!skipTimeChanged){
			emit timeChanged(time);
		}
		else{
			skipTimeChanged=false;
		}
	});

	connect(mp,&QMediaPlayer::mediaChanged,this,[this](){
		emit mediaChanged(getMedia());
	});
}

QList<QAction *> QPlayer::getTracks(int)
{
	return QList<QAction *>();
}

void QPlayer::play()
{
	QMetaObject::invokeMethod(mp,getState()==Play?"pause":"play",
							  Qt::BlockingQueuedConnection);
}

void QPlayer::stop(bool manually)
{
	manuallyStopped=manually;
	QMetaObject::invokeMethod(mp,"stop",
							  Qt::BlockingQueuedConnection);
}

void QPlayer::setTime(qint64 _time)
{
	QMetaObject::invokeMethod(mp,"setPosition",
							  Qt::BlockingQueuedConnection,
							  Q_ARG(qint64,_time));
	skipTimeChanged=true;
	emit jumped(_time);
}

qint64 QPlayer::getTime()
{
	return mp->position();
}

void QPlayer::setMedia(QString _file,bool manually)
{
	stop(manually);
	QMetaObject::invokeMethod(mp,"setMedia",
							  Qt::BlockingQueuedConnection,
							  Q_ARG(QMediaContent,QUrl::fromLocalFile(_file)));
	QMetaObject::invokeMethod(mp,"setPlaybackRate",
							  Qt::BlockingQueuedConnection,
							  Q_ARG(qreal,1.0));
	if(Config::getValue("/Playing/Immediate",false)){
		play();
	}
}

QString QPlayer::getMedia()
{
	QUrl u=mp->media().canonicalUrl();
	return u.isLocalFile()?u.toLocalFile():QString();
}

qint64 QPlayer::getDuration()
{
	return mp->duration();
}

void QPlayer::addSubtitle(QString)
{
}

void QPlayer::setVolume(int _volume)
{
	_volume=qBound(0,_volume,100);
	mp->setVolume(_volume);
	Config::setValue("/Playing/Volume",_volume);
}

int QPlayer::getVolume()
{
	return mp->volume();
}

QSize QPlayer::getSize()
{
	//TODO
	return QSize();
}

void QPlayer::setRate(double _rate)
{
	mp->setPlaybackRate(_rate);
}

double QPlayer::getRate()
{
	return mp->playbackRate();
}

void QPlayer::event(int)
{
}
#endif

#ifdef BACKEND_NIL
class NPlayer:public APlayer
{
public:
	explicit NPlayer(QObject *parent=0);
	QList<QAction *> getTracks(int type);

private:
	qint64 start; 
	int state;

	void	timerEvent(QTimerEvent * e);

public slots:
	void	play();
	void	stop(bool manually=true);
	int 	getState(){return state;}

	void	setTime(qint64 _time);
	qint64	getTime();

	void	setMedia(QString _file,bool manually=true);
	QString getMedia();

	qint64	getDuration();
	void	addSubtitle(QString _file);

	void	setVolume(int _volume);
	int 	getVolume();

	QSize   getSize();

	void    setRate(double _rate);
	double  getRate();

	void	event(int type);

};

NPlayer::NPlayer(QObject *parent):
	APlayer(parent)
{
	ins=this;
	setObjectName("NPlayer");

	state=Stop;
	startTimer(100);
}

void NPlayer::timerEvent(QTimerEvent *)
{
	if(state==Play){
		emit timeChanged(getTime());
	}
}

QList<QAction *> NPlayer::getTracks(int)
{
	return QList<QAction *>();
}

void NPlayer::play()
{
	if(state!=Stop){
		return;
	}
	Render::instance()->setMusic(true);
	emit stateChanged(state=Play);
	emit begin();
	start=QDateTime::currentMSecsSinceEpoch();
}

void NPlayer::stop(bool)
{
	emit reach(true);
	emit stateChanged(state=Stop);
}

void NPlayer::setTime(qint64)
{
}

qint64 NPlayer::getTime()
{
	return state==Stop?-1:(QDateTime::currentMSecsSinceEpoch()-start);
}

void NPlayer::setMedia(QString,bool)
{
}

QString NPlayer::getMedia()
{
	return QString();
}

qint64 NPlayer::getDuration()
{
	return -1;
}

void NPlayer::addSubtitle(QString)
{
}

void NPlayer::setVolume(int)
{
}

int NPlayer::getVolume()
{
	return 0;
}

QSize NPlayer::getSize()
{
	return QSize();
}

void NPlayer::setRate(double)
{
}

double NPlayer::getRate()
{
	return 0;
}

void NPlayer::event(int)
{
}
#endif

APlayer *APlayer::ins=nullptr;

APlayer *APlayer::instance()
{
	if (ins){
		return ins;
	}
	QString d;
	QStringList l=Utils::getDecodeModules();
	switch(l.size()){
	case 0:
		break;
	case 1:
		d=l[0];
		break;
	default:
		d=Config::getValue("/Performance/Decode",l[0]);
		d=l.contains(d)?d:l[0];
		break;
	}
#ifdef BACKEND_VLC
	if (d=="VLC"){
		return new VPlayer(qApp);
	}
#endif
#ifdef BACKEND_QMM
	if (d=="QMM"){
		return new QPlayer(qApp);
	}
#endif
#ifdef BACKEND_NIL
	if (d=="NIL"){
		return new NPlayer(qApp);
	}
#endif
	return 0;
}
