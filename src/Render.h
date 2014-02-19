/*=======================================================================
*
*   Copyright (C) 2013 Lysine.
*
*   Filename:    Render.h
*   Time:        2013/12/27
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

#ifndef RENDER_H
#define RENDER_H

#include <QtGui>
#include <QtCore>
#include <QtWidgets>

class Render:public QObject
{
	Q_OBJECT
public:
	~Render(){}
	QWidget *getWidget(){return widget;}
	static Render *create(QWidget *parent=NULL);
	static Render *instance(){return ins;}

private:
	QMovie tv;
	double time;
	QImage me,background;
	QTime last;

protected:
	QWidget *widget;
	static Render *ins;
	explicit Render(QWidget *parent=0);

public slots:
	void setTime(double t);
	virtual void draw(QRect rect=QRect())=0;
	void drawPlay(QPainter *painter,QRect rect);
	void drawStop(QPainter *painter,QRect rect);
	void drawTime(QPainter *painter,QRect rect);
};

#endif // RENDER_H
