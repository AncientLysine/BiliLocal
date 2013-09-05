/*=======================================================================
*
*   Copyright (C) 2013 Lysine.
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

#ifndef SHIELD_H
#define SHIELD_H

#include <QtCore>
#include "Utils.h"

class Shield
{
public:
	enum {Top,Bottom,Slide,Guest,Color,Whole};
	static bool block[6];
	static QList<QString> shieldU;
	static QList<QRegExp> shieldR;
	static QList<QString> shieldC;
	static QCache<Comment,bool> cacheS;
	static void init();
	static void free();
	static bool isBlocked(const Comment &comment);

private:
	static bool judge(const Comment &comment);
};

#endif // SHIELD_H
