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

class Config:public QJsonObject
{
public:
	Config();
	Config(const QJsonObject& o);
	~Config();
};

class Utils
{
public:
	static void setBack(QWidget *widget,QColor color);

	template<class Func>
	static void delayExec(int time,Func func)
	{
		QTimer *delay=new QTimer;
		delay->setSingleShot(true);
		delay->start(time);
		delay->connect(delay,&QTimer::timeout,[=](){
			func();
			delay->deleteLater();
		});
	}

	template<class T>
	static T getSetting(QString name,T def=T())
	{
		QJsonObject o=config["Global"].toObject();
		if(o.contains(name)){
			return o.value(name).toVariant().value<T>();
		}
		else{
			o[name]=def;
			config["Global"]=o;
			return def;
		}
	}

	template<class T>
	static void setSetting(T setting,QString name)
	{
		QJsonObject o=config["Global"].toObject();
		o[name]=setting;
		config["Global"]=o;
	}

	static QJsonValue findConfig(QString name,QJsonObject s=config);
	static QJsonObject getConfig(QString area=QString());
	static void setConfig(QJsonObject _config,QString area=QString(),bool rewrite=false);
	static void loadConfig();
	static QJsonObject unionObject(QJsonObject f,QJsonObject s);

private:
	static Config config;

};

#endif // UTILS_H
