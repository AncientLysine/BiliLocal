/*=======================================================================
*
*   Copyright (C) 2013 Lysine.
*
*   Filename:    Utils.cpp
*   Time:        2013/05/10
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

QJsonObject Utils::config;

void Utils::setBack(QWidget *widget,QColor color)
{
	QPalette options;
	options.setColor(QPalette::Window,color);
	widget->setPalette(options);
}

void Utils::setCenter(QWidget *widget)
{
	QPoint center;
	QWidget *parent=widget->parentWidget();
	if(parent==NULL){
		center=QApplication::desktop()->screenGeometry(widget).center();
	}
	else if(widget->isWindow()){
		center=parent->geometry().center();
	}
	else{
		center=parent->rect().center();
	}
	QRect rect=widget->geometry();
	rect.moveCenter(center);
	widget->setGeometry(rect);
}

void Utils::loadConfig()
{
	QFile conf("./Config.txt");
	conf.open(QIODevice::ReadOnly|QIODevice::Text);
	config=QJsonDocument::fromJson(conf.readAll()).object();
	conf.close();
}

void Utils::saveConfig()
{
	QFile conf("./Config.txt");
	conf.open(QIODevice::WriteOnly|QIODevice::Text);
	QString buffer(QJsonDocument(config).toJson());
	QRegExp regexp(": [\\d|\\.]+");
	int cur=0;
	while((cur=regexp.indexIn(buffer,cur))!=-1){
		QString cap=regexp.cap();
		buffer.replace(cap,QString(": %1").arg(cap.mid(2).toDouble()));
		cur+=regexp.matchedLength();
	}
	conf.write(buffer.toUtf8());
	conf.close();
}

