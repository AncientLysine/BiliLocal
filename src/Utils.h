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
	bool operator < (const Comment &o) const;
	bool operator ==(const Comment &o) const;
};
Q_DECLARE_METATYPE(Comment)

namespace{
template<class T>
T fromJsonValue(QJsonValue v)
{
	return v.toVariant().value<T>();
}

template<>
QJsonArray fromJsonValue(QJsonValue v)
{
	return v.toArray();
}

template<>
QJsonObject fromJsonValue(QJsonValue v)
{
	return v.toObject();
}
}

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
	static T getConfig(QString key,T def=T())
	{
		QStringList tree=key.split('/',QString::SkipEmptyParts);
		QString last=tree.takeLast();
		QJsonObject cur=config;
		for(const QString &k:tree){
			cur=cur.value(k).toObject();
		}
		if(cur.contains(last)){
			return fromJsonValue<T>(cur.value(last));
		}
		else{
			setConfig(key,def);
			return def;
		}
	}

	template<class T>
	static void setConfig(QString key,T set)
	{
		QStringList tree=key.split('/',QString::SkipEmptyParts);
		QString last=tree.takeLast();
		QJsonObject cur=config;
		QList<QJsonObject> path;
		for(const QString &k:tree){
			path.append(cur);
			cur=cur.value(k).toObject();
		}
		cur[last]=set;
		while(!path.isEmpty()){
			QJsonObject pre=path.takeLast();
			pre[tree.takeLast()]=cur;
			cur=pre;
		}
		config=cur;
	}

	static void loadConfig();
	static void saveConfig();

private:
	static QJsonObject config;

};

#endif // UTILS_H
