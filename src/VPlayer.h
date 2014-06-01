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

class QActionGroup;

class VPlayer:public QObject
{
	Q_OBJECT
public:
	enum State
	{
		Stop,
		Play,
		Pause,
		Loop
	};
	~VPlayer();
	QList<QAction *> getTracks(int type);

	virtual void getBuffer(void **planes)=0;
	virtual void setBuffer(char *chroma,unsigned *width,unsigned *height,unsigned *pitches,unsigned *lines)=0;
	virtual void draw(QPainter *painter,QRect rect)=0;

	static QMutex data;
	static QMutex time;
	static VPlayer *instance();

private:
	int state;
	QTimer *fake;
	double ratio;
	QActionGroup *tracks[3];
	libvlc_instance_t *vlc;
	libvlc_media_t *m;
	libvlc_media_player_t *mp;

protected:
	bool start;
	bool music;
	bool dirty;
	QSize size;
	QImage sound;
	static VPlayer *ins;

	VPlayer(QObject *parent=0);
	QRect getRect(QRect rect);

signals:
	void begin();
	void reach(bool);
	void decode();
	void jumped(qint64);
	void timeChanged(qint64);
	void stateChanged(int);
	void mediaChanged(QString);
	void volumeChanged(int);


protected slots:
	void init();
	void free();
	void release();
	void setState(int _state);

public slots:
	qint64 getTime();
	qint64 getDuration();
	QString getFile();
	QSize getSize(){return size;}
	int getState(){return state;}

	void play();
	void stop(bool manually=true);
	void setDirty();
	void setTime(qint64 _time);
	void setMedia(QString _file,bool manually=true);
	void setRatio(double _ratio);
	void setVolume(int _volume);
	void addSubtitle(QString _file);
};

#endif // VPLAYER_H
