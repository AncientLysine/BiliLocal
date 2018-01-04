#pragma once

#include "APlayer.h"
#include <QtCore>
#include <QtAndroidExtras>

class ABuffer;

class JPlayer : public APlayer
{
public:
	enum Event
	{
		Fail,
		Info,
		Prep,
		Comp,
		Size,
		Avai
	};

	explicit JPlayer(QObject *parent = nullptr);
	virtual ~JPlayer();

private:
	enum State
	{
		Idel = InternalState + 1,
		Initialized,
		Preparing,
		Prepared,
		Started = Play,
		Paused = Pause,
		Complete,
		Stopped = Stop,
		Error
	};

	QAndroidJniObject mp;
	QAndroidJniObject st;
	QSharedPointer<GLuint> tn;
	int state;
	QString meida;

public slots:
	void    play() override;
	void    stop(bool manually = true) override;
	int     getState() override;

	void    setTime(qint64 time) override;
	qint64  getTime() override;

	void    setMedia(QString file) override;
	QString getMedia() override;

	qint64  getDuration() override;

	void    setVolume(int volume) override;
	int     getVolume() override;

	void    setLoop(bool loop) override;
	bool    getLoop() override;

	void    event(int type, QVariant args = QVariant()) override;
};
