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

#include "VPlayer.h"
#include "Utils.h"
#include "Printer.h"

VPlayer *VPlayer::ins=NULL;

int avpicture_alloc(AVPicture *picture,enum AVPixelFormat pix_fmt,int width,int height)
{
	int ret=av_image_alloc(picture->data,picture->linesize,width,height,pix_fmt,1);
	if(ret<0){
		memset(picture,0,sizeof(AVPicture));
		return ret;
	}
	return 0;
}

void avpicture_free(AVPicture *picture)
{
	av_free(picture->data[0]);
}

static void *lck(void *,void **planes)
{
	*planes=VPlayer::instance()->getSrc();
	return NULL;
}

static void dsp(void *,void *)
{
	VPlayer::instance()->setFrame();
}

static void log(void *,int level,const libvlc_log_t *,const char *fmt,va_list args)
{
	if(level>0){
		char *string=new char[1024];
#ifdef Q_CC_MSVC
		vsprintf_s(string,1024,fmt,args);
#else
		vsprintf(string,fmt,args);
#endif
		Printer::instance()->append(QString("[VPlayer]%1").arg(string));
		delete []string;
	}
}

static void sta(const struct libvlc_event_t *,void *)
{
	QMetaObject::invokeMethod(VPlayer::instance(),"init");
}

static void end(const struct libvlc_event_t *,void *)
{
	QMetaObject::invokeMethod(VPlayer::instance(),"free");
}

VPlayer::VPlayer(QObject *parent) :
	QObject(parent)
{
	vlc=libvlc_new(0,NULL);
	libvlc_log_set(vlc,log,NULL);
	m=NULL;
	mp=NULL;
	swsctx=NULL;
	srcFrame=NULL;
	dstFrame=NULL;
	state=Stop;
	ratio=0;
	soundOnly=false;
	fake=new QTimer(this);
	fake->setInterval(33);
	fake->setTimerType(Qt::PreciseTimer);
	connect(fake,&QTimer::timeout,this,&VPlayer::decode);
	sound=QPixmap(":/Picture/sound.png");
	ins=this;
}

VPlayer::~VPlayer()
{
	if(mp){
		libvlc_media_player_release(mp);
	}
	if(m){
		libvlc_media_release(m);
	}
	if(swsctx){
		sws_freeContext(swsctx);
	}
	if(srcFrame){
		avpicture_free(srcFrame);
		delete srcFrame;
	}
	if(dstFrame){
		avpicture_free(dstFrame);
		delete dstFrame;
	}
	libvlc_release(vlc);
}

uchar *VPlayer::getSrc()
{
	return (uchar *)*srcFrame->data;
}

uchar *VPlayer::getDst()
{
	return (uchar *)*dstFrame->data;
}

int VPlayer::getState()
{
	return state;
}

qint64 VPlayer::getTime()
{
	return state==Stop?-1:libvlc_media_player_get_time(mp);
}

qint64 VPlayer::getDuration()
{
	return mp?libvlc_media_player_get_length(mp):-1;
}

QSize VPlayer::getSize(int t)
{
	switch(t){
	case Scaled:
	{
		if(ratio==0){
			return srcSize;
		}
		else{
			int w=srcSize.width(),h=srcSize.height();
			return h*ratio>w?QSize(w,w/ratio):QSize(h*ratio,h);
		}
	}
	case Destinate:
	{
		return guiSize;
	}
	default:
	{
		return srcSize;
	}
	}
}

void VPlayer::setFrame(bool force)
{
	if(state!=Pause||force){
		size.lock();
		if(dstSize!=guiSize){
			dstSize=guiSize;
			avpicture_free (dstFrame);
			avpicture_alloc(dstFrame,PIX_FMT_RGB32,dstSize.width(),dstSize.height());
		}
		swsctx=sws_getCachedContext(swsctx,
									srcSize.width(),srcSize.height(),PIX_FMT_RGB32,
									dstSize.width(),dstSize.height(),PIX_FMT_RGB32,
									SWS_FAST_BILINEAR,NULL,NULL,NULL);
		int height=srcSize.height();
		size.unlock();
		sws_scale(swsctx,srcFrame->data,srcFrame->linesize,0,height,dstFrame->data,dstFrame->linesize);
		QPixmap buf=QPixmap::fromImage(QImage(getDst(),dstSize.width(),dstSize.height(),QImage::Format_RGB32).copy());
		data.lock();
		frame=buf;
		data.unlock();
		emit decode();
	}
}

void VPlayer::draw(QPainter *painter,QRect rect)
{
	if(state!=Stop){
		painter->fillRect(rect,Qt::black);
		if(soundOnly){
			painter->drawPixmap(rect.center()-QRect(QPoint(0,0),sound.size()).center(),sound);
		}
		else{
			data.lock();
			painter->drawPixmap(rect.center()-QRect(QPoint(0,0),frame.size()).center(),frame);
			data.unlock();
		}
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
			libvlc_media_track_t **info;
			libvlc_media_parse(m);
			int n=libvlc_media_tracks_get(m,&info);
			soundOnly=true;
			dstSize=guiSize=srcSize=QSize();
			for(int i=0;i<n;++i){
				if(info[i]->i_type==libvlc_track_video){
					libvlc_video_track_t *v=info[i]->video;
					soundOnly=false;
					double r=v->i_sar_den==0?1:(double)v->i_sar_num/v->i_sar_den;
					dstSize=guiSize=srcSize=QSize(v->i_width*r,v->i_height);
					break;
				}
			}
			libvlc_media_tracks_release(info,n);
			if(soundOnly){
				fake->start();
			}
			if(srcFrame){
				avpicture_free(srcFrame);
			}
			if(dstFrame){
				avpicture_free(dstFrame);
			}
			srcFrame=new AVPicture;
			dstFrame=new AVPicture;
			avpicture_alloc(srcFrame,PIX_FMT_RGB32,srcSize.width(),srcSize.height());
			avpicture_alloc(dstFrame,PIX_FMT_RGB32,dstSize.width(),dstSize.height());
			libvlc_video_set_format(mp,"RV32",srcSize.width(),srcSize.height(),srcSize.width()*4);
			libvlc_video_set_callbacks(mp,lck,NULL,dsp,NULL);
			libvlc_media_player_play(mp);
		}
		else{
			libvlc_media_player_pause(mp);
			setState(state==Play?Pause:Play);
		}
	}
}

