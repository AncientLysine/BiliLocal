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

#include <QtQml>
#include <QtGui>
#include <QtCore>
#include "Utils.h"
#include "Shield.h"

class Danmaku:public QAbstractItemModel
{
	Q_OBJECT
public:
	explicit Danmaku(QObject *parent=0);
	void draw(QPainter *painter,bool move=true);
	QVariant data(const QModelIndex &index,int role) const;
	int rowCount(const QModelIndex &parent=QModelIndex()) const;
	int columnCount(const QModelIndex &parent=QModelIndex()) const;
	QModelIndex parent(const QModelIndex &) const;
	QModelIndex index(int row,int colum,const QModelIndex &parent=QModelIndex()) const;
	QVariant headerData(int section,Qt::Orientation orientation,int role) const;
	qint64 getTime(){return time;}
	QList<Record> &getPool(){return pool;}
	static Danmaku *instance(){return ins;}

private:
	int cur;
	QTime last;
	QSize size;
	qint64 time;
	QJSEngine engine;
	QList<Record> pool;
	struct Static
	{
		double life;
		double speed;
		QRectF rect;
		QPixmap text;
	};
	QList<Static> current[5];
	QVector<const Comment *> danmaku;
	static Danmaku *ins;

public slots:
	void resetTime();
	void clearPool();
	void clearCurrent();
	void parse(int flag=0);
	void setSize(QSize _size);
	void setTime(qint64 _time);
	void jumpToTime(qint64 _time);
	void saveToFile(QString _file);
	void appendToPool(const Record &record);
	void appendToCurrent(const Comment &comment);
};

#endif // DANMAKU_H
