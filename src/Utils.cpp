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

bool Comment::operator < (const Comment &o) const
{
	return time==o.time?string<o.string:time<o.time;
}

bool Comment::operator ==(const Comment &o) const
{
	return mode==o.mode&&color==o.color&&sender==o.sender&&string==o.string&&time==o.time&&date==o.date;
}

uint qHash(const Comment &key,uint seed)
{
	uint h=0;
	h+=qHash(key.mode,seed);
	h+=qHash(key.color,seed);
	h+=qHash(key.sender,seed);
	h+=qHash(key.string,seed);
	return h;
}

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
	conf.write(QJsonDocument(config).toJson());
	conf.close();
}

