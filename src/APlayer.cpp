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
#include "Next.h"
#include "Local.h"
#include "Utils.h"
#include "Render.h"
#include "Config.h"

APlayer *APlayer::ins=NULL;

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
		Free
	};

	explicit VPlayer(QObject *parent=0);
	~VPlayer();
	static QMutex time;
	QList<QAction *> getTracks(int type);

private:
	int state;
	QActionGroup *tracks[3];
	libvlc_instance_t *vlc;
	libvlc_media_t *m;
	libvlc_media_player_t *mp;

	void	init();
	void	free();
	void	setState(int _state);

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

	void	event(int type);

};

QMutex VPlayer::time;

static unsigned fmt(void **,char *chroma,
					unsigned *width,unsigned *height,
					unsigned *p,unsigned *l)
{
	QString c(chroma);
	QList<QSize> b;
	Render::instance()->setBuffer(c,QSize(*width,*height),&b);
	memcpy(chroma,c.toUtf8(),4);
	int i=0;
	for(const QSize &s:b){
		p[i]=s.width();l[i++]=s.height();
	}
	return 1;
}

static void *lck(void *,void **planes)
{
	int i=0;
	for(void *p:Render::instance()->getBuffer()){
		planes[i++]=p;
	}
	return NULL;
}

static void dsp(void *,void *)
{
	Render::instance()->releaseBuffer();
	QMetaObject::invokeMethod(APlayer::instance(),"decode");
}

static void sta(const libvlc_event_t *,void *)
{
	QMetaObject::invokeMethod(APlayer::instance(),"event",Q_ARG(int,VPlayer::Init));
}

static void mid(const libvlc_event_t *,void *)
{
	if (VPlayer::time.tryLock()) {
		QMetaObject::invokeMethod(APlayer::instance(),
								  "timeChanged",
								  Q_ARG(qint64,APlayer::instance()->getTime()));
		VPlayer::time.unlock();
	}
}

static void end(const libvlc_event_t *,void *)
{
	QMetaObject::invokeMethod(APlayer::instance(),"event",Q_ARG(int,VPlayer::Free));
}

VPlayer::VPlayer(QObject *parent):
	APlayer(parent)
{
	QList<QByteArray> args;
	for(QJsonValue arg:Config::getValue<QJsonArray>("/Playing/Arguments")){
		args.append(arg.toString().toUtf8());
	}
	if(!args.isEmpty()){
		const char **argv=new const char *[args.size()];
		for(int i=0;i<args.size();++i){
			argv[i]=args[i];
		}
		vlc=libvlc_new(args.size(),argv);
	}
	else{
		vlc=libvlc_new(0,NULL);
	}
#if (defined Q_OS_WIN)&&(defined QT_NO_DEBUG)
	libvlc_add_intf(vlc,"bililocal");
#endif
	m=NULL;
	mp=NULL;
	state=Stop;
	setObjectName("VPlayer");
	for(auto &iter:tracks){
		iter=new QActionGroup(this);
		iter->setExclusive(true);
	}
	ins=this;
}

