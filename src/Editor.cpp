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
	interval=100;
	pool=Danmaku::instance()->getPool();
	for(auto &line:pool){
		qSort(line.danmaku);
	}
	resize(width(),pool.count()*interval);
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
	auto keys=pool.keys();
	int l=e->rect().bottom()/interval+1;
	int s=point.isNull()?-1:point.y()/interval;
	for(int i=e->rect().top()/interval;i<l;++i){
		const Record &r=pool[keys[i]];
		int w=width()-100,h=i*interval;
		painter.fillRect(0,h,width(),interval,Qt::gray);
		if(i==s){
			painter.drawPixmap(100+mapFromGlobal(QCursor::pos()).x()-point.x(),h,snapshot);
		}
		painter.fillRect(0,h,100-2,interval-2,Qt::white);
		QStringList text;
		text<<QFileInfo(keys[i]).fileName();
		text<<translation.arg(r.delay/1000);
		painter.drawText(0,h,100-2,interval-2,Qt::AlignCenter,text.join("\n"));
		if(i==s){
			continue;
		}
		const QList<Comment> &line=r.danmaku;
		int c=0,m=0;
		for(;c<line.count()&&line[c].time<0;++c);
		QList<int> height;
		for(int j=0;j<w;j+=5){
			int n=c;
			qint64 e=(j+5)*duration/w;
			for(;c<line.count()&&line[c].time<e;++c);
			n=c-n;
			m=m>n?m:n;
			height.append(n);
		}
		if(m==0){
			continue;
		}
		for(int j=0;j<w;j+=5){
			int he=height.takeFirst()*(interval-2)/m;
			painter.fillRect(100+j,h+interval-2-he,5,he,Qt::white);
		}
	}
	painter.end();
	QWidget::paintEvent(e);
}

void Widget::mouseMoveEvent(QMouseEvent *e)
{
	if(point.isNull()){
		snapshot=QPixmap(width()-100,interval);
		render(&snapshot,QPoint(),QRegion(100,e->y()/interval*interval,width()-100,interval));
		point=e->pos();
	}
	else{
		update(QRegion(100,point.y()/interval*interval,width()-100,interval));
	}
}

void Widget::mouseReleaseEvent(QMouseEvent *e)
{
	if(!point.isNull()){
		auto &p=Danmaku::instance()->getPool();
		auto &r=p[p.keys()[point.y()/interval]];
		qint64 d=(e->x()-point.x())*duration/(width()-100);
		r.delay+=d;
		for(Comment &c:r.danmaku){
			c.time+=d;
		}
		pool=p;
		for(auto &line:pool){
			qSort(line.danmaku);
		}
		update();
	}
	point=QPoint();
	snapshot=QPixmap();
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
	resize(640,450);
	setMinimumSize(300,200);
	setWindowTitle(tr("Editor"));
}

Editor::~Editor()
{
	Danmaku::instance()->parse(0x1);
}

void Editor::resizeEvent(QResizeEvent *)
{
	Utils::delayExec(0,[this](){widget->resize(scroll->viewport()->width(),widget->height());});
}
