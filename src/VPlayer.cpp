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
	*planes=player->getSrc();
	return NULL;
}

static void display(void *opaque,void *)
{
	VPlayer *player=static_cast<VPlayer *>(opaque);
	player->setFrame();
}

VPlayer::VPlayer(QObject *parent) :
	QObject(parent)
{
	vlc=libvlc_new(0,NULL);
	m=NULL;
	mp=NULL;
	swsctx=NULL;
	state=Stop;
	valid=false;
	ratio=0;
	connect(this,SIGNAL(rendered(QImage)),this,SLOT(emitFrame(QImage)));
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
}

uchar *VPlayer::getSrc()
{
	return (uchar *)*srcFrame.data;
}

uchar *VPlayer::getDst()
{
	return (uchar *)*dstFrame.data;
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

QString VPlayer::getSubtitle()
{
	if(mp){
		if(subtitle.isEmpty()){
			getSubtitles();
		}
		return subtitle.key(libvlc_video_get_spu(mp));
	}
	else{
		return QString();
	}
}

QStringList VPlayer::getSubtitles()
{
	if(mp&&subtitle.isEmpty()){
		auto list=libvlc_video_get_spu_description(mp);
		auto iter=list;
		while(iter){
			QString title=iter->psz_name;
			title.replace("Track"  ,tr("Track"));
			title.replace("Disable",tr("Disable"));
			subtitle[title]=iter->i_id;
			iter=iter->p_next;
		}
		libvlc_track_description_list_release(list);
	}
	return subtitle.keys();
}

void VPlayer::setFrame()
{
	if(state!=Pause){
		mutex.lock();
		swsctx=sws_getCachedContext(swsctx,
									srcSize.width(),srcSize.height(),PIX_FMT_RGB32,
									dstSize.width(),dstSize.height(),PIX_FMT_RGB32,
									SWS_FAST_BILINEAR,NULL,NULL,NULL);
		if(!valid){
			avpicture_free (&dstFrame);
			avpicture_alloc(&dstFrame,PIX_FMT_RGB32,dstSize.width(),dstSize.height());
			valid=true;
		}
		sws_scale(swsctx,srcFrame.data,srcFrame.linesize,0,srcSize.height(),dstFrame.data,dstFrame.linesize);
		QImage frame=QImage(getDst(),dstSize.width(),dstSize.height(),QImage::Format_RGB32).copy();
		mutex.unlock();
		emit rendered(frame);
	}
}

void VPlayer::draw(QPainter *painter,QRect rect)
{
	painter->drawPixmap(rect.center()-QRect(QPoint(0,0),frame.size()).center(),frame);
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
			dstSize=srcSize;
			avpicture_alloc(&srcFrame,PIX_FMT_RGB32,srcSize.width(),srcSize.height());
			avpicture_alloc(&dstFrame,PIX_FMT_RGB32,dstSize.width(),dstSize.height());
			valid=true;
			libvlc_video_set_format(mp,"RV32",srcSize.width(),srcSize.height(),srcSize.width()*4);
			libvlc_video_set_callbacks(mp,lock,NULL,display,this);
			Utils::delayExec(50,[this](){libvlc_media_player_play(mp);});
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
			Utils::delayExec(50,[this](){
				state=Stop;
				frame=QPixmap();
				subtitle.clear();
				emit ended();
			});
		}
	}
}

void VPlayer::setSize(QSize _size)
{
	mutex.lock();
	auto _dstSize=dstSize;
	if(ratio>0){
		int w=_size.width(),h=_size.height()*ratio;
		int sw=w>h?h:w;
		dstSize=QSize(sw,sw/ratio);
	}
	else{
		dstSize=srcSize.scaled(_size,Qt::KeepAspectRatio);
	}
	dstSize/=4;
	dstSize*=4;
	if(_dstSize!=dstSize){
		valid=false;
		mutex.unlock();
		if(state==Pause){
			state=Play;
			setFrame();
			state=Pause;
		}
	}
	else{
		mutex.unlock();
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

void VPlayer::setSubTitle(QString _track)
{
	if(mp){
		libvlc_video_set_spu(mp,subtitle[_track]);
	}
}

void VPlayer::emitFrame(QImage _frame)
{
	if(state==Stop){
		state=Play;
		emit opened();
	}
	if(state==Play){
		frame=QPixmap::fromImage(_frame);
		emit decoded();
		if(getDuration()-getTime()<500){
			stop();
		}
	}
}