VPlayer::~VPlayer()
{
	QMutex exit;
	libvlc_set_exit_handler(vlc,[](void *opaque){
		((QMutex *)opaque)->unlock();
	},&exit);
	exit.lock();
	if(mp){
		libvlc_media_player_release(mp);
	}
	if(m){
		libvlc_media_release(m);
	}
	libvlc_release(vlc);
	exit.lock();
	exit.unlock();
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

static void copyTracks(libvlc_track_description_t *head,QActionGroup *group)
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

void VPlayer::init()
{
	if(mp){
		auto *connection=new QMetaObject::Connection;
		*connection=QObject::connect(this,&VPlayer::timeChanged,[=](){
			if(state==Stop){
				setState(Play);
				bool music=true;
				libvlc_media_track_t **info;
				int n=libvlc_media_tracks_get(m,&info);
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
			}
			if(state==Loop){
				setState(Play);
				for(auto *g:tracks){
					for(QAction *i:g->actions()){
						if(i->isChecked()){
							i->trigger();
						}
					}
				}
			}
			setVolume(Config::getValue("/Playing/Volume",50));
			QObject::disconnect(*connection);
			delete connection;
		});
	}
}

void VPlayer::free()
{
	if(state==Play&&Config::getValue("/Playing/Loop",false)){
		libvlc_media_player_stop(mp);
		setState(Loop);
		libvlc_media_player_play(mp);
		emit jumped(0);
	}
	else{
		stop(false);
	}
}

void VPlayer::setState(int _state)
{
	state=_state;
	emit stateChanged(state);
}

void VPlayer::play()
{
	if(mp){
		if(state==Stop){
			libvlc_video_set_format_callbacks(mp,fmt,NULL);
			libvlc_video_set_callbacks(mp,lck,NULL,dsp,NULL);
			libvlc_media_player_play(mp);
		}
		else{
			libvlc_media_player_pause(mp);
			setState(state==Play?Pause:Play);
		}
	}
}

void VPlayer::stop(bool manually)
{
	if(mp&&state!=Stop){
		libvlc_media_player_stop(mp);
		setState(Stop);
		for(auto g:tracks){
			qDeleteAll(g->actions());
		}
		if(manually){
			Next::instance()->clear();
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
	if(m){
		libvlc_media_release(m);
	}
	if(mp){
		libvlc_media_player_release(mp);
	}
	m=libvlc_media_new_path(vlc,QDir::toNativeSeparators(_file).toUtf8());
	emit mediaChanged(m?getMedia():QString());
	if(m){
		mp=libvlc_media_player_new_from_media(m);
		if(mp){
			Config::setValue("/Playing/Path",QFileInfo(_file).absolutePath());
			libvlc_event_manager_t *man=libvlc_media_player_event_manager(mp);
			libvlc_event_attach(man,
								libvlc_MediaPlayerPlaying,
								sta,NULL);
			libvlc_event_attach(man,
								libvlc_MediaPlayerTimeChanged,
								mid,NULL);
			libvlc_event_attach(man,
								libvlc_MediaPlayerEndReached,
								end,NULL);
			if(Config::getValue("/Playing/Immediate",false)){
				play();
			}
		}
	}
}

QString VPlayer::getMedia()
{
	if(m){
		char *s=libvlc_media_get_mrl(m);
		QUrl u(s);
		libvlc_free(s);
		if(u.isLocalFile()){
			return u.toLocalFile();
		}
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
			QAction *action=tracks[2]->addAction(tr("Disable"));
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
	if(mp){
		_volume=qBound(0,_volume,100);
		libvlc_audio_set_volume(mp,_volume);
		emit volumeChanged(_volume);
	}
}

int VPlayer::getVolume()
{
	return mp?libvlc_audio_get_volume(mp):0;
}

void VPlayer::event(int type)
{
	switch(type){
	case Init:
		init();
		break;
	case Free:
		free();
		break;
	}
}
#endif

#ifdef BACKEND_QMM
#include <QtMultimedia>

static QString getFormat(QVideoFrame::PixelFormat format)
{
	switch(format){
	case QVideoFrame::Format_NV12:
		return "NV12";
	case QVideoFrame::Format_NV21:
		return "NV21";
	case QVideoFrame::Format_YV12:
		return "YV12";
	case QVideoFrame::Format_YUV420P:
		return "I420";
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
		if(!chroma.isEmpty()){
			QString buffer=chroma;
			Render::instance()->setBuffer(buffer,format.frameSize());
			if(buffer==chroma){
				QSize p=format.pixelAspectRatio();
				Render::instance()->setPixelAspectRatio(p.width()/(double)p.height());
				return true;
			}
		}
		return false;
	}

	bool present(const QVideoFrame &frame)
	{
		bool flag=false;
		QVideoFrame f(frame);
		if(f.map(QAbstractVideoBuffer::ReadOnly)){
			int len=f.mappedBytes();
			const quint8 *dat=f.bits();
			QList<quint8 *> buffer=Render::instance()->getBuffer();
			switch(frame.pixelFormat()){
			case QVideoFrame::Format_NV12:
			case QVideoFrame::Format_NV21:
				memcpy(buffer[0],dat,len*2/3);
				memcpy(buffer[1],dat+len*2/3,len*1/3);
				flag=true;
				break;
			case QVideoFrame::Format_YV12:
			case QVideoFrame::Format_YUV420P:
				memcpy(buffer[0],dat,len*2/3);
				memcpy(buffer[1],dat+len*2/3,len/6);
				memcpy(buffer[2],dat+len*5/6,len/6);
				flag=true;
				break;
			default:
				break;
			}
			Render::instance()->releaseBuffer();
			f.unmap();
		}
		QMetaObject::invokeMethod(APlayer::instance(),"decode");
		return flag;
	}

	QList<QVideoFrame::PixelFormat> supportedPixelFormats(QAbstractVideoBuffer::HandleType handleType) const
	{
		QList<QVideoFrame::PixelFormat> f;
		if(handleType==QAbstractVideoBuffer::NoHandle){
			f<<QVideoFrame::Format_NV12<<
			   QVideoFrame::Format_NV21<<
			   QVideoFrame::Format_YV12<<
			   QVideoFrame::Format_YUV420P;
		}
		return f;
	}
};

class PlayerThread:public QThread
{
public:
	explicit PlayerThread(QObject *parent=0):
		QThread(parent),mp(0)
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
		mp->setVideoOutput(new RenderAdapter(mp));
		m.unlock();
		w.wakeAll();
		exec();
		delete mp;
	}
};

class QPlayer:public APlayer
{
public:
	explicit QPlayer(QObject *parent=0);
	QList<QAction *> getTracks(int type);

private:
	QMediaPlayer *mp;
	bool manuallyStopped;
	bool waitingForBegin;

public slots:
	void	play();
	void	stop(bool manually=true);
	int 	getState();

	void	setTime(qint64 _time);
	qint64	getTime();

	void	setMedia(QString _file,bool manually=true);
	QString getMedia();

	qint64	getDuration();
	void	addSubtitle(QString _file);

	void	setVolume(int _volume);
	int 	getVolume();

	void	event(int type);

};

QPlayer::QPlayer(QObject *parent):
	APlayer(parent)
{
	mp=(new PlayerThread(this))->getMediaPlayer();
	connect(mp,&QMediaPlayer::stateChanged,		this,&QPlayer::stateChanged	);
	connect(mp,&QMediaPlayer::positionChanged,	this,&QPlayer::timeChanged	);

	connect(mp,&QMediaPlayer::videoAvailableChanged,[this](bool a){
		Render::instance()->setMusic(!a);
	});

	manuallyStopped=false;
	connect(this,&QPlayer::stateChanged,[=](int state){
		static int lastState;
		if(state==Stop){
			if(!manuallyStopped&&Config::getValue("/Playing/Loop",false)){
				lastState=Loop;
				play();
			}
			else{
				emit reach(manuallyStopped);
			}
		}
		manuallyStopped=false;
		if(state==Play&&lastState==Stop){
			waitingForBegin=true;
		}
		lastState=state;
	});

	connect(this,&QPlayer::timeChanged,[this](qint64 t){
		if (waitingForBegin&&t>0){
			waitingForBegin=false;
			emit begin();
		}
	});
	
	setObjectName("QPlayer");
	ins=this;
}

QList<QAction *> QPlayer::getTracks(int)
{
	return QList<QAction *>();
}

void QPlayer::play()
{
	QMetaObject::invokeMethod(mp,getState()==Play?"pause":"play",Qt::BlockingQueuedConnection);
}

void QPlayer::stop(bool manually)
{
	if(manually){
		Next::instance()->clear();
	}
	manuallyStopped=manually;
	QMetaObject::invokeMethod(mp,"stop",Qt::BlockingQueuedConnection);
}

int QPlayer::getState()
{
	return waitingForBegin?(int)Stop:(int)mp->state();
}

void QPlayer::setTime(qint64 _time)
{
	mp->setPosition(_time);
	qApp->processEvents();
	emit jumped(_time);
}

qint64 QPlayer::getTime()
{
	return mp->position();
}

void QPlayer::setMedia(QString _file,bool manually)
{
	stop(manually);
	QMetaObject::invokeMethod(mp,"setMedia",Qt::BlockingQueuedConnection,Q_ARG(QMediaContent,QUrl::fromLocalFile(_file)));
	Config::setValue("/Playing/Path", QFileInfo(_file).absolutePath());
	emit mediaChanged(getMedia());
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
	mp->setVolume(_volume);
}

int QPlayer::getVolume()
{
	return mp->volume();
}

void QPlayer::event(int)
{
}
#endif

APlayer *APlayer::instance()
{
	if(ins){
		return ins;
	}
#if (defined BACKEND_VLC)&&(defined BACKEND_QMM)
	if(Config::getValue("/Playing/Native",false)){
		return new QPlayer(Local::mainWidget());
	}
	else{
		return new VPlayer(Local::mainWidget());
	}
#endif
#if (defined BACKEND_QMM)&&(!(defined BACKEND_VLC))
	return new QPlayer(Local::mainWidget());
#endif
#if (defined BACKEND_VLC)&&(!(defined BACKEND_QMM))
	return new VPlayer(Local::mainWidget());
#endif
}
