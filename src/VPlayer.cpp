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
	*planes=player->getBit();
	return NULL;
}

static void display(void *opaque,void *)
{
	VPlayer *player=static_cast<VPlayer *>(opaque);
	player->bufferFrame();
}

VPlayer::VPlayer(QObject *parent) :
	QObject(parent)
{
	vlc=libvlc_new(0,NULL);
	m=NULL;
	mp=NULL;
	swsctx=NULL;
	srcFrame=NULL;
	dstFrame=NULL;
	state=Stop;
	open=false;
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
	}
	if(dstFrame){
		avpicture_free(dstFrame);
	}
}

uchar *VPlayer::getBit()
{
	return (uchar *)*srcFrame->data;
}

QSize VPlayer::getSize()
{
	return srcSize;
}

qint64 VPlayer::getTime()
{
	return state==Stop?-1:libvlc_media_player_get_time(mp);
}

int VPlayer::getState()
{
	return state;
}

qint64 VPlayer::getDuration()
{
	return mp?libvlc_media_player_get_length(mp):-1;
}

void VPlayer::bufferFrame()
{
	QTimer::singleShot(0,this,SLOT(emitFrame()));
}

void VPlayer::draw(QPainter *painter,QRect rect)
{
	painter->drawPixmap(rect.center()-QRect(QPoint(0,0),buffer.size()).center(),buffer);
}

void VPlayer::play()
{
	if(mp){
		if(state==Stop){
			libvlc_media_track_info_t *info;
			libvlc_media_parse(m);
			int i=libvlc_media_get_tracks_info(m,&info);
			for(--i;i>=0;--i){
				if(info[i].i_type==libvlc_track_video){
					srcSize.setWidth (info[i].u.video.i_width);
					srcSize.setHeight(info[i].u.video.i_height);
				}
			}
			free(info);
			open=true;
			dstSize=srcSize;
			srcFrame=new AVPicture;
			dstFrame=new AVPicture;
			avpicture_alloc(srcFrame,PIX_FMT_RGB32,srcSize.width(),srcSize.height());
			avpicture_alloc(dstFrame,PIX_FMT_RGB32,dstSize.width(),dstSize.height());
			libvlc_video_set_format(mp,"RV32",srcSize.width(),srcSize.height(),srcSize.width()*4);
			libvlc_video_set_callbacks(mp,lock,NULL,display,this);
			QTimer *delay=new QTimer(this);
			delay->setSingleShot(true);
			delay->start(50);
			connect(delay,&QTimer::timeout,[this](){
				libvlc_media_player_play(mp);
			});
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
			libvlc_media_player_stop(mp);
			QTimer *delay=new QTimer(this);
			delay->setSingleShot(true);
			delay->start(50);
			connect(delay,&QTimer::timeout,[this](){
				state=Stop;
				open=false;
				buffer=QPixmap();
				emit ended();
			});
		}
	}
}

void VPlayer::setSize(QSize _size)
{
	dstSize=srcSize.scaled(_size,Qt::KeepAspectRatio);
	dstSize/=4;
	dstSize*=4;
	if(open){
		avpicture_free(dstFrame);
		avpicture_alloc(dstFrame,PIX_FMT_RGB32,dstSize.width(),dstSize.height());
		if(state==Pause){
			state=Play;
			emitFrame();
			state=Pause;
		}
	}
}

void VPlayer::setTime(qint64 _time)
{
	if(mp){
		libvlc_media_player_set_time(mp,_time);
		emit jumped(_time);
	}
}

void VPlayer::setFile(QString _file)
{
	stop();
	file=_file;
	if(m){
		libvlc_media_release(m);
	}
	if(mp){
		libvlc_media_player_release(mp);
	}
	m=libvlc_media_new_path(vlc,file.toUtf8());
	if(m){
		mp=libvlc_media_player_new_from_media(m);
	}
}

void VPlayer::setVolume(int _volume)
{
	if(mp){
		libvlc_audio_set_volume(mp,_volume);
	}
}

void VPlayer::emitFrame()
{
	if(state==Stop){
		state=Play;
		emit opened();
	}
	if(state==Play){
		swsctx=sws_getCachedContext(swsctx,
									srcSize.width(),srcSize.height(),PIX_FMT_RGB32,
									dstSize.width(),dstSize.height(),PIX_FMT_RGB32,
									SWS_FAST_BILINEAR,NULL,NULL,NULL);
		sws_scale(swsctx,srcFrame->data,srcFrame->linesize,0,srcSize.height(),dstFrame->data,dstFrame->linesize);
		QImage frame((uchar *)*dstFrame->data,dstSize.width(),dstSize.height(),QImage::Format_RGB32);
		buffer=QPixmap::fromImage(frame.copy());
		emit decoded();
	}
	if(getDuration()-getTime()<500){
		stop();
	}
}
