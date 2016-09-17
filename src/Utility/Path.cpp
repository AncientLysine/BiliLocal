/*=======================================================================
*
*   Copyright (C) 2013-2016 Lysine.
*
*   Filename:    Text.cpp
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

#include "Common.h"
#include "Path.h"
#include <QCoreApplication>
#include <QStandardPaths>

QString Utils::localPath(Path path)
{
#ifdef Q_OS_WIN32
	QString base = qApp->applicationDirPath() + '/';
	switch (path){
	case Cache:
		return base + "cache/";
	case Config:
		return base;
	case Locale:
		return base + "locale/";
	case Plugin:
		return base + "plugins/";
	case Script:
		return base + "scripts/";
	default:
		return base;
	}
#else
	switch (path){
	case Cache:
		return QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + '/';
	case Config:
		return QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + '/';
	case Locale:
		return QStandardPaths::writableLocation(QStandardPaths::DataLocation) + "/Locale/";
	case Plugin:
		return QStandardPaths::writableLocation(QStandardPaths::DataLocation) + "/Plugin/";
	case Script:
		return QStandardPaths::writableLocation(QStandardPaths::DataLocation) + "/Script/";
	default:
		return QStandardPaths::writableLocation(QStandardPaths::DataLocation) + '/';
	}
#endif
}
