/*=======================================================================
*
*   Copyright (C) 2013 Lysine.
*
*   Filename:    Menu.h
*   Time:        2013/04/05
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

#ifndef MENU_H
#define MENU_H

#include <QtCore>
#include <QtWidgets>

class Menu:public QWidget
{
	Q_OBJECT
public:
	explicit Menu(QWidget *parent=0);
	bool isShown(){return isPoped;}
	QTimer *getPower(){return powerC;}
	bool preferStay(){return isStay||!danmC->popup()->isHidden();}

private:
	bool isStay;
	bool isPoped;
	QLineEdit *fileL;
	QLineEdit *danmL;
	QLineEdit *sechL;
	QCompleter *danmC;
	QPushButton *fileB;
	QPushButton *danmB;
	QPushButton *sechB;
	QAction *fileA;
	QAction *danmA;
	QAction *sechA;
	QLabel *alphaT;
	QSlider *alphaS;
	QLabel *powerT;
	QTimer *powerC;
	QLineEdit *powerL;
	QLabel *localT;
	QCheckBox *localC;
	QLabel *subT;
	QCheckBox *subC;
	QLabel *loopT;
	QCheckBox *loopC;
	QPropertyAnimation *animation;

	bool eventFilter(QObject *o,QEvent *e);
	void setPower(qint16 fps);

public slots:
	void pop();
	void push(bool force=false);
	void terminate();
	
};

#endif // MENU_H
