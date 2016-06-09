/*=======================================================================
*
*   Copyright (C) 2013-2015 Lysine.
*
*   Filename:    Sign.h
*   Time:        2015/11/14
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
#include <QtNetwork>
#include <functional>

class SignPrivate;

class Sign : public QObject
{
	Q_OBJECT
public:
	enum State
	{
		None,
		Test,
		Code,
		Wait,
		Salt,
		Data
	};

	struct Proc
	{
		QString name;
		inline bool regular(QString &code) const { return code == name; }
		int priority;
		std::function<void(QNetworkReply *)> process;
	};

	struct Task
	{
		QString username;
		QString password;
		QString captcha;

		QString code;
		QNetworkRequest request;
		int state;
		const Proc *processer;
		QByteArray data;
		bool logged;
		QString error;

		Task() :state(None), processer(nullptr), logged(false){}
	};

	explicit Sign(QObject *parent = nullptr);
	virtual ~Sign();

private:
	SignPrivate *const d_ptr;
	Q_DECLARE_PRIVATE(Sign);


signals:
	void stateChanged(int state);
	void errorOccured(int state);

public slots:
	void addProc(const Sign::Proc *proc);
	const Sign::Proc *getProc(QString name);
	QStringList modules();

	void forward();
	void forward(int state);
	void dequeue();
	bool enqueue(QString site);
	Sign::Task *getHead();
};