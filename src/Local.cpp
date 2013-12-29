﻿/*=======================================================================
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

#include "Utils.h"
#include "Shield.h"
#include "Cookie.h"
#include "Platform.h"
#include "Interface.h"
#include <QtCore>

/*
static void setEventFilter()
{
	QAbstractNativeEventFilter *filter=Platform::getNativeEventFilter();
	if(filter!=NULL){
		qApp->installNativeEventFilter(filter);
	}
}
*/

static void setDefaultFont()
{
	QString def=Utils::defaultFont();
	QFont f=qApp->font();
	if(!QFontDatabase().families().contains(def)){
		def=QFontInfo(f).family();
	}
	f.setFamily(Utils::getConfig("/Interface/Font",def));
	qApp->setFont(f);
}

static void setToolTipBase()
{
	QPalette tip=qApp->palette();
	tip.setColor(QPalette::Inactive,QPalette::ToolTipBase,Qt::white);
	qApp->setPalette(tip);
	QToolTip::setPalette(tip);
}

int main(int argc,char *argv[])
{
	QApplication::setStyle("Fusion");
	QApplication a(argc,argv);
	QDir::setCurrent(a.applicationDirPath());
	QString locale=QLocale::system().name();
	QTranslator myTrans;
	myTrans.load(locale+".qm","./translations");
	QTranslator qtTrans;
	qtTrans.load(locale+".qt.qm","./translations");
	a.installTranslator(&myTrans);
	a.installTranslator(&qtTrans);
	Utils::loadConfig();
	Shield::init();
	Cookie::init();
    //setEventFilter();
	setDefaultFont();
	setToolTipBase();
	a.connect(&a,&QApplication::aboutToQuit,[](){
		Cookie::free();
		Shield::free();
		Utils::saveConfig();
	});
	Interface w;
	w.show();
	return a.exec();
}
