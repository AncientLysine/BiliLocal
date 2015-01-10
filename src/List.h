/*=======================================================================
*
*   Copyright (C) 2013 Lysine.
*
*   Filename:    List.h
*   Time:        2014/11/19
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

#ifndef LIST_H
#define LIST_H

#include <QtGui>
#include <QtCore>

class List:public QStandardItemModel
{
	Q_OBJECT
public:
	enum Role
	{
		FileRole=Qt::UserRole,
		CodeRole,
		TimeRole,
		DateRole
	};

	enum Danm
	{
		Records,
		Inherit,
		Surmise
	};

	~List();
	static List *instance();
	bool dropMimeData(const QMimeData *data,Qt::DropAction action,int row,int column,const QModelIndex &parent);

private:
	QStandardItem *cur;
	qint64 time;
	QList<QIcon> icons;
	static List *ins;
	List(QObject *parent);
	void setRelated(const QModelIndexList &indexes,int reason);

public slots:
	QString defaultPath(int type);
	QStandardItem *getCurrent(){return cur;}
	QStandardItem *itemFromFile(QString file,bool create=false);
	bool finished();
	void updateCurrent();
	void merge(const QModelIndexList &indexes);
	void group(const QModelIndexList &indexes);
	void split(const QModelIndexList &indexes);
	bool jumpToIndex(const QModelIndex &index,bool manually=true);
};

#endif // LIST_H
