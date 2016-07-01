#pragma once

#include "APlayer.h"

class NPlayer :public APlayer
{
public:
	explicit NPlayer(QObject *parent = 0);

private:
	qint64  start;
	int     state;

	void    timerEvent(QTimerEvent * e);

public slots:
	void    play();
	void    stop(bool manually = true);
	int     getState(){ return state; }

	void    setTime(qint64 time);
	qint64  getTime();

	void    setMedia(QString file);
	QString getMedia();

	qint64  getDuration();

	void    setVolume(int volume);
	int     getVolume();
};
