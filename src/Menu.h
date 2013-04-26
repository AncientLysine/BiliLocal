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
#include "Search.h"

class Menu:public QWidget
{
	Q_OBJECT
public:
	explicit Menu(QWidget *parent=0);
	~Menu();
	bool isPopped(){return isPop;}

private:
	bool isPop;
	bool isTurn;
	bool isLocal;
	QString lastPath;
	QLineEdit *fileL;
	QLineEdit *danmL;
	QLineEdit *sechL;
	QPushButton *fileB;
	QPushButton *danmB;
	QPushButton *sechB;
	QLabel *alphaT;
	QSlider *alphaS;
	QLabel *delayT;
	QLineEdit *delayL;
	QLabel *powerT;
	QLineEdit *powerL;
	QLabel *localT;
	QCheckBox *localC;
	QLabel *subT;
	QCheckBox *subC;
	QLabel *fontT;
	QComboBox *fontC;
	QPropertyAnimation *animation;

signals:
	void open(QString file);
	void load(QString danm);
	void power(qint16 _power);
	void alpha(double _alpha);
	void delay(qint64 _delay);
	void dfont(QString _font);
	void protect(bool _protect);

public slots:
	void pop();
	void push();
	void setDm(QString _file);
	void setFile(QString _file);
	
};

#endif // MENU_H
