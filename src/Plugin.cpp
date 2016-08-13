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
#include "Utils.h"

QList<Plugin> Plugin::plugins;

Plugin::Plugin(QString path)
{
	m_regist = (RegistPtr)QLibrary::resolve(path, "regist");
	m_config = (ConfigPtr)QLibrary::resolve(path, "config");
	m_string = (StringPtr)QLibrary::resolve(path, "string");
}

bool Plugin::loaded()
{
	return m_regist && m_config && m_string;
}

void Plugin::regist(QObject *app)
{
	if (m_regist)
		m_regist(app);
}

void Plugin::config()
{
	if (m_config)
		m_config();
}

QString Plugin::string(QString query)
{
	QString result;
	m_string(&query, &result);
	return result;
}

void Plugin::load()
{
	QFileInfoList list = QDir(Utils::localPath(Utils::Plugin)).entryInfoList();
	for (const QFileInfo &info : list){
		if (info.isFile() && QLibrary::isLibrary(info.fileName())){
			Plugin lib(info.absoluteFilePath());
			if (lib.loaded()) {
				bool success = false;
				try {
					if (Config::getValue("/Plugin/" + lib.string("Name"), true)) {
						lib.regist(lApp);
						success = true;
					}
				}
				catch (...) {
					qDebug() << QString("Plugin: failed to regist %1").arg(info.fileName());
				}
				if (success == false) {
					lib.m_regist = nullptr;
					lib.m_config = nullptr;
				}
				plugins.append(lib);
			}
		}
	}
}
