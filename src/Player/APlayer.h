/*=======================================================================
*
*   Copyright (C) 2013-2015 Lysine.
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

class APlayer :public QObject
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

	enum Error
	{
		UnknownError,
		ResourceError,
		FormatError,
		NetworkError,
		AccessDeniedError,
		ServiceMissingError
	};

	static QStringList getModules();
	static APlayer *create(QObject *parent = nullptr, QString name = QString());

	virtual QList<QAction *> getTracks(int type) = 0;

protected:
	explicit APlayer(QObject *parent);

signals:
	void errorOccurred(int);
	void begin();
	void reach(bool);
	void decode();
	void jumped(qint64);
	void stateChanged(int);
	void mediaChanged(QString);
	void delayChanged(int, qint64);
	void timeChanged(qint64);
	void rateChanged(double);
	void volumeChanged(int);

public slots:
	virtual void    play() = 0;
	virtual void    stop(bool manually = true) = 0;
	virtual int     getState() = 0;

	virtual void    setTime(qint64 time) = 0;
	virtual qint64  getTime() = 0;

	virtual void    setMedia(QString file, bool manually = true) = 0;
	virtual QString getMedia() = 0;

	virtual qint64  getDuration() = 0;

	virtual void    setVolume(int volume) = 0;
	virtual int     getVolume() = 0;

	virtual void    setRate(double rate);
	virtual double  getRate();

	virtual qint64  getDelay(int type);
	virtual void    setDelay(int type, qint64 delay);

	virtual void    addSubtitle(QString file);

	virtual void    event(int type);
};
