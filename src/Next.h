/*=======================================================================
*
*   Copyright (C) 2013 Lysine.
*
*   Filename:    Next.h
*   Time:        2013/04/22
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

#ifndef NEXT_H
#define NEXT_H

#include <QtCore>
#include <QtWidgets>

class Next:public QDialog
{
	Q_OBJECT
public:
	enum Action
	{
		DoNotContinnue,
		WaitUntilEnded,
		InheritDanmaku,
		PlayImmediately
	};
	QString getNext();
	static Next *instance();

private:
	QString fileC;
	QString fileN;
	QLineEdit *fileL;
	QMenu *nextM;
	QPushButton *nextB;
	qint64 duration;
	static Next *ins;

	Next(QWidget *parent);
	bool eventFilter(QObject *o,QEvent *e);

signals:
	void nextChanged(QString);

public slots:
	void parse();
	void clear();
	void shift();

private slots:
	void moveWithParent();
};

#endif // NEXT_H
