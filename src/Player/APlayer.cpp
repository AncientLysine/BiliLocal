/*=======================================================================
*
*   Copyright (C) 2013-2015 Lysine.
*
*   Filename:    VPlayer.cpp
*   Time:        2013/03/18
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

#include "APlayer.h"
#include "../Config.h"
#include "../Utils.h"

#ifdef BACKEND_VLC
#include "VPlayer.h"
#endif
#ifdef BACKEND_QMM
#include "QPlayer.h"
#endif
#ifdef BACKEND_NIL
#include "NPlayer.h"
#endif

QStringList APlayer::getModules()
{
	QStringList modules;
#ifdef BACKEND_VLC
	modules << "VLC";
#endif
#ifdef BACKEND_QMM
	modules << "QMM";
#endif
#ifdef BACKEND_NIL
	modules << "NIL";
#endif
	return modules;
}

APlayer *APlayer::ins = nullptr;

APlayer *APlayer::instance()
{
	if (ins){
		return ins;
	}
	QString d;
	QStringList l = getModules();
	switch (l.size()){
	case 0:
		break;
	case 1:
		d = l[0];
		break;
	default:
		d = Config::getValue("/Performance/Decode", l[0]);
		d = l.contains(d) ? d : l[0];
		break;
	}
#ifdef BACKEND_VLC
	if (d == "VLC"){
		return new VPlayer(qApp);
	}
#endif
#ifdef BACKEND_QMM
	if (d == "QMM"){
		return new QPlayer(qApp);
	}
#endif
#ifdef BACKEND_NIL
	if (d == "NIL"){
		return new NPlayer(qApp);
	}
#endif
	return 0;
}

void APlayer::setRate(double)
{
}

double APlayer::getRate()
{
	return 0;
}

qint64 APlayer::getDelay(int)
{
	return 0;
}

void APlayer::setDelay(int, qint64)
{
}

void APlayer::addSubtitle(QString)
{
}

void APlayer::event(int)
{
}
