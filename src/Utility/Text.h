/*=======================================================================
*
*   Copyright (C) 2013-2016 Lysine.
*
*   Filename:    Text.h
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

#pragma once

#include <QString>
#include <QStringList>

namespace Utils
{
	enum Site
	{
		Unknown,
		Bilibili,
		AcFun,
		Tudou,
		Letv,
		AcfunLocalizer,
		Niconico,
		TuCao,
		ASS
	};

	enum Type
	{
		Video = 1,
		Audio = 2,
		Subtitle = 4,
		Danmaku = 8
	};

	Site parseSite(QString url);
	QString defaultFont(bool monospace = false);
	QString customUrl(Site site);
	QString decodeTxt(QString &&txt);
	QString decodeXml(QString &&xml, bool fast = false);
	QStringList getSuffix(int type, QString format = QString());
}
