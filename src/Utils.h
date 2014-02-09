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

class Comment
{
public:
	int mode;
	int font;
	int color;
	qint64 time;
	qint64 date;
	QString sender;
	QString string;
	bool blocked;
	Comment()
	{
		mode=font=color=time=date=0;
		blocked=false;
	}
	inline bool operator ==(const Comment &o) const
	{
		return mode==o.mode&&font==o.font&&color==o.color&&sender==o.sender&&string==o.string&&date==o.date&&qAbs(time-o.time)<10;
	}
};

inline uint qHash(const Comment &c,uint seed)
{
	uint h=qHash(c.mode,seed);
	h=(h<<1)^qHash(c.font,seed);
	h=(h<<1)^qHash(c.color,seed);
	h=(h<<1)^qHash(c.date,seed);
	h=(h<<1)^qHash(c.sender,seed);
	h=(h<<1)^qHash(c.string,seed);
	return h;
}

class Record
{
public:
	bool full;
	qint64 delay;
	qint64 limit;
	QString source;
	QList<Comment> danmaku;
	Record()
	{
		full=false;
		delay=limit=0;
	}
};

namespace{
template<class T>
T fromJsonValue(QJsonValue v)
{
	return v.toVariant().value<T>();
}

template<>
QVariant fromJsonValue(QJsonValue v)
{
	return v.toVariant();
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

template<class T>
QJsonValue toJsonValue(T v)
{
	return QJsonValue(v);
}

template<>
QJsonValue toJsonValue(QVariant v)
{
	switch(v.type()){
	case QVariant::Bool:
		return v.toBool();
	case QVariant::Int:
	case QVariant::Double:
		return v.toDouble();
	case QVariant::String:
		return v.toString();
	default:
		return QJsonValue();
	}
}
}

class Utils
{
public:
	enum Site
	{
		Unknown,
		Bilibili,
		AcFun,
		Letv,
		AcPlay,
		AcfunLocalizer
	};
	static Site getSite(QString url);
	static void setCenter(QWidget *widget);
	static void setGround(QWidget *widget,QColor color);
	static void setSelection(QAbstractItemView *view);
	static QString defaultPath();
	static QString defaultFont(bool monospace=false);
	static QString splitString(QString text,int width);
	static QList<Comment> parseComment(QByteArray data,Site site,bool isSync=false);


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
		QJsonValue val=toJsonValue(set);
		if(!val.isNull()){
			cur[last]=val;
			while(!path.isEmpty()){
				QJsonObject pre=path.takeLast();
				pre[tree.takeLast()]=cur;
				cur=pre;
			}
			config=cur;
		}
	}

	template<class Func>
	static void getReply(QNetworkAccessManager *manager,const QNetworkRequest &request,Func func)
	{
		QNetworkReply *reply=manager->get(request);
		reply->connect(reply,&QNetworkReply::finished,[reply,func](){
			func(reply);
			reply->deleteLater();
		});
	}

	static void loadConfig();
	static void saveConfig();

private:
	static QJsonObject config;

};

#endif // UTILS_H
