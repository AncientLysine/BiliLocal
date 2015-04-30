/*=======================================================================
*
*   Copyright (C) 2013-2015 Lysine.
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
#include "../Local.h"
#include "../Model/List.h"
#include "../Player/APlayer.h"

using namespace UI;

Jump::Jump(QWidget *parent) :
QWidget(parent)
{
	setObjectName("Jump");
	setFixedSize(parent->minimumWidth(), 25);

	auto layout = new QHBoxLayout(this);
	layout->setMargin(0); layout->setSpacing(0);

	fileL = new QLineEdit(this);
	fileL->setReadOnly(true);
	fileL->setFocusPolicy(Qt::NoFocus);
	layout->addWidget(fileL);

	jumpB = new QPushButton(this);
	jumpB->setFixedWidth(55);
	connect(jumpB, &QPushButton::clicked, [this](){
		QStandardItem *c = List::instance()->getCurrent();
		APlayer::instance()->setTime(c->data(List::TimeRole).toDouble());
		hide();
	});
	layout->addWidget(jumpB);

	connect(APlayer::instance(), &APlayer::begin, [this](){
		QStandardItem *c = List::instance()->getCurrent();
		if (!c){
			return;
		}
		qint64 t = c->data(List::TimeRole).toDouble() / 1000;
		if (t <= 0){
			return;
		}
		fileL->setText(c->text());
		QString time("%1:%2");
		time = time.arg(t / 60, 2, 10, QChar('0'));
		time = time.arg(t % 60, 2, 10, QChar('0'));
		jumpB->setText(time);
		show();
	});
	hide();
}
