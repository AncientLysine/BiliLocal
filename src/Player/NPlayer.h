#pragma once

#include "APlayer.h"

class NPlayer :public APlayer
{
public:
	explicit NPlayer(QObject *parent = 0);
	QList<QAction *> getTracks(int type);

private:
	qint64	start;
	int		state;

	void	timerEvent(QTimerEvent * e);

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
