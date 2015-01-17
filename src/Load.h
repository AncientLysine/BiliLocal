/*=======================================================================
*
*   Copyright (C) 2013 Lysine.
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

#ifndef LOAD_H
#define LOAD_H

#include <QtGui>
#include <QtCore>
#include <QtNetwork>
#include <functional>

class Record;

class Load:public QObject
{
	Q_OBJECT
public:
	enum State
	{
		None=0,
		Page=381,
		Part=407,
		Code=379,
		File=384,
	};

	enum Role
	{
		UrlRole=Qt::UserRole,
		StrRole,
		NxtRole
	};

	struct Proc
	{
		std::function<bool(QString &      )> regular;
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
		Task():state(None),processer(nullptr),delay(0){}
	};

	Task codeToTask(QString code);
	QStandardItemModel *getModel();
	int size(){return queue.size();}
	static Load *instance();

private:
	QStandardItemModel *model;
	QList <Proc> pool;
	QNetworkAccessManager *manager;
	QQueue<Task> queue;
	QSet<QNetworkReply *> remain;
	static Load *ins;

	Load(QObject *parent=0);

signals:
	void stateChanged(int state);
	void errorOccured(int state);
	void progressChanged(double progress);

public slots:
	void addProc(const Proc *proc);
	const Proc *getProc(QString code);

	void fixCode(QString &);
	bool canLoad(QString);
	bool canFull(QString);
	bool canHist(QString);

	void loadDanmaku(QString);
	void loadDanmaku(const QModelIndex &index=QModelIndex());
	void fullDanmaku(QString);
	void loadHistory(QString,QDate);
	void dumpDanmaku(const QByteArray &data,int site,Record *r);
	void dumpDanmaku(const QByteArray &data,int site,bool full);

	void dequeue();
	bool enqueue(const Task &);
	Task*getHead();
	void forward();
	void forward(QNetworkRequest);
	void forward(QNetworkRequest,int);
};

#endif // LOAD_H
