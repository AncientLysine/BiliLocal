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
#include <QtNetwork>
#include <QtWidgets>

namespace{
template<class TypeName>
TypeName fromJsonValue(QJsonValue v)
{
	QVariant t=v.toVariant();
	if(!t.canConvert<TypeName>()){
		throw("type missmatch");
	}
	return t.value<TypeName>();
}

template<>
QVariant fromJsonValue(QJsonValue v)
{
	return v.toVariant();
}

template<>
QJsonArray fromJsonValue(QJsonValue v)
{
	if (QJsonValue::Array!=v.type()){
		throw("type missmatch");
	}
	return v.toArray();
}

template<>
QJsonObject fromJsonValue(QJsonValue v)
{
	if (QJsonValue::Object!=v.type()){
		throw("type missmatch");
	}
	return v.toObject();
}

template<class TypeName>
QJsonValue toJsonValue(TypeName v)
{
	return QJsonValue(v);
}

template<>
QJsonValue toJsonValue(QVariant v)
{
	return QJsonValue::fromVariant(v);
}
}

class Config:public QObject
{
	Q_OBJECT
public:
	explicit Config(QObject *parent=0);

	template<class T>
	static T getValue(QString key,T def=T())
	{
		QStringList tree=key.split('/',QString::SkipEmptyParts);
		QString last=tree.takeLast();
		QJsonObject cur=config;
		QList<QJsonObject> path;
		for(const QString &k:tree){
			path.append(cur);
			cur=cur.value(k).toObject();
		}
		if (cur.contains(last)){
			try{
				return fromJsonValue<T>(cur.value(last));
			}
			catch(...){}
		}
		QJsonValue val=toJsonValue(def);
		if(!val.isNull()){
			cur[last]=val;
			while(!path.isEmpty()){
				QJsonObject pre=path.takeLast();
				pre[tree.takeLast()]=cur;
				cur=pre;
			}
			config=cur;
		}
		return def;
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
		cur[last]=toJsonValue(set);
		while(!path.isEmpty()){
			QJsonObject pre=path.takeLast();
			pre[tree.takeLast()]=cur;
			cur=pre;
		}
		config=cur;
	}

	static Config *instance();

private:
	static Config *ins;
	static QJsonObject config;

public slots:
	static void exec(QWidget *parent=0,int index=0);
	static void load();
	static void save();
	static void setManager(QNetworkAccessManager *manager);
	void     setVariant(QString key,QVariant val);
	QVariant getVariant(QString key,QVariant val=QVariant());
};

#endif // CONFIG_H
