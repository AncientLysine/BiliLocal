/*=======================================================================
*
*   Copyright (C) 2013 Lysine.
*
*   Filename:    Shield.h
*   Time:        2013/05/20
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

#ifndef SHIELD_H
#define SHIELD_H

#include <QtCore>
#include "Utils.h"

class Shield
{
public:
	enum {Top,Bottom,Slide,Guest,Color,Whole};
	Shield();
	~Shield();
	bool block[6];
	QList<QString> shieldU;
	QList<QRegExp> shieldR;
	static Shield *instance;
	static void configure(QWidget *parent=0);
	static bool isBlocked(const Comment &comment);
	static bool setEnabled(quint8 t,bool enabled);
};

class Editor:public QDialog
{
	Q_OBJECT
public:
	explicit Editor(Shield *shield,QWidget *parent=0);
	QStringList getRegexp() const {return rm->stringList();}
	QStringList getSender() const {return sm->stringList();}
	void import(QString path);

private:
	QLineEdit *edit;
	QCheckBox *check[6];
	QListView *regexp;
	QListView *sender;
	QStringListModel *rm;
	QStringListModel *sm;
	QAction *action[3];
	QPushButton *button[2];

	void dropEvent(QDropEvent *e);
	void dragEnterEvent(QDragEnterEvent *e);
};

#endif // SHIELD_H
