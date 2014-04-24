/*=======================================================================
*
*   Copyright (C) 2013 Lysine.
*
*   Filename:    Plugin.h
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

#ifndef PLUGIN_H
#define PLUGIN_H

#include <QtCore>

class Plugin
{
public:
	typedef void (*Regist)(const QHash<QString,QObject *> &);
	typedef void (*Config)();
	typedef QString (*String)(QString);

	static QList<Plugin> plugins;
	static QHash<QString,QObject *> objects;

	static void loadPlugins();

	Plugin(QString path);
	bool loaded();
	void regist(const QHash<QString,QObject *> &);
	void config();
	QString string(QString query);

private:
	Regist m_regist;
	Config m_config;
	String m_string;
};

#endif // PLUGIN_H
