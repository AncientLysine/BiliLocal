/*=======================================================================
*
*   Copyright (C) 2013 Lysine.
*
*   Filename:    Local.cpp
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
#include "Interface.h"
#include <QApplication>

class DoubleClickStyle : public QProxyStyle
{
public:
	DoubleClickStyle(QStyle *proxy = 0) : QProxyStyle(proxy){}
	int styleHint(StyleHint hint, const QStyleOption *option = 0,
				  const QWidget *widget = 0, QStyleHintReturn *returnData = 0) const {
		if (hint == QStyle:: SH_ItemView_ActivateItemOnSingleClick)
			return 0;
		return QProxyStyle::styleHint(hint, option, widget, returnData);
	}
};

int main(int argc, char *argv[])
{
	QTranslator myTrans;
	myTrans.load("zh_CN.qm",":/Trans/");
	QTranslator qtTrans;
	qtTrans.load("zh_CN.qt.qm",":/Trans/");
#ifdef Q_OS_LINUX
	QApplication::setStyle(new DoubleClickStyle(QStyleFactory::create("Fusion")));
#endif
	QApplication a(argc, argv);
	a.installTranslator(&myTrans);
	a.installTranslator(&qtTrans);
	Interface w;
	w.show();
	return a.exec();
}
