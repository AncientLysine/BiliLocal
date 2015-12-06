/*=======================================================================
*
*   Copyright (C) 2013-2015 Lysine.
*
*   Filename:    Plugin.cpp
*   Time:        2013/04/23
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
#include "Plugin.h"
#include "Config.h"
#include "Local.h"

QList<Plugin> Plugin::plugins;

Plugin::Plugin(QString path)
{
	m_regist = (RegistPtr)QLibrary::resolve(path, "regist");
	m_config = (ConfigPtr)QLibrary::resolve(path, "config");
	m_string = (StringPtr)QLibrary::resolve(path, "string");
}

bool Plugin::loaded()
{
	return m_regist&&m_config&&m_string;
}

void Plugin::regist(const QHash<QString, QObject *> &objects)
{
	if (m_regist)
		m_regist(objects);
}

void Plugin::config(QWidget *parent)
{
	if (m_config)
		m_config(parent);
}

QString Plugin::string(QString query)
{
	return m_string(query);
}

void Plugin::loadPlugins()
{
	QFileInfoList list = QDir("./plugins/bililocal/").entryInfoList();
	for (const QFileInfo &info : list){
		if (info.isFile() && QLibrary::isLibrary(info.fileName())){
			Plugin lib(info.absoluteFilePath());
			if (lib.loaded()){
				if (Config::getValue("/Plugin/" + lib.string("Name"), true)){
					lib.regist(Local::objects);
				}
				else{
					lib.m_config = nullptr;
					lib.m_regist = nullptr;
				}
				plugins.append(lib);
			}
		}
	}
}
