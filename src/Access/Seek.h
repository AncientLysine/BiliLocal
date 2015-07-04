/*=======================================================================
*
*   Copyright (C) 2013-2015 Lysine.
*
*   Filename:    Seek.h
*   Time:        2015/06/30
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
#include <QStandardItemModel>
#include <functional>

class SeekPrivate;

class Seek : public QObject
{
	Q_OBJECT
public:
	enum State
	{
		None,
		List,
		More,
		Data
	};

	struct Proc
	{
		const QString name;
		const QStringList sort;
		inline bool regular(QString &code) const { return code == name; }
		int priority;
		std::function<void(QNetworkReply *)> process;
	};

	struct Task
	{
		QString code;
		QString text;
		int sort;
		QPair<int, int> page;
		QSize cover;
		QStandardItemModel *model;
		int state;
		QNetworkRequest request;
		const Proc *processer;
		Task() :sort(0), state(None), processer(nullptr){}
	};

	static Seek *instance();

private:
	static Seek *ins;
	QScopedPointer<SeekPrivate> const d_ptr;
	Q_DECLARE_PRIVATE(Seek);

	explicit Seek(QObject *parent);

signals:
	void stateChanged(int code);
	void errorOccured(int code);

public slots:
	void addProc(const Seek::Proc *proc);
	const Seek::Proc *getProc(QString name);

	QStringList modules();
	void dequeue();
	bool enqueue(const Seek::Task &);
	Seek::Task *getHead();
	void forward();
};
