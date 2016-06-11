/*=======================================================================
*
*   Copyright (C) 2013-2015 Lysine.
*
*   Filename:    Interface.cpp
*   Time:        2013/03/18
*   Author:      Lysine
*   Contributor: Chaserhkj
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
#include "Interface.h"
#include "../Config.h"
#include <QtCore>

#ifdef INTERFACE_WIDGET
#include "Widget/WidgetInterfacePrivate.h"
#endif

#ifdef INTERFACE_QUICK2
#include "Quick2/Quick2InterfacePrivate.h"
#endif

QStringList Interface::getModules()
{
	QStringList modules;
#ifdef INTERFACE_WIDGET
	modules << "Widget";
#endif
#ifdef INTERFACE_QUICK2
	modules << "Quick2";
#endif

	return modules;
}

Interface::Interface(QObject *parent)
	: QObject(parent), d_ptr(nullptr)
{
	setObjectName("Interface");
}

Interface::~Interface()
{
	delete d_ptr;
}

void Interface::setup()
{
	QString name;
	QStringList l = getModules();
	switch (l.size()) {
	case 0:
		break;
	case 1:
		name = l[0];
		break;
	default:
		name = Config::getValue("/Interface/Type", l[0]);
		name = l.contains(name) ? name : l[0];
		break;
	}
#ifdef INTERFACE_WIDGET
	if (name == "Widget") {
		d_ptr = new WidgetInterfacePrivate();
		return;
	}
#endif
#ifdef INTERFACE_QUICK2
	if (name == "Quick2") {
		d_ptr = new Quick2InterfacePrivate();
		return;
	}
#endif
}

void Interface::warning(QString title, QString text)
{
	Q_D(Interface);
	d->warning(title, text);
}

void Interface::percent(double degree)
{
	Q_D(Interface);
	d->percent(degree);
}

void Interface::show()
{
	Q_D(Interface);
	d->show();
}

void Interface::hide()
{
	Q_D(Interface);
	d->hide();
}

QWidget *Interface::widget()
{
	Q_D(Interface);
	return d ? d->widget() : nullptr;
}

QWindow *Interface::window()
{
	Q_D(Interface);
	return d ? d->window() : nullptr;
}
