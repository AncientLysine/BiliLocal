#pragma once

#include "APlayer.h"
#include <QtMultimedia>

class QPlayer :public APlayer
{
public:
	explicit QPlayer(QObject *parent = 0);
	virtual ~QPlayer();

private:
	QThread *pt;
	QMediaPlayer *mp;
	int	    state;
	bool    manuallyStopped;
	bool    waitingForBegin;
	bool    skipTimeChanged;

public slots:
	void    play();
	void    stop(bool manually = true);
	int 	getState(){ return state; }

	void    setTime(qint64 time);
	qint64  getTime();

	void    setMedia(QString file);
	QString getMedia();

	qint64  getDuration();

	void    setVolume(int volume);
	int     getVolume();

	void    setRate(double rate);
	double  getRate();
};
