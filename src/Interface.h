/*=======================================================================
*
*   Copyright (C) 2013 Lysine.
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

#ifndef INTERFACE_H
#define INTERFACE_H

#include <QtGui>
#include <QtCore>
#include <QtWidgets>

class Menu;
class Info;
class Panel;
class Printer;
class VPlayer;
class Danmaku;

class Interface:public QWidget
{
	Q_OBJECT
public:
	explicit Interface(QWidget *parent=0);

private:
	QLabel *tv;
	QLabel *me;
	QTimer *timer;
	QTimer *power;
	QTimer *delay;
	QAction *quitA;
	QAction *fullA;
	QAction *confA;
	QAction *toggA;
	QMenu *rat;
	QMenu *sca;

	Menu *menu;
	Info *info;
	Panel *panel;
	Printer *printer;
	VPlayer *vplayer;
	Danmaku *danmaku;

	QTime lst;
	QPoint pre;
	QPoint sta;
	QPoint wgd;
	QPixmap background;

	void dropEvent(QDropEvent *e);
	void paintEvent(QPaintEvent *e);
	void resizeEvent(QResizeEvent *e);
	void keyPressEvent(QKeyEvent *e);
	void mouseMoveEvent(QMouseEvent *e);
	void mouseReleaseEvent(QMouseEvent *e);
	void dragEnterEvent(QDragEnterEvent *e);
	void mouseDoubleClickEvent(QMouseEvent *e);

private slots:
	void saveSize();
	void setCenter(QSize s,bool f);

};

#endif // INTERFACE_H
