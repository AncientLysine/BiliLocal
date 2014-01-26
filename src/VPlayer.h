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

class libvlc_instance_t;
class libvlc_media_t;
class libvlc_media_player_t;

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
	State getState(){return state;}
	qint64 getTime();
	qint64 getDuration();
	QSize getSize(){return size;}
	QString getFile(){return file;}
	QList<QAction *> getSubtitles(){return subtitle;}
	QList<QAction *> getVideoTracks(){return video;}
	QList<QAction *> getAudioTracks(){return audio;}
	void releaseAndLock();

	virtual uchar *getBuffer()=0;
	virtual void initBuffer()=0;
	virtual void freeBuffer()=0;
	virtual void draw(QPainter *painter,QRect rect)=0;

	static VPlayer *create(QObject *parent=NULL);
	static VPlayer *instance(){return ins;}

private:
	State state;
	QTimer *fake;
	double ratio;
	QString file;
	QList<QAction *> subtitle;
	QList<QAction *> video;
	QList<QAction *> audio;
	libvlc_instance_t *vlc;
	libvlc_media_t *m;
	libvlc_media_player_t *mp;

	void setState(State _state);

protected:
	bool music;
	bool dirty;
	QSize size;
	QImage sound;
	static VPlayer *ins;

	VPlayer(QObject *parent=0);
	QRect getRect(QRect rect);

signals:
	void begin();
	void reach();
	void decode();
	void jumped(qint64 _time);
	void timeChanged(qint64 _time);
	void stateChanged(int _state);

public slots:
	void play();
	void stop();
	void init();
	void free();
	void setDirty();
	void setTime(qint64 _time);
	void setFile(QString _file);
	void setRatio(double _ratio);
	void setVolume(int _volume);
};

#endif // VPLAYER_H

