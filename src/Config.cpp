/*=======================================================================
*
*   Copyright (C) 2013-2015 Lysine.
*
*   Filename:    Config.cpp
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

#include "Common.h"
#include "Config.h"
#include "Local.h"
#include "Utils.h"

QReadWriteLock Config::lock;

Config::Config(QObject *parent)
	: QObject(parent)
{
}

QJsonObject Config::config;

void Config::load()
{
	QFile conf(Utils::localPath(Utils::Config) + "Config.txt");
	if (conf.open(QIODevice::ReadOnly | QIODevice::Text)){
		config = QJsonDocument::fromJson(conf.readAll()).object();
		conf.close();
	}
}

void Config::save()
{
	emit lApp->findObject<Config>()->aboutToSave();
	QFile conf(Utils::localPath(Utils::Config) + "Config.txt");
	conf.open(QIODevice::WriteOnly | QIODevice::Text);
	conf.write(QJsonDocument(config).toJson());
	conf.close();
}

void     Config::setVariant(QString key, QVariant val)
{
	setValue(key, val);
}

QVariant Config::getVariant(QString key, QVariant val)
{
	return getValue(key, val);
}
