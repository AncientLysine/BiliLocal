#pragma once

#include "APlayer.h"

class NPlayer : public APlayer
{
public:
	explicit NPlayer(QObject *parent = nullptr);

private:
	qint64  start;
	int     state;

	void    timerEvent(QTimerEvent * e) override;

public slots:
	void    play() override;
	void    stop(bool manually = true) override;
	int     getState() override { return state; }

	void    setTime(qint64 time) override;
	qint64  getTime() override;

	void    setMedia(QString file) override;
	QString getMedia() override;

	qint64  getDuration() override;

	void    setVolume(int volume) override;
	int     getVolume() override;
};
