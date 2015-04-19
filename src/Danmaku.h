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

class Comment;
class Graphic;
class Record;
class DanmakuPrivate;

class Danmaku:public QAbstractItemModel
{
	Q_OBJECT
public:
	~Danmaku();
	void draw(QPainter *painter,qint64 move);
	QList<Record> &getPool();
	QVariant data(const QModelIndex &index,int role) const;
	int rowCount(const QModelIndex &parent=QModelIndex()) const;
	int columnCount(const QModelIndex &parent=QModelIndex()) const;
	QModelIndex parent(const QModelIndex &) const;
	QModelIndex index(int row,int colum,const QModelIndex &parent=QModelIndex()) const;
	QVariant headerData(int section,Qt::Orientation orientation,int role) const;
	static Danmaku *instance();

private:
	static Danmaku *ins;
	DanmakuPrivate *const d_ptr;
	Q_DECLARE_PRIVATE(Danmaku)

	Danmaku(QObject *parent=0);
	void setTime(qint64 _time);

signals:
	void alphaChanged(int);
	void unrecognizedComment(const Comment *);

public slots:
	void setAlpha(int alpha);
	void resetTime();
	void clearPool();
	void appendToPool(const Record *record);
	void appendToPool(QString source,const Comment *comment);
	void clearCurrent(bool soft=false);
	void insertToCurrent(Graphic *graphic,int index=-1);
	const Comment *commentAt(QPointF point) const;
	void parse(int flag=0);
	void delayAll(qint64 time);
	void jumpToTime(qint64 time);
	void saveToFile(QString file) const;
};

#endif // DANMAKU_H
