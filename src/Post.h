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
	explicit Post(QWidget *parent);
	bool isValid(){return !getRecords().isEmpty();}

private:
	QAction *commentA;
	QLineEdit *commentL;
	QComboBox *commentS;
	QComboBox *commentM;
	QPushButton *commentC;
	QPushButton *commentB;
	QNetworkAccessManager *manager;
	QColor getColor();
	Comment getComment();
	QList<const Record *> getRecords();
	bool eventFilter(QObject *o,QEvent *e);

private slots:
	void setColor(QColor color);
	void postComment();
	void moveWithParent();
};

#endif // POST_H
