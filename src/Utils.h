/*=======================================================================
*
*   Copyright (C) 2013-2015 Lysine.
*
*   Filename:    Utils.h
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

#include <QtCore>
#include <QtGui>

class Comment
{
public:
	int mode;
	int font;
	int color;
	qint64 time;
	qint64 date;
	QString sender;
	QString string;
	bool blocked;

	Comment()
	{
		mode = font = color = time = date = 0;
		blocked = false;
	}

	inline bool operator <(const Comment &o) const
	{
		return time < o.time;
	}

	inline bool operator==(const Comment &o) const
	{
		return mode == o.mode && font == o.font && color == o.color && qFuzzyCompare((float)time, (float)o.time) && date == o.date && sender == o.sender && string == o.string;
	}

	inline bool isLocal() const
	{
		return date == 0 && sender.isEmpty();
	}

	inline bool isEmpty() const
	{
		return mode == 0 && font == 0 && color == 0 && time == 0 && date == 0 && sender.isEmpty() && string.isEmpty() && !blocked;
	}
};

inline uint qHash(const Comment &c, uint seed = 0)
{
	uint h = qHash(c.mode, seed);
	h = (h << 1) ^ qHash(c.font, seed);
	h = (h << 1) ^ qHash(c.color, seed);
	h = (h << 1) ^ qHash(c.date, seed);
	h = (h << 1) ^ qHash(c.sender, seed);
	h = (h << 1) ^ qHash(c.string, seed);
	return h;
}

class Record
{
public:
	bool full;
	qint64 delay;
	qint64 limit;
	QString source;
	QString string;
	QString access;
	QVector<Comment> danmaku;

	Record()
	{
		full = false;
		delay = limit = 0;
	}
};

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
	QString decodeTxt(const QByteArray &data);
	QString decodeXml(QString string, bool fast = false);
	QString decodeXml(QStringRef ref, bool fast = false);
	QStringList getSuffix(int type, QString format = QString());
	double evaluate(QString expression);
}
