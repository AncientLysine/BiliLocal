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
#include "Menu.h"
#include "Info.h"
#include "Utils.h"
#include "VPlayer.h"
#include "Danmaku.h"

class Render:public QWidget
{
	Q_OBJECT
public:
	explicit Render(QWidget *parent=0);
	void setVplayer(VPlayer *_vplayer){vplayer=_vplayer;}
	void setDanmaku(Danmaku *_danmaku){danmaku=_danmaku;}

private:
	VPlayer *vplayer;
	Danmaku *danmaku;
	void paintEvent(QPaintEvent *e);

};

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

	Menu *menu;
	Info *info;
	Render *render;
	VPlayer *vplayer;
	Danmaku *danmaku;

	void dropEvent(QDropEvent *e);
	void resizeEvent(QResizeEvent *e);
	void keyPressEvent(QKeyEvent *e);
	void mouseMoveEvent(QMouseEvent *e);
	void dragEnterEvent(QDragEnterEvent *e);
	void mouseDoubleClickEvent(QMouseEvent *e);

};

#endif // INTERFACE_H
