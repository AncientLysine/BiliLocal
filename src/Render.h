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

class Render:public QWindow
{
	Q_OBJECT
public:
	explicit Render(QWindow *parent=0);
	void draw();
	static Render *instance(){return ins;}

private:
	QTime last;
	QMovie tv;
	QImage me;
	QImage background;
	QOpenGLContext *context;
	QOpenGLPaintDevice *device;
	static Render *ins;
	void exposeEvent(QExposeEvent *){draw();}
	void drawInit(QPainter *painter,QRect rect);
};

#endif // RENDER_H