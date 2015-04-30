/*=======================================================================
*
*   Copyright (C) 2013-2015 Lysine.
*
*   Filename:    Graphic.cpp
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

#include "Graphic.h"
#include "../Config.h"
#include "../Render/ARender.h"
#include "Mode1.h"
#include "Mode4.h"
#include "Mode5.h"
#include "Mode6.h"
#include "Mode7.h"

Graphic *Graphic::create(const Comment &comment)
{
	Graphic *graphic = nullptr;
	switch (comment.mode){
	case 1:
		graphic = new Mode1(comment);
		break;
	case 4:
		graphic = new Mode4(comment);
		break;
	case 5:
		graphic = new Mode5(comment);
		break;
	case 6:
		graphic = new Mode6(comment);
		break;
	case 7:
		graphic = new Mode7(comment);
		break;
	}
	if (graphic&&!graphic->isEnabled()){
		delete graphic;
		return nullptr;
	}
	else{
		return graphic;
	}
}

void Graphic::setIndex()
{
	static quint64 globalIndex;
	index = globalIndex++;
}
