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

#ifdef Q_OS_WIN32
#include <winbase.h>
#endif

VPlayer *VPlayer::ins=NULL;

static void *lck(void *,void **planes)
{
	*planes=VPlayer::instance()->getBuffer();
	return NULL;
}

static void dsp(void *,void *)
{
	VPlayer::instance()->setFrame();
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
	m=NULL;
	mp=NULL;
	frame=0;
	index=0;
	buffer[0]=buffer[1]=NULL;
	state=Stop;
	ratio=0;
	music=false;
	fake=new QTimer(this);
	fake->setInterval(33);
	fake->setTimerType(Qt::PreciseTimer);
	connect(fake,&QTimer::timeout,this,&VPlayer::decode);
	sound=QImage(":/Picture/sound.png");
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
	if(frame){
		glDeleteTextures(1,&frame);
	}
	if(buffer[0]){
		delete buffer[0];
	}
	if(buffer[1]){
		delete buffer[1];
	}
	libvlc_release(vlc);
}

qint64 VPlayer::getTime()
{
	return state==Stop?-1:libvlc_media_player_get_time(mp);
}

qint64 VPlayer::getDuration()
{
	return mp?libvlc_media_player_get_length(mp):-1;
}

void VPlayer::setFrame()
{
	if(state!=Pause){
		data.lock();
		index=(!(index&1))|2;
		data.unlock();
		emit decode();
	}
}

void VPlayer::draw(QPainter *painter,QRect rect)
{
	if(state!=Stop){
		painter->fillRect(rect,Qt::black);
		if(music){
			painter->drawImage(rect.center()-QRect(QPoint(0,0),sound.size()).center(),sound);
		}
		else{
			int w=size.width(),h=size.height();
			QRect dest;
			dest.setSize((ratio>0?QSizeF(ratio,1):QSizeF(size)).scaled(rect.size(),Qt::KeepAspectRatio).toSize());
			dest.moveCenter(rect.center());
			data.lock();
			painter->beginNativePainting();
			if(index&2){
				index&=1;
				if(!frame){
					glGenTextures(1,&frame);
				};
				glBindTexture(GL_TEXTURE_2D,frame);
				glTexImage2D(GL_TEXTURE_2D,0,GL_RGB,w,h,0,GL_BGRA,GL_UNSIGNED_BYTE,buffer[!index]);
			}
			else{
				glBindTexture(GL_TEXTURE_2D,frame);
			}
			glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
			GLfloat l=dest.left(),t=dest.top(),r=dest.right()+1,b=dest.bottom()+1;
			GLfloat vtx[8]={l,t,r,t,r,b,l,b};
			GLfloat tex[8]={0,0,1,0,1,1,0,1};
			glVertexPointer(2,GL_FLOAT,0,vtx);
			glTexCoordPointer(2,GL_FLOAT,0,tex);
			glDrawArrays(GL_TRIANGLE_FAN,0,4);
			painter->endNativePainting();
			data.unlock();
		}
	}
}

void VPlayer::setState(State _state)
{
#ifdef Q_OS_WIN32
	if (_state == Play || _state == Loop)
	{
		SetThreadExecutionState(ES_DISPLAY_REQUIRED | ES_SYSTEM_REQUIRED | ES_CONTINUOUS);
	}
	else
	{
		SetThreadExecutionState(ES_CONTINUOUS);
	}
#endif
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
			music=true;
			size=QSize();
			for(int i=0;i<n;++i){
				if(info[i]->i_type==libvlc_track_video){
					libvlc_video_track_t *v=info[i]->video;
					music=false;
					double r=v->i_sar_den==0?1:(double)v->i_sar_num/v->i_sar_den;
					size=QSize(v->i_width*r,v->i_height);
					break;
				}
			}
			libvlc_media_tracks_release(info,n);
			if(music){
				fake->start();
			}
			if(buffer[0]){
				delete buffer[0];
			}
			if(buffer[1]){
				delete buffer[1];
			}
			buffer[0]=new uchar[size.width()*size.height()*4];
			buffer[1]=new uchar[size.width()*size.height()*4];
			libvlc_video_set_format(mp,"RV32",size.width(),size.height(),size.width()*4);
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
		size=QSize();
		ratio=0;
		setState(Stop);
		if(music){
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
						music=1;
					}
					else{
						fake->stop();
						music=0;
					}
				});
				i->setChecked(t==libvlc_video_get_track(mp));
			}
			for(QAction *i:audio){
				connect(i,&QAction::triggered,[=](){libvlc_audio_set_track(mp,i->data().toInt());});
				i->setChecked(i->data().toInt()==libvlc_audio_get_track(mp));
			}
			QMetaObject::Connection *connect=new QMetaObject::Connection;
			*connect=QObject::connect(this,&VPlayer::decode,[=](){
				setVolume(Utils::getConfig("/Playing/Volume",100));
				QObject::disconnect(*connect);
				delete connect;
			});
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
