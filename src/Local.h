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

#include <QHash>
#include <QObject>
#include <QString>
#include <QThreadPool>

#define lApp Local::instance()

#define qThreadPool QThreadPool::globalInstance()

class Local :public QObject
{
	Q_OBJECT
public:
	explicit Local(QObject *parent = 0);
	virtual ~Local();

	//last instance of app
	static Local *instance();

	QHash<QByteArray, QObject *> objects;

	template<class T>
	T *findObject() const
	{
		return qobject_cast<T *>(objects.value(T::staticMetaObject.className()));
	}

private:
	static Local *ins;

public slots:
	QObject *findObject(QByteArray name) const
	{
		return objects.value(name);
	}

	void synchronize(void *func) const
	{
		((void(*)())func)();
	}

	void synchronize(void *func, void *args) const
	{
		((void(*)(void *))func)(args);
	}

	void tryLocal(QString path);
};
