/*=======================================================================
*
*   Copyright (C) 2013-2015 Lysine.
*
*   Filename:    Interface.h
*   Time:        2013/03/18
*   Author:      Lysine
*   Contributor: Chaserhkj
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

#pragma once

#include <QtCore>

class QWidget;
class QWindow;
class InterfacePrivate;

class Interface :public QObject
{
	Q_OBJECT
public:
	explicit Interface(QObject *parent = nullptr);
	virtual ~Interface();
	void setup();

private:
	InterfacePrivate *d_ptr;
	Q_DECLARE_PRIVATE(Interface);


public slots:
	void percent(double degree);
	void warning(QString title, QString text);
	void show();
	void hide();
	QWidget *widget();
	QWindow *window();
};
