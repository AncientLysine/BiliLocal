/*=======================================================================
*
*   Copyright (C) 2013 Lysine.
*
*   Filename:    VPlayer.h
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

#ifndef VPLAYER_H
#define VPLAYER_H

#include <QtGui>
#include <QtCore>

extern "C"
{
#include <vlc/vlc.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

class VPlayer:public QObject
{
	Q_OBJECT
public:
	enum {Stop,Play,Pause,Loop,Source,Scaled,Destinate};

	explicit VPlayer(QObject *parent=0);
	~VPlayer();
	uchar *getSrc();
	uchar *getDst();
	int getState();
	qint64 getTime();
	qint64 getDuration();
	QSize getSize(int t=Source);
	QString getFile(){return file;}
	QList<QAction *> getSubtitles(){return subtitle;}
	QList<QAction *> getVideoTracks(){return video;}
	QList<QAction *> getAudioTracks(){return audio;}
	void setFrame(bool force=false);
	void draw(QPainter *painter,QRect rect);
	static VPlayer *instance(){return ins;}

private:
	int state;
	bool soundOnly;
	double ratio;
	QMutex data;
	QMutex size;
	QSize srcSize;
	QSize dstSize;
	QSize guiSize;
	QPixmap frame;
	QPixmap sound;
	QTimer *fake;
	QString file;
	QList<QAction *> subtitle;
	QList<QAction *> video;
	QList<QAction *> audio;
	libvlc_instance_t *vlc;
	libvlc_media_t *m;
	libvlc_media_player_t *mp;
	SwsContext *swsctx;
	AVPicture *srcFrame;
	AVPicture *dstFrame;
	static VPlayer *ins;

	void setState(int _state);

signals:
	void begin();
	void reach();
	void reset();
	void decode();
	void jumped(qint64 _time);
	void stateChanged(int _state);

public slots:
	void play();
	void stop();
	void init();
	void free();
	void setSize(QSize _size);
	void setTime(qint64 _time);
	void setFile(QString _file);
	void setRatio(double _ratio);
	void setVolume(int _volume);
};

#endif // VPLAYER_H

