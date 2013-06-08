/*=======================================================================
*
*   Copyright (C) 2013 Lysine.
*
*   Filename:    Utils.h
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

#ifndef UTILS_H
#define UTILS_H

#include <QtCore>
#include <QtWidgets>

class Config:public QJsonObject
{
public:
	Config();
	Config(const QJsonObject& o);
	~Config();

};

struct Comment
{
	int mode;
	int font;
	qint64 time;
	QColor color;
	QString sender;
	QString content;
};
Q_DECLARE_METATYPE(Comment)

class Utils
{
public:
	template<class Func>
	static void delayExec(QObject *parent,int time,Func func)
	{
		QTimer *delay=new QTimer(parent);
		delay->setSingleShot(true);
		delay->start(time);
		parent->connect(delay,&QTimer::timeout,func);
	}

	static void setBack(QWidget *widget,QColor color)
	{
		QPalette options;
		options.setColor(QPalette::Background,color);
		widget->setPalette(options);
	}

	static void setCenter(QWidget *widget,QSize _size,bool move=true)
	{
		QRect rect;
		rect.setSize(_size);
		QRect prer=move?QApplication::desktop()->screenGeometry():widget->geometry();
		rect.moveCenter(prer.center());
		widget->setGeometry(rect);
	}

	static QVariant getSetting(QString name);
	static void setSetting(QVariant setting,QString name);
	static QJsonObject getConfig(QString area=QString());
	static void setConfig(QJsonObject _config,QString area=QString(),bool rewrite=false);
	static void loadConfig();
	static QJsonObject unionObject(QJsonObject f,QJsonObject s);

private:
	static Config config;

};

#endif // UTILS_H
