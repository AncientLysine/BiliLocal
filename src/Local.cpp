/*=======================================================================
*
*   Copyright (C) 2013 Lysine.
*
*   Filename:    Local.cpp
*   Time:        2013/03/18
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

#include "Interface.h"
#include <QApplication>

int main(int argc,char *argv[])
{
	QApplication::setStyle("Fusion");
	QApplication a(argc,argv);
	QString locale=QLocale::system().name();
	QTranslator myTrans;
	myTrans.load(locale+".qm","./translations");
	QTranslator qtTrans;
	qtTrans.load(locale+".qt.qm","./translations");
	a.installTranslator(&myTrans);
	a.installTranslator(&qtTrans);
	QDir::setCurrent(a.applicationDirPath());
	Utils::loadConfig();
	Interface w;
	w.show();
	return a.exec();
}
