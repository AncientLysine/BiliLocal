/*=======================================================================
*
*   Copyright (C) 2013 Lysine.
*
*   Filename:    Editor.cpp
*   Time:        2013/06/30
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

#include "Editor.h"

Widget::Widget(QWidget *parent,QString trans):
	QWidget(parent),translation(trans)
{
	length=100;
	pool=Danmaku::instance()->getPool();
	for(auto &line:pool){
		qSort(line.danmaku);
	}
	resize(width(),pool.count()*length);
	current=VPlayer::instance()->getTime();
	duration=VPlayer::instance()->getDuration();
	if(duration==-1){
		for(auto &line:pool){
			qint64 t=line.danmaku.last().time+5000-line.delay;
			duration=duration>t?duration:t;
		}
	}
}

void Widget::paintEvent(QPaintEvent *e)
{
	QPainter painter;
	painter.begin(this);
	painter.fillRect(rect(),Qt::gray);
	const QList<QString> &keys=pool.keys();
	int l=e->rect().bottom()/length+1;
	int s=point.isNull()?-1:point.y()/length;
	for(int i=e->rect().top()/length;i<l;++i){
		const Record &r=pool[keys[i]];
		int w=width()-100,h=i*length;
		painter.fillRect(0,h,100-2,length-2,Qt::white);
		QStringList text;
		text<<QFileInfo(keys[i]).fileName();
		text<<translation.arg(r.delay/1000);
		painter.drawText(0,h,100-2,length-2,Qt::AlignCenter,text.join("\n"));
		int m=0,d=5*duration/w;
		QHash<int,int> c;
		for(const Comment &com:r.danmaku){
			int k=(com.time-r.delay)/d,v=c.value(k,0)+1;
			c.insert(k,v);
			m=v>m?v:m;
		}
		if(m==0){
			continue;
		}
		int o=w*r.delay/duration+100;
		if(i==s){
			o+=mapFromGlobal(QCursor::pos()).x()-point.x();
		}
		painter.setClipRect(100,h,w,length);
		for(int j=0;j<w;j+=5){
			int he=c[j/5]*(length-2)/m;
			painter.fillRect(o+j,h+length-2-he,5,he,Qt::white);
		}
		painter.setClipping(false);
	}
	if(current>=0){
		painter.fillRect(current*(width()-100)/duration+100,0,1,height(),Qt::red);
	}
	painter.end();
	QWidget::paintEvent(e);
}

void Widget::mouseMoveEvent(QMouseEvent *e)
{
	if(point.isNull()){
		point=e->pos();
	}
	else{
		update(100,point.y()/length*length,width()-100,length);
	}
}

void Widget::mouseReleaseEvent(QMouseEvent *e)
{
	if(!point.isNull()){
		auto &p=Danmaku::instance()->getPool();
		auto &r=p[p.keys()[point.y()/length]];
		qint64 d=(e->x()-point.x())*duration/(width()-100);
		r.delay+=d;
		for(Comment &c:r.danmaku){
			c.time+=d;
		}
		pool=p;
		for(auto &line:pool){
			qSort(line.danmaku);
		}
		update(0,point.y()/length*length,100,length);
	}
	point=QPoint();
}

Editor::Editor(QWidget *parent):
	QDialog(parent)
{
	layout=new QGridLayout(this);
	scroll=new QScrollArea(this);
	widget=new Widget(this,tr("Delay: %1s"));
	layout->addWidget(scroll);
	scroll->setWidget(widget);
	scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	state=VPlayer::instance()->getState();
	resize(640,450);
	setMinimumSize(300,200);
	setWindowTitle(tr("Editor"));
	if(state==VPlayer::Play){
		VPlayer::instance()->play();
	}
}

Editor::~Editor()
{
	if(state==VPlayer::Play){
		VPlayer::instance()->play();
	}
	Danmaku::instance()->parse(0x1);
}

void Editor::resizeEvent(QResizeEvent *)
{
	Utils::delayExec(0,[this](){widget->resize(scroll->viewport()->width(),widget->height());});
}
