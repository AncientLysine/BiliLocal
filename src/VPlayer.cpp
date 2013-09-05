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

VPlayer*VPlayer::ins=NULL;

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

static void *lock(void *opaque,void **planes)
{
	VPlayer *player=static_cast<VPlayer *>(opaque);
	*planes=player->getSrc();
	return NULL;
}

static void display(void *opaque,void *)
{
	VPlayer *player=static_cast<VPlayer *>(opaque);
	player->setFrame();
}

static void log(void *,int level,const libvlc_log_t *,const char *fmt,va_list args)
{
	if(level>0){
		char *string=new char[1024];
		vsprintf(string,fmt,args);
		Printer::instance()->append(QString("[VPlayer]%1").arg(string));
		delete []string;
	}
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
	ins=this;
	connect(this,SIGNAL(rendered()),this,SLOT(emitFrame()));
}

VPlayer::~VPlayer()
{
	libvlc_release(vlc);
	if(m){
		libvlc_media_release(m);
	}
	if(mp){
		libvlc_media_player_release(mp);
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
}

uchar *VPlayer::getSrc()
{
	return (uchar *)*srcFrame->data;
}

uchar *VPlayer::getDst()
{
	return (uchar *)*dstFrame->data;
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

qint64 VPlayer::getTime()
{
	return state==Stop?-1:libvlc_media_player_get_time(mp);
}

int VPlayer::getState()
{
	return state;
}

int VPlayer::getSubtitle()
{
	return mp?libvlc_video_get_spu(mp):-1;
}

qint64 VPlayer::getDuration()
{
	return mp?libvlc_media_player_get_length(mp):-1;
}

QMap<int,QString> VPlayer::getSubtitles()
{
	QMap<int,QString> map;
	if(mp){
		libvlc_track_description_t *list=libvlc_video_get_spu_description(mp);
		libvlc_track_description_t *iter=list;
		while(iter){
			QString title=iter->psz_name;
			title.replace("Track"  ,tr("Track"));
			title.replace("Disable",tr("Disable"));
			map[iter->i_id]=title;
			iter=iter->p_next;
		}
		libvlc_track_description_list_release(list);
	}
	return map;
}

void VPlayer::setFrame(bool force)
{
	if(state!=Pause||force){
		mutex.lock();
		if(dstSize!=guiSize){
			dstSize=guiSize;
			avpicture_free (dstFrame);
			avpicture_alloc(dstFrame,PIX_FMT_RGB32,dstSize.width(),dstSize.height());
		}
		swsctx=sws_getCachedContext(swsctx,
									srcSize.width(),srcSize.height(),PIX_FMT_RGB32,
									dstSize.width(),dstSize.height(),PIX_FMT_RGB32,
									SWS_FAST_BILINEAR,NULL,NULL,NULL);
		sws_scale(swsctx,srcFrame->data,srcFrame->linesize,0,srcSize.height(),dstFrame->data,dstFrame->linesize);
		frame=QPixmap::fromImage(QImage(getDst(),dstSize.width(),dstSize.height(),QImage::Format_RGB32).copy());
		mutex.unlock();
		emit rendered();
	}
}

void VPlayer::draw(QPainter *painter,QRect rect)
{
	mutex.lock();
	painter->drawPixmap(rect.center()-QRect(QPoint(0,0),frame.size()).center(),frame);
	mutex.unlock();
}

void VPlayer::play()
{
	if(mp){
		if(state==Stop){
			libvlc_media_track_t **info;
			libvlc_media_parse(m);
			int n=libvlc_media_tracks_get(m,&info);
			for(int i=0;i<n;++i){
				if(info[i]->i_type==libvlc_track_video){
					libvlc_video_track_t *v=info[i]->video;
					double r=v->i_sar_den==0?1:(double)v->i_sar_num/v->i_sar_den;
					dstSize=guiSize=srcSize=QSize(v->i_width*r,v->i_height);
					break;
				}
			}
			libvlc_media_tracks_release(info,n);
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
			libvlc_video_set_callbacks(mp,lock,NULL,display,this);
			libvlc_media_player_play(mp);
		}
		else{
			libvlc_media_player_pause(mp);
			if(state==Play){
				state=Pause;
				emit paused();
			}
			else{
				state=Play;
			}
		}
	}
}

void VPlayer::stop()
{
	if(mp){
		if(state!=Stop){
			state=Invalid;
			libvlc_media_player_stop(mp);
			qApp->processEvents();
			state=Stop;
			frame=QPixmap();
			emit ended();
		}
	}
}

void VPlayer::setSize(QSize _size)
{
	if(state==Play||state==Pause){
		mutex.lock();
		if(ratio>0){
			int w=qMax<int>(_size.width(),_size.height()*ratio);
			guiSize=QSize(w,w/ratio);
		}
		else{
			guiSize=srcSize.scaled(_size,Qt::KeepAspectRatio);
		}
		guiSize/=4;
		guiSize*=4;
		if(guiSize!=dstSize&&state==Pause){
			mutex.unlock();
			setFrame(true);
		}
		else{
			mutex.unlock();
		}
	}
}

void VPlayer::setTime(qint64 _time)
{
	if(mp){
		_time=qBound<qint64>(0,_time,getDuration()-500);
		libvlc_media_player_set_time(mp,_time);
		emit jumped(_time);
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
	m=libvlc_media_new_path(vlc,_file.toUtf8());
	if(m){
		mp=libvlc_media_player_new_from_media(m);
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

void VPlayer::setSubTitle(int _track)
{
	if(mp){
		libvlc_video_set_spu(mp,_track);
	}
}

void VPlayer::emitFrame()
{
	if(state==Stop){
		state=Play;
		emit opened();
	}
	if(state==Play){
		emit decoded();
		if(getDuration()-getTime()<500){
			if(Utils::getConfig("/Playing/Loop",false)){
				setTime(0);
			}
			else{
				stop();
			}
		}
	}
}
