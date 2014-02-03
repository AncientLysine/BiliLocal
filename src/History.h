/*=======================================================================
*
*   Copyright (C) 2013 Lysine.
*
*   Filename:    History.h
*   Time:        2014/02/03
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

#include <QtCore>
#include <QtWidgets>
#include <QtNetwork>

class Record;

class History:public QDialog
{
	Q_OBJECT
public:
	explicit History(Record &record,QWidget *parent=0);
	QDate selectedDate();

private:
	QDate page;
	QDate curr;
	QLabel *date;
	QToolButton *prev;
	QToolButton *next;
	QTableWidget *table;
	QNetworkAccessManager *manager;
	QMap<QDate,int> count;
	QDate currentLimit();

private slots:
	void setCurrentPage(QDate _d);
};

#endif // HISTORY_H
