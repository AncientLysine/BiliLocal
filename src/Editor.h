/*=======================================================================
*
*   Copyright (C) 2013 Lysine.
*
*   Filename:    Editor.h
*   Time:        2013/06/30
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

#ifndef EDITOR_H
#define EDITOR_H

#include <QtGui>
#include <QtCore>
#include <QtWidgets>
#include "Danmaku.h"
#include "VPlayer.h"

class Editor:public QDialog
{
	Q_OBJECT
public:
	explicit Editor(QWidget *parent=0);

private:
	class Widget:public QWidget
	{
	public:
		explicit Widget(QWidget *parent=0);

	private:
		int scale;
		int length;
		QPoint point;
		qint64 current;
		qint64 duration;
		QList<qint64> magnet;
		QList<QLineEdit *> time;
		void load();
		void paintEvent(QPaintEvent *e);
		void wheelEvent(QWheelEvent *e);
		void mouseMoveEvent(QMouseEvent *e);
		void mouseReleaseEvent(QMouseEvent *e);
		void delayRecord(int index,qint64 delay);
	};
	Widget *widget;
	QGridLayout *layout;
	QScrollArea *scroll;
	void resizeEvent(QResizeEvent *e);
};

#endif // EDITOR_H
