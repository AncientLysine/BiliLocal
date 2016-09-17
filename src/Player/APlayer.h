/*=======================================================================
*
*   Copyright (C) 2013-2016 Lysine.
*
*   Filename:    APlayer.h
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

#pragma once

#include <QtCore>
#include <QtGui>

class APlayerPrivate;

class APlayer : public QObject
{
	Q_OBJECT
public:
	enum State
	{
		Stop,
		Play,
		Pause,
		InternalState
	};
	Q_ENUM(State);

	enum Error
	{
		UnknownError,
		ResourceError,
		FormatError,
		NetworkError,
		AccessDeniedError,
		ServiceMissingError
	};
	Q_ENUM(Error);

	static QStringList getModules();
	static APlayer *create(QObject *parent = nullptr, QString name = QString());

	virtual void setup();

	Q_PROPERTY(int state READ getState NOTIFY stateChanged);
	Q_PROPERTY(qint64 time READ getTime WRITE setTime NOTIFY timeChanged);
	Q_PROPERTY(QString media READ getMedia WRITE setMedia NOTIFY mediaChanged);
	Q_PROPERTY(int volume READ getVolume WRITE setVolume NOTIFY volumeChanged);
	Q_PROPERTY(double rate READ getRate WRITE setRate NOTIFY rateChanged);

protected:
	explicit APlayer(QObject *parent);

signals:
	void errorOccurred(int error);
	void begin();
	void reach(bool manually);
	void decode();
	void jumped(qint64 time);
	void stateChanged(int state);
	void mediaChanged(QString media);
	void delayChanged(int type, qint64 delay);
	void trackChanged(int type, int current);
	void timeChanged(qint64 time);
	void rateChanged(double rate);
	void volumeChanged(int volume);

public slots:
	virtual void    play() = 0;
	virtual void    stop(bool manually = true) = 0;
	virtual int     getState() = 0;

	virtual void    setTime(qint64 time) = 0;
	virtual qint64  getTime() = 0;

	virtual void    setMedia(QString file) = 0;
	virtual QString getMedia() = 0;

	virtual qint64  getDuration() = 0;

	virtual void    setVolume(int volume) = 0;
	virtual int     getVolume() = 0;

	virtual void    setLoop(bool loop);
	virtual bool    getLoop();

	virtual void    setRate(double rate);
	virtual double  getRate();

	virtual void    setDelay(int type, qint64 delay);
	virtual qint64  getDelay(int type);

	virtual int     getTrack(int type);
	virtual void    setTrack(int type, int index);
	virtual QStringList getTracks(int type);

	virtual void    addSubtitle(QString file);

	virtual void    event(int type, QVariant args = QVariant());
};
