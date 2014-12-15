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
#include <QtNetwork>

class Menu;
class Info;
class Jump;
class Post;
class List;
class Load;
class Render;
class APlayer;
class Danmaku;

class Interface:public QWidget
{
	Q_OBJECT
public:
	explicit Interface(QWidget *parent=0);
	bool event(QEvent *e);

private:
	QTimer *timer;
	QTimer *delay;

	QAction *quitA;
	QAction *fullA;
	QAction *confA;
	QAction *toggA;
	QAction *listA;
	QAction *postA;
	QMenu *rat;
	QMenu *sca;

	Render *render;
	Menu *menu;
	Info *info;
	Jump *jump;
	Post *post;
	List *list;
	Load *load;
	APlayer *aplayer;
	Danmaku *danmaku;

	QPoint sta;
	QPoint wgd;
	QByteArray geo;
	QPointer<QNetworkReply> update;

	bool showprg;
	bool sliding;

	void closeEvent(QCloseEvent *e);
	void dragEnterEvent(QDragEnterEvent *e);
	void dropEvent(QDropEvent *e);
	void mouseDoubleClickEvent(QMouseEvent *e);
	void mouseMoveEvent(QMouseEvent *e);
	void mousePressEvent(QMouseEvent *e);
	void mouseReleaseEvent(QMouseEvent *e);
	void resizeEvent(QResizeEvent *e);

public slots:
	void tryLocal(QString p);
	void tryLocal(QStringList p);
	void setVisible(bool f);

private slots:
	void checkForUpdate();
	void setCenter(QSize s,bool f);
	void showContextMenu(QPoint p);

};

#endif // INTERFACE_H
