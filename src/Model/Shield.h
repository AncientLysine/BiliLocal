/*=======================================================================
*
*   Copyright (C) 2013-2015 Lysine.
*
*   Filename:    Shield.h
*   Time:        2013/05/20
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

class Comment;
class ShieldPrivate;

class Shield :public QObject
{
	Q_OBJECT
public:
	enum Group
	{
		Top,
		Bottom,
		Slide,
		Reverse,
		Guest,
		Advanced,
		Color,
		Whole
	};

	explicit Shield(QObject *parent = nullptr);
	virtual ~Shield();

private:
	ShieldPrivate *const d_ptr;
	Q_DECLARE_PRIVATE(Shield);


signals:
	void shieldChanged();

public slots:
	void setAllShields(const QStringList &);
	QStringList getAllShields();
	bool contains(const QString &);
	void insert(const QString &);
	void remove(const QString &);
	bool isBlocked(const Comment &);
};
