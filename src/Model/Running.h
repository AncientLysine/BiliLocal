/*=======================================================================
*
*   Copyright (C) 2013-2016 Lysine.
*
*   Filename:    Layout.h
*   Time:        2016/05/29
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

#pragma once

#include <QtCore>
#include <QtGui>

class Comment;
class Graphic;
class RunningPrivate;

class Running : public QObject
{
	Q_OBJECT
public:
	static Running *instance();
	virtual ~Running();

private:
	static Running *ins;
	RunningPrivate *const d_ptr;
	Q_DECLARE_PRIVATE(Running);

	explicit Running(QObject *parent = 0);

signals:
	void unrecognizedComment(const Comment *);

public slots:
	void clear(bool soft = false);
	void insert(Graphic *graphic, int index = -1);
	void moveTime(qint64 time);
	void jumpTime(qint64 time);
	void draw(QPainter *painter, double move);
	const Comment *commentAt(QPointF point) const;
	int size() const;
};