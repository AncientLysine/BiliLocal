/*=======================================================================
*
*   Copyright (C) 2013-2015 Lysine.
*
*   Filename:    Load.h
*   Time:        2014/04/22
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

class Comment;
class Record;
class LoadPrivate;

class Load : public QObject
{
	Q_OBJECT
public:
	enum State
	{
		None = 0,
		Page = 381,
		Part = 407,
		Code = 379,
		File = 384,
	};

	enum Role
	{
		UrlRole = Qt::UserRole,
		StrRole,
		NxtRole
	};

	struct Proc
	{
		std::function<bool(QString &)> regular;
		int priority;
		std::function<void(QNetworkReply *)> process;
	};

	struct Task
	{
		QString code;
		QNetworkRequest request;
		int state;
		const Proc *processer;
		qint64 delay;
		Task() :state(None), processer(nullptr), delay(0){}
	};

	Task codeToTask(QString code);
	static Load *instance();
	~Load();

private:
	static Load *ins;
	LoadPrivate *const d_ptr;
	Q_DECLARE_PRIVATE(Load);

	explicit Load(QObject *parent);

signals:
	void stateChanged(int state);
	void errorOccured(int state);
	void progressChanged(double progress);

public slots:
	void addProc(const Load::Proc *proc);
	const Load::Proc *getProc(QString code);

	void fixCode(QString &);
	bool canLoad(QString);
	bool canFull(const Record *);
	bool canHist(const Record *);

	void loadDanmaku(QString);
	void loadDanmaku(const QModelIndex &index = QModelIndex());
	void fullDanmaku(const Record *);
	void loadHistory(const Record *, QDate);
	void dumpDanmaku(const QList<Comment> *data, bool full);
	void dumpDanmaku(const QByteArray &data, int site, bool full);

	QStandardItemModel *getModel();
	void forward();
	void forward(QNetworkRequest);
	void forward(QNetworkRequest, int);
	void dequeue();
	bool enqueue(const Load::Task &);
	Load::Task *getHead();
};
