#pragma once

#include "APlayer.h"
#include <QtMultimedia>

class QPlayer :public APlayer
{
public:
	explicit QPlayer(QObject *parent = 0);
	QList<QAction *> getTracks(int type);

private:
	QMediaPlayer *mp;
	int		state;
	bool	manuallyStopped;
	bool	waitingForBegin;
	bool	skipTimeChanged;

public slots:
	void	play();
	void	stop(bool manually = true);
	int 	getState(){ return state; }

	void	setTime(qint64 _time);
	qint64	getTime();

	void	setMedia(QString _file, bool manually = true);
	QString getMedia();

	qint64	getDuration();
	void	addSubtitle(QString _file);

	void	setVolume(int _volume);
	int 	getVolume();

	void    setRate(double _rate);
	double  getRate();

	void	event(int type);
};
