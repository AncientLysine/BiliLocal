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

Config Utils::config;

Config::Config()
{
}

Config::Config(const QJsonObject &o):
	QJsonObject(o)
{
}

Config::~Config()
{
	QFile conf("./Config.txt");
	conf.open(QIODevice::WriteOnly|QIODevice::Text);
	conf.write(QJsonDocument(*this).toJson());
	conf.close();
}

void Utils::setBack(QWidget *widget,QColor color)
{
	QPalette options;
	options.setColor(QPalette::Window,color);
	widget->setPalette(options);
}

QJsonValue Utils::findConfig(QString name,QJsonObject s)
{
	if(s.contains(name)){
		return s.value(name);
	}
	else{
		for(auto i:s){
			if(i.isObject()){
				auto v=findConfig(name,i.toObject());
				if(!v.isNull()){
					return v;
				}
			}
		}
		return QJsonValue();
	}
}

QJsonObject Utils::getConfig(QString area)
{
	if(area.isEmpty()){
		return config;
	}
	else if(config.contains(area)){
		return config[area].toObject();
	}
	else{
		return QJsonObject();
	}
}

void Utils::setConfig(QJsonObject _config,QString area,bool rewrite)
{
	if(area.isEmpty()){
		config=rewrite?_config:unionObject(_config,config);
	}
	else{
		config[area]=rewrite?_config:unionObject(_config,config[area].toObject());
	}
}

void Utils::loadConfig()
{
	QFile conf("./Config.txt");
	conf.open(QIODevice::ReadOnly|QIODevice::Text);
	auto read=QJsonDocument::fromJson(conf.readAll()).object();
	conf.close();
	config=unionObject(config,read);
}

QJsonObject Utils::unionObject(QJsonObject f,QJsonObject s)
{
	for(auto iter=s.begin();iter!=s.end();++iter){
		if(!f.contains(iter.key())){
			f[iter.key()]=iter.value();
		}
		else if(f[iter.key()].isObject()){
			f[iter.key()]=unionObject(f[iter.key()].toObject(),s[iter.key()].toObject());
		}
	}
	return f;
}