void VPlayer::stop()
{
	if(mp&&state!=Stop){
		libvlc_media_player_stop(mp);
		setState(Stop);
		frame=QPixmap();
		if(soundOnly){
			fake->stop();
		}
		emit reach();
	}
}

void VPlayer::init()
{
	if(mp){
		if(state==Stop){
			setState(Play);
			if(!Utils::getConfig("/Playing/Subtitle",true)){
				libvlc_video_set_spu(mp,-1);
			}
			auto transTracks=[this](libvlc_track_description_t *head)
			{
				libvlc_track_description_t *iter=head;
				QActionGroup *group=new QActionGroup(this);
				group->setExclusive(true);
				while(iter){
					QString title=iter->psz_name;
					title.replace("Track"  ,tr("Track"));
					title.replace("Disable",tr("Disable"));
					QAction *action=group->addAction(title);
					action->setCheckable(true);
					action->setData(iter->i_id);
					iter=iter->p_next;
				}
				libvlc_track_description_list_release(head);
				return group->actions();
			};
			subtitle=transTracks(libvlc_video_get_spu_description(mp));
			video=transTracks(libvlc_video_get_track_description(mp));
			audio=transTracks(libvlc_audio_get_track_description(mp));
			for(QAction *i:subtitle){
				connect(i,&QAction::triggered,[=](){libvlc_video_set_spu(mp,i->data().toInt());});
				i->setChecked(i->data().toInt()==libvlc_video_get_spu(mp));
			}
			for(QAction *i:video){
				int t=i->data().toInt();
				connect(i,&QAction::triggered,[=](){
					libvlc_video_set_track(mp,t);
					if(t==-1){
						fake->start();
						soundOnly=1;
					}
					else{
						fake->stop();
						soundOnly=0;
					}
				});
				i->setChecked(t==libvlc_video_get_track(mp));
			}
			for(QAction *i:audio){
				connect(i,&QAction::triggered,[=](){libvlc_audio_set_track(mp,i->data().toInt());});
				i->setChecked(i->data().toInt()==libvlc_audio_get_track(mp));
			}
			Utils::delayExec(this,&VPlayer::decode,[this](){setVolume(Utils::getConfig("/Playing/Volume",100));});
			emit begin();
		}
		if(state==Loop){
			setState(Play);
			for(QAction *i:subtitle){
				if(i->isChecked()) libvlc_video_set_spu(mp,i->data().toInt());
			}
			for(QAction *i:video){
				if(i->isChecked()) libvlc_video_set_track(mp,i->data().toInt());
			}
			for(QAction *i:audio){
				if(i->isChecked()) libvlc_audio_set_track(mp,i->data().toInt());
			}
			emit reset();
		}
	}
}

void VPlayer::free()
{
	if(Utils::getConfig("/Playing/Loop",false)){
		libvlc_media_player_stop(mp);
		setState(Loop);
		libvlc_media_player_play(mp);
		emit jumped(0);
	}
	else{
		stop();
		auto clear=[](QList<QAction *> &list){
			for(QAction *i:list){
				delete i;
			}
			list.clear();
		};
		clear(video);
		clear(audio);
		clear(subtitle);
	}
}

void VPlayer::setSize(QSize _size)
{
	if(state==Play||state==Pause){
		size.lock();
		if(ratio>0){
			int w=qMin<int>(_size.width(),_size.height()*ratio);
			guiSize=QSize(w,w/ratio);
		}
		else{
			guiSize=srcSize.scaled(_size,Qt::KeepAspectRatio);
		}
		guiSize/=4;
		guiSize*=4;
		bool flag=guiSize!=dstSize&&state==Pause;
		size.unlock();
		if(flag){
			setFrame(true);
		}
	}
}

void VPlayer::setTime(qint64 _time)
{
	if(mp){
		if(getDuration()==_time){
			if(Utils::getConfig("/Playing/Loop",false)){
				setTime(0);
			}
			else{
				stop();
			}
		}
		else{
			libvlc_media_player_set_time(mp,qBound<qint64>(0,_time,getDuration()));
			emit jumped(_time);
		}
	}
}

void VPlayer::setFile(QString _file)
{
	stop();
	if(m){
		libvlc_media_release(m);
	}
	if(mp){
		libvlc_media_player_release(mp);
	}
	file=_file;
	m=libvlc_media_new_path(vlc,file.toUtf8());
	if(m){
		mp=libvlc_media_player_new_from_media(m);
		if(mp){
			libvlc_event_manager_t *man=libvlc_media_player_event_manager(mp);
			libvlc_event_attach(man,libvlc_MediaPlayerPlaying   ,sta,NULL);
			libvlc_event_attach(man,libvlc_MediaPlayerEndReached,end,NULL);
		}
	}
}

void VPlayer::setRatio(double _ratio)
{
	ratio=_ratio;
}

void VPlayer::setVolume(int _volume)
{
	if(mp){
		libvlc_audio_set_volume(mp,_volume);
	}
}
