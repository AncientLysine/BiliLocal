/*=======================================================================
*
*   Copyright (C) 2013 Lysine.
*
*   Filename:    Info.h
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

#ifndef INFO_H
#define INFO_H

#include <QtCore>
#include <QtWidgets>

class Info:public QWidget
{
	Q_OBJECT
public:
	explicit Info(QWidget *parent=0);
	bool isShown(){return isPoped;}
	bool preferStay(){return isStay;}

private:
	bool isStay;
	bool isPoped;
	bool updating;
	qint64 duration;

	QLabel *durT;
	QLabel *timeT;
	QLabel *volmT;
	QLabel *plfmT;
	QSlider *timeS;
	QSlider *volmS;
	QLineEdit *plfmL;
	QTableView *danmV;
	QPushButton *playB;
	QPushButton *stopB;
	QAction *playA;
	QAction *stopA;
	QIcon playI,stopI,pauseI;
	QPropertyAnimation *animation;
	void resizeEvent(QResizeEvent *e);
	
public slots:
	void pop();
	void push(bool force=false);
	void terminate();
	void resizeHeader();
	void setTime(qint64 _time);
	void setDuration(qint64 _duration);
	
};

#endif // INFO_H
