/*=======================================================================
*
*   Copyright (C) 2013 Lysine.
*
*   Filename:    Render.h
*   Time:        2013/06/09
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

#ifndef RENDER_H
#define RENDER_H

#include <QtCore>
#include <QtWidgets>
#include "VPlayer.h"
#include "Danmaku.h"

class Render:public QWidget
{
	Q_OBJECT
public:
	explicit Render(QWidget *parent=0);
	void setVplayer(VPlayer *_vplayer){vplayer=_vplayer;}
	void setDanmaku(Danmaku *_danmaku){danmaku=_danmaku;}

private:
	VPlayer *vplayer;
	Danmaku *danmaku;
	void paintEvent(QPaintEvent *e);

public slots:
	void updateVplayer();
	void updateDanmaku();

};

#endif // RENDER_H
