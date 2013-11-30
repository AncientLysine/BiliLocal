/*=======================================================================
*
*   Copyright (C) 2013 Lysine.
*
*   Filename:    Graphic.h
*   Time:        2013/10/19
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

#ifndef GRAPHIC_H
#define GRAPHIC_H

#include <QtGui>
#include <QtCore>
#include "Utils.h"

class Graphic
{
public:
	virtual bool move(qint64 time)=0;
	virtual void draw(QPainter *painter)=0;
	virtual bool intersects(Graphic *other)=0;
	virtual QRectF currentRect(){return QRect();}
	virtual ~Graphic(){}
	int getMode(){return source?source->mode:0;}
	const Comment *getSource(){return source;}

protected:
	const Comment *source;
	Graphic(){source=NULL;}
};

class Mode1:public Graphic
{
public:
	Mode1(const Comment &comment,QList<Graphic *> &current,QSize size);
	bool move(qint64 time);
	void draw(QPainter *painter);
	bool intersects(Graphic *other);
	QRectF currentRect(){return rect;}

private:
	QRectF rect;
	double speed;
	QPixmap cache;
};

class Mode4:public Graphic
{
public:
	Mode4(const Comment &comment,QList<Graphic *> &current,QSize size);
	bool move(qint64 time);
	void draw(QPainter *painter);
	bool intersects(Graphic *other);
	QRectF currentRect(){return rect;}

private:
	QRectF rect;
	double life;
	QPixmap cache;
};

class Mode5:public Graphic
{
public:
	Mode5(const Comment &comment,QList<Graphic *> &current,QSize size);
	bool move(qint64 time);
	void draw(QPainter *painter);
	bool intersects(Graphic *other);
	QRectF currentRect(){return rect;}

private:
	QRectF rect;
	double life;
	QPixmap cache;
};

class Mode6:public Graphic
{
public:
	Mode6(const Comment &comment,QList<Graphic *> &current,QSize size);
	bool move(qint64 time);
	void draw(QPainter *painter);
	bool intersects(Graphic *other);
	QRectF currentRect(){return rect;}

private:
	QRectF rect;
	double speed;
	QPixmap cache;
};

class Mode7:public Graphic
{
public:
	Mode7(const Comment &comment,QList<Graphic *> &current,QSize size);
	bool move(qint64 time);
	void draw(QPainter *painter);
	bool intersects(Graphic *other);

private:
	QPointF bPos;
	QPointF ePos;
	double bAlpha;
	double eAlpha;
	double zRotate;
	double yRotate;
	QPixmap cache;
	double wait;
	double stay;
	double life;
	double time;
};

#endif // GRAPHIC_H
