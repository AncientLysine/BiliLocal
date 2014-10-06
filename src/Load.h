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
		Pool=410
	};

	enum Role
	{
		UrlRole=Qt::UserRole,
		StrRole,
		NxtRole
	};

	struct Task
	{
		QString code;
		QNetworkRequest request;
		int state;
		qint64 delay;
		Task():state(None),delay(0){}
	};

	bool event(QEvent *e);
	int  size();
	Task codeToTask(QString code);
	QStandardItemModel *getModel();
	static Load *instance();

private:
	bool automated;
	QStandardItemModel *model;
	QNetworkAccessManager *manager;
	QQueue<Task> queue;
	static Load *ins;

	Load(QObject *parent=0);

signals:
	void stateChanged(int state);

public slots:
	QString getStr();
	QString getUrl();

	void setAutoLoad(bool enabled);
	bool autoLoad();

	void dequeue();
	bool enqueue(const Task &task);
	void forward();
	void forward(QNetworkRequest request);
	void forward(QNetworkRequest request,int state);

	void loadDanmaku(QString code);
	void loadDanmaku(const QModelIndex &index=QModelIndex());
};

#endif // LOAD_H
