/*=======================================================================
*
*   Copyright (C) 2013 Lysine.
*
*   Filename:    Jump.cpp
*   Time:        2013/04/22
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

#include "Jump.h"
#include "Load.h"
#include "Local.h"
#include "Config.h"
#include "APlayer.h"
#include "Danmaku.h"
#include <functional>

Jump::Jump(QWidget *parent):
	QDialog(parent,Qt::FramelessWindowHint)
{
	setFixedSize(parent->minimumWidth(),25);
	setAttribute(Qt::WA_TranslucentBackground);
	setObjectName("Jump");
	setWindowOpacity(Config::getValue("/Interface/Floating/Alpha",60)/100.0);
	moveWithParent();
	parent->installEventFilter(this);

	auto layout=new QHBoxLayout(this);
	layout->setMargin(0);layout->setSpacing(0);

	fileL=new QLineEdit(this);
	fileL->setReadOnly(true);
	fileL->setFocusPolicy(Qt::NoFocus);
	layout->addWidget(fileL);

}

bool Jump::eventFilter(QObject *,QEvent *e)
{
	switch(e->type()){
	case QEvent::Move:
	case QEvent::Resize:
		moveWithParent();
		return false;
	default:
		return false;
	}
}

void Jump::moveWithParent()
{
	QRect p=parentWidget()->geometry(),c=geometry();
	move(p.center().x()-c.width()/2,p.top()+2);
}
