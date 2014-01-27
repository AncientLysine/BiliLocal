/*=======================================================================
*
*   Copyright (C) 2013 Lysine.
*
*   Filename:    Post.h
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

#ifndef POST_H
#define POST_H

#include <QtCore>
#include <QtWidgets>
#include <QtNetwork>

class Record;
class Comment;

class Post:public QDialog
{
	Q_OBJECT
public:
	explicit Post(QWidget *parent=0);
	static bool isValid(){return !getRecords().isEmpty();}

private:
	QLabel *commentP;
	QAction *commentA;
	QLineEdit *commentL;
	QComboBox *commentS;
	QComboBox *commentM;
	QPushButton *commentC;
	QPushButton *commentB;
	QNetworkAccessManager *manager;
	QIcon close;
	QColor getColor();
	Comment getComment();
	void paintEvent(QPaintEvent *e);
	void mouseReleaseEvent(QMouseEvent *e);
	static QList<const Record *> getRecords();

private slots:
	void setColor(QColor color);
	void drawComment();
	void postComment();

public slots:
	int exec();
};

#endif // POST_H
