/*=======================================================================
*
*   Copyright (C) 2013-2016 Lysine.
*
*   Filename:    Graphic.h
*   Time:        2013/10/19
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

#include "../Define/Comment.h"
#include <QtGui>
#include <QtCore>
#include <exception>

class Graphic
{
public:
	class format_unrecognized :public std::runtime_error
	{
	public:
		format_unrecognized() :
			runtime_error("format unrecognized")
		{
		}
	};

	class args_prasing_error :public std::runtime_error
	{
	public:
		args_prasing_error() :
			runtime_error("args prasing error")
		{
		}
	};

	virtual QList<QRectF> locate() = 0;
	virtual bool move(double time) = 0;
	virtual void draw(QPainter *painter) = 0;
	virtual uint intersects(Graphic *other) = 0;
	virtual bool stay(){ return false; }
	virtual QRectF &currentRect(){ return rect; }
	virtual ~Graphic() = default;

	inline int getMode() const
	{
		return source ? source->mode : 0;
	}

	inline bool isEnabled() const
	{
		return enabled;
	}

	inline void setEnabled(bool enabled)
	{
		this->enabled = enabled;
	}

	inline quint64 getIndex() const
	{
		return index;
	}

	void setIndex();

	inline const Comment *getSource() const
	{
		return source;
	}

	inline void setSource(const Comment *source)
	{
		this->source = source;
	}

	static Graphic *create(const Comment &comment);

protected:
	bool enabled;
	QRectF rect;
	quint64 index;
	const Comment *source;
	Graphic() :enabled(true), source(nullptr){}
};
