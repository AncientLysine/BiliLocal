/*=======================================================================
*
*   Copyright (C) 2013 Lysine.
*
*   Filename:    Config.h
*   Time:        2013/06/17
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

#ifndef CONFIG_H
#define CONFIG_H

#include <QtCore>
#include <QtWidgets>
#include <QtNetwork>

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

class ConfigPrivate;

class Config:public QDialog
{
	Q_OBJECT
public:
	explicit Config(QWidget *parent=0,int index=0);
	~Config();

	template<class T>
	static T getValue(QString key,T def=T())
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
			setValue(key,def);
			return def;
		}
	}

	template<class T>
	static void setValue(QString key,T set)
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

	static void load();
	static void save();

	static void setManager(QNetworkAccessManager *manager);

private:
	ConfigPrivate * const d_ptr;
	Q_DECLARE_PRIVATE(Config)

	static QJsonObject config;
};

#endif // CONFIG_H
