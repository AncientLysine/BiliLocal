#pragma once

#include "APlayer.h"
#include <QtMultimedia>

class QPlayer : public APlayer
{
public:
	explicit QPlayer(QObject *parent = nullptr);
	virtual ~QPlayer();

private:
	enum State
	{
		Loop = InternalState
	};

	QThread *pt;
	QMediaPlayer *mp;
	int	    state;
	bool    manuallyStopped;
	bool    waitingForBegin;
	bool    skipTimeChanged;

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

	void    setRate(double rate) override;
	double  getRate() override;
};
