/*=======================================================================
*
*   Copyright (C) 2013 Lysine.
*
*   Filename:    Danmaku.h
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

#ifndef DANMAKU_H
#define DANMAKU_H

#include <QtGui>
#include <QtCore>
#include "Utils.h"

class Graphic;

class Danmaku:public QAbstractItemModel
{
	Q_OBJECT
public:
	explicit Danmaku(QObject *parent=0);
	QList<Record> &getPool(){return pool;}
	void draw(QPainter *painter,QRect rect,qint64 move);
	QVariant data(const QModelIndex &index,int role) const;
	int rowCount(const QModelIndex &parent=QModelIndex()) const;
	int columnCount(const QModelIndex &parent=QModelIndex()) const;
	QModelIndex parent(const QModelIndex &) const;
	QModelIndex index(int row,int colum,const QModelIndex &parent=QModelIndex()) const;
	QVariant headerData(int section,Qt::Orientation orientation,int role) const;
	const Comment *commentAt(QPoint point) const;
	static Danmaku *instance(){return ins;}

private:
	int cur;
	QSize size;
	qint64 time;
	QList<Record> pool;
	QList<Graphic *> current;
	QList<Comment *> danmaku;
	static Danmaku *ins;
	void setTime(qint64 _time);
	Graphic *render(const Comment &comment);

public slots:
	void resetTime();
	void clearPool();
	void clearCurrent();
	void parse(int flag=0);
	void jumpToTime(qint64 _time);
	void saveToFile(QString _file);
	void appendToPool(const Record &record);
	void appendToCurrent(const Comment *comment,bool isLocal=false);
	void appendToCurrent(const QList<const Comment *> &comments,bool isLocal=false);
};

#endif // DANMAKU_H
