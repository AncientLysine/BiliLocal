/*=======================================================================
*
*   Copyright (C) 2013 Lysine.
*
*   Filename:    Printer.cpp
*   Time:        2013/08/14
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

#include "Printer.h"

Printer *Printer::ins=NULL;

Printer::Printer(QWidget *parent):
	QWidget(parent)
{
	ioo=0;
	ins=this;
	timer=new QTimer(this);
	delay=new QTimer(this);
	connect(timer,&QTimer::timeout,[this](){
		if(ioo==0){
			timer->stop();
		}
		else if(ioo==1){
			if(effect->opacity()>=0.9){
				effect->setOpacity(1.0);
				timer->stop();
				ioo=2;
			}
			else{
				effect->setOpacity(effect->opacity()+0.1);
			}
		}
		else if(ioo==3){
			if(effect->opacity()<=0.1){
				effect->setOpacity(0.0);
				timer->stop();
				ioo=0;
				hide();
				list.clear();
			}
			else{
				effect->setOpacity(effect->opacity()-0.1);
			}
		}
	});
	connect(delay,&QTimer::timeout,this,&Printer::fadeOut);
	connect(this,SIGNAL(receive(QString)),this,SLOT(process(QString)));
	effect=new QGraphicsOpacityEffect(this);
	effect->setOpacity(0.0);
	setGraphicsEffect(effect);
	QFont f=font();
	f.setPointSize(10);
	setFont(f);
	hide();
	stream.setDevice(new QFile("Log.txt",this));
	stream.device()->open(QIODevice::Append|QIODevice::Text);
}

void Printer::paintEvent(QPaintEvent *e)
{
	QPainter painter;
	painter.begin(this);
	painter.fillRect(rect(),QColor(0,0,0,20));
	painter.setPen(Qt::white);
	auto iter=list.begin();
	int h=5,w=width()-20;
	for(;iter!=list.end();++iter){
		if(iter->textWidth()!=w){
			iter->setTextWidth(w);
		}
		int i=iter->size().height();
		if(h+i>height()-5){
			break;
		}
		painter.drawStaticText(10,h,*iter);
		h+=i;
	}
	painter.end();
	list.erase(iter,list.end());
	QWidget::paintEvent(e);
}

void Printer::process(QString content)
{
	if(Utils::getConfig("/Interface/Debug",false)){
		fadeIn();
		list.prepend(QStaticText(content));
		update();
		delay->start(8000);
		stream<<(QString("[%1]").arg(QDateTime::currentDateTime().toString("hh:mm:ss.zzz"))+content).trimmed()<<endl;
	}
}

void Printer::fadeIn()
{
	if(ioo==0||ioo==3){
		ioo=1;
		timer->start(50);
		show();
	}
}

void Printer::fadeOut()
{
	if(ioo==2||ioo==1){
		ioo=3;
		timer->start(50);
	}
}