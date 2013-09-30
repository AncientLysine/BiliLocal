/*=======================================================================
*
*   Copyright (C) 2013 Lysine.
*
*   Filename:    Printer.h
*   Time:        2013/08/14
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

#ifndef PRINTER_H
#define PRINTER_H

#include <QtGui>
#include <QtCore>
#include <QtWidgets>
#include "Utils.h"

class Printer:public QWidget
{
	Q_OBJECT
public:
	explicit Printer(QWidget *parent=0);
	bool isShown(){return ioo!=0;}
	void append(QString content){QMetaObject::invokeMethod(this,"process",Q_ARG(QString,content));}
	static Printer *instance(){return ins;}

private:
	int ioo;
	QTimer *timer;
	QTimer *delay;
	QTextStream stream;
	QStringList list;
	QGraphicsOpacityEffect *effect;
	static Printer *ins;
	void paintEvent(QPaintEvent *e);

public slots:
	void process(QString content);
	void fadeIn();
	void fadeOut();
	
};

#endif // PRINTER_H
