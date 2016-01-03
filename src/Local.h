/*=======================================================================
*
*   Copyright (C) 2013-2015 Lysine.
*
*   Filename:    Local.h
*   Time:        2014/05/10
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

#pragma once

#include <QApplication>
#include <QHash>
#include <QObject>
#include <QString>
#include <QWidget>
#include <QThreadPool>

#define lApp (static_cast<Local *>(QCoreApplication::instance()))

#define qThreadPool QThreadPool::globalInstance()

class Local :public QApplication
{
	Q_OBJECT
public:
	Local(int &argc, char **argv);

	static Local *instance()
	{
		return lApp;
	}

	static QHash<QString, QObject *> objects;

public slots:
	void exit(int code = 0);

	QWidget *mainWidget()
	{
		return qobject_cast<QWidget *>(objects["Interface"]);
	}

	QObject *findObject(QString name)
	{
		return objects[name];
	}

	void synchronize(void *func)
	{
		((void(*)())func)();
	}

	void synchronize(void *func, void *args)
	{
		((void(*)(void *))func)(args);
	}
};
