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
#include <QtNetwork>

struct Comment
{
	int mode;
	int font;
	int color;
	qint64 time;
	qint64 date;
	QString sender;
	QString string;
	inline bool operator < (const Comment &o) const
	{
		return time<o.time;
	}
	inline bool operator ==(const Comment &o) const
	{
		return mode==o.mode&&font==o.font&&color==o.color&&sender==o.sender&&string==o.string&&time==o.time&&date==o.date;
	}
};

struct Record
{
	qint64 delay;
	QString source;
	QList<Comment> danmaku;
	Record(QString s,const QList<Comment> &d=QList<Comment>(),qint64 l=0):
		delay(l),source(s),danmaku(d){}
};

inline uint qHash(const Comment &key,uint seed=0)
{
	uint h=qHash(key.mode,seed);
	h=(h<<1)^qHash(key.font,seed);
	h=(h<<1)^qHash(key.color,seed);
	h=(h<<1)^qHash(key.time,seed);
	h=(h<<1)^qHash(key.date,seed);
	h=(h<<1)^qHash(key.sender,seed);
	h=(h<<1)^qHash(key.string,seed);
	return h;
}

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
	static void setCenter(QWidget *widget);
	static void setGround(QWidget *widget,QColor color);
	static QString split(QString text,int width);

	template<class Sender,class Func>
	static void delayExec(Sender *parent,int time,Func func)
	{
		QTimer *delay=new QTimer(parent);
		delay->setSingleShot(true);
		delay->start(time);
		delay->connect(delay,&QTimer::timeout,[=](){
			func();
			delay->deleteLater();
		});
	}

	template<class Sender,class Wait,class Func>
	static void delayExec(Sender *parent,Wait wait,Func func)
	{
		QMetaObject::Connection *connect=new QMetaObject::Connection;
		*connect=QObject::connect(parent,wait,[=](){
			func();
			QObject::disconnect(*connect);
			delete connect;
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

	template<class Func>
	static void getReply(QNetworkAccessManager *manager,const QNetworkRequest &request,Func func)
	{
		QNetworkReply *reply=manager->get(request);
		reply->connect(reply,&QNetworkReply::finished,[reply,func](){func(reply);});
	}

	static void loadConfig();
	static void saveConfig();

private:
	static QJsonObject config;

};

#endif // UTILS_H
