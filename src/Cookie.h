/*=======================================================================
*
*   Copyright (C) 2013 Lysine.
*
*   Filename:    Cookie.h
*   Time:        2013/10/17
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

#ifndef COOKIE_H
#define COOKIE_H

#include <QtCore>
#include <QtNetwork>

class Cookie:public QNetworkCookieJar
{
	Q_OBJECT
public:
	static void init();
	static void free();
	static Cookie *instance(){return &data;}

private:
	static Cookie data;

};

#endif // COOKIE_H
