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
#include <QtWidgets>
#include <QtNetwork>
#include "Utils.h"
#include "Shield.h"

struct Static
{
	double life;
	double speed;
	QRectF rect;
	QPixmap text;
};

class Danmaku:public QAbstractItemModel
{
	Q_OBJECT
public:
	explicit Danmaku(QObject *parent=0);
	void draw(QPainter *painter,bool move=true);
	QVariant data(const QModelIndex &index,int role) const;
	int rowCount(const QModelIndex &parent) const;
	int columnCount(const QModelIndex &parent) const;
	QModelIndex parent(const QModelIndex &) const;
	QModelIndex index(int row,int colum,const QModelIndex &parent=QModelIndex()) const;
	QVariant headerData(int section,Qt::Orientation orientation,int role) const;
	bool removeRows(int row,int count,const QModelIndex &parent);
	QString getCid(){return cid;}

private:
	int currentIndex;
	bool sub;
	double alpha;
	qint64 delay;

	QFont font;
	QSize size;
	QTime last;
	QString cid;
	Shield shield;
	QList<Static> current[5];
	QVector<Comment> danmaku;

signals:
	void loaded();

public slots:
	void reset();
	void setLast();
	void setDm(QString dm);
	void setTime(qint64 time);
	void setSize(QSize _size);
	void setFont(QString _font);
	void setAlpha(double _alpha);
	void setDelay(qint64 _delay);
	void setProtect(bool enabled);
	void jumpToTime(qint64 time);
};

#endif // DANMAKU_H
