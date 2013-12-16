/*=======================================================================
*
*   Copyright (C) 2013 Lysine.
*
*   Filename:    Panel.h
*   Time:        2013/05/23
*   Author:      zhengdanwei
*   Contributor: Lysine
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

#ifndef PANEL_H
#define PANEL_H

#include <QtCore>
#include <QtWidgets>
#include <QtNetwork>

class Record;

class Panel:public QWidget
{
	Q_OBJECT
public:
	explicit Panel(QWidget *parent = 0);
	bool isShown(){return ioo==2;}
	bool isValid(){return getBilibili()!=NULL;}
	QColor getColor();

public slots:
	void fadeIn();
	void fadeOut();
	void setTime(qint64 _time);
	void setColor(QColor color);
	void postComment(QString comment);
	void setDuration(qint64 _duration);

private:
	bool sliding;
	bool updating;
	qint64 duration;
	QAbstractSlider *timeS;
	QAction *commentA;
	QLineEdit *commentL;
	QComboBox *commentM;
	QPushButton *commentC;
	QPushButton *commentB;
	QNetworkAccessManager *manager;
	int ioo;
	QTimer *timer;
	QGraphicsOpacityEffect *effect;
	const Record *getBilibili();
};

#endif // POSTER_H
