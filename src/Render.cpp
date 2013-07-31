/*=======================================================================
*
*   Copyright (C) 2013 Lysine.
*
*   Filename:    Render.cpp
*   Time:        2013/07/26
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

#include "Render.h"

Render::Render(QObject *parent):
	QObject(parent)
{
	thread=new QThread(parent);
	this->moveToThread(thread);
	thread->start();
	connect(this,SIGNAL(middle(QVariantMap)),this,SLOT(render(QVariantMap)));
}

Render::~Render()
{
	if(thread->isRunning()){
		thread->exit();
		thread->wait();
	}
	delete thread;
}

void Render::append(QVariantMap arguments)
{
	emit middle(arguments);
}

void Render::render(QVariantMap arguments)
{
	QFont font;
	font.setBold(arguments["effect"].toInt()%2);
	font.setFamily(arguments["family"].toString());
	font.setPixelSize(arguments["size"].toInt());
	QStaticText text;
	text.setText(arguments["text"].toString().replace("\n","<br>"));
	text.prepare(QTransform(),font);
	QSize textSize=text.size().toSize()+QSize(4,4);
	QPixmap fst(textSize);
	fst.fill(Qt::transparent);
	QPainter painter;
	painter.begin(&fst);
	painter.setFont(font);
	auto draw=[&](QColor c,QPoint p){
		painter.setPen(c);
		painter.drawStaticText(p+=QPoint(2,2),text);
	};
	int color=arguments["color"].toInt();
	QColor edge=qGray(color)<50?Qt::white:Qt::black;
	switch(arguments["effect"].toInt()/2){
	case 0:
		draw(edge,QPoint(+1,0));
		draw(edge,QPoint(-1,0));
		draw(edge,QPoint(0,+1));
		draw(edge,QPoint(0,-1));
		break;
	case 1:
		draw(edge,QPoint(2,2));
		break;
	}
	draw(color,QPoint(0,0));
	painter.end();
	QPixmap sec(textSize);
	sec.fill(Qt::transparent);
	painter.begin(&sec);
	painter.setOpacity(arguments["alpha"].toDouble());
	painter.drawPixmap(QPoint(0,0),fst);
	painter.end();
	arguments["result"]=sec;
	emit result(arguments);
}
