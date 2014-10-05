/*=======================================================================
*
*   Copyright (C) 2013 Lysine.
*
*   Filename:    History.h
*   Time:        2014/10/05
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

#ifndef HISTORY_H
#define HISTORY_H

#include <QtGui>
#include <QtCore>

class History:public QObject
{
    Q_OBJECT
public:
	~History();
	QString lastPath();
	QStandardItemModel *getModel(){return model;}
	static History *instance();

private:
	QStandardItemModel *model;
	static History *ins;

    History(QObject *parent = 0);
};

#endif // HISTORY_H
