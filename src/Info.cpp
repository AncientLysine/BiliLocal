/*=======================================================================
*
*   Copyright (C) 2013 Lysine.
*
*   Filename:    Info.cpp
*   Time:        2013/04/05
*   Author:      Lysine
*   Contributor: Chaserhkj
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

#include "Info.h"

Info::Info(QWidget *parent):
	QWidget(parent)
{
	isPop=false;
	opened=false;
	playing=false;
	sliding=false;
	updating=false;
	setAutoFillBackground(true);
	Utils::setBack(this,Qt::white);
	duration=100;
	animation=new QPropertyAnimation(this,"pos",this);
	animation->setDuration(200);
	animation->setEasingCurve(QEasingCurve::OutCubic);
	timeT=new QLabel(tr("Time"),this);
	volmT=new QLabel(tr("Volume"),this);
	timeT->setGeometry(QRect(10,50,100,25));
	volmT->setGeometry(QRect(10,100,100,25));
	timeS=new QSlider(this);
	volmS=new QSlider(this);
	timeS->setOrientation(Qt::Horizontal);
	volmS->setOrientation(Qt::Horizontal);
	timeS->setGeometry(QRect(10,75,180,15));
	volmS->setGeometry(QRect(10,125,180,15));
	timeS->setRange(0,0);
	volmS->setRange(0,100);
	timeS->setValue(0);
	volmS->setValue(Utils::getConfig("/Playing/Volume",100));
	timeS->setTracking(false);
	volmS->setTracking(false);
	connect(timeS,&QSlider::valueChanged,[this](int _time){
		if(!updating){
			emit time(duration*_time/400);
		}
	});
	connect(timeS,&QSlider::sliderPressed, [this](){sliding=true;});
	connect(timeS,&QSlider::sliderReleased,[this](){sliding=false;});
	connect(volmS,&QSlider::valueChanged,[this](int _volm){
		Utils::setConfig("Playing/Volume",_volm);
		emit volume(_volm);
	});
	playB=new QPushButton(this);
	stopB=new QPushButton(this);
	playB->setGeometry(QRect(10,15,25,25));
	stopB->setGeometry(QRect(40,15,25,25));
	playB->setIcon(QIcon(":/Picture/play.png"));
	stopB->setIcon(QIcon(":/Picture/stop.png"));
	playA=new QAction(QIcon(":/Picture/play.png"),tr("Play"),this);
	stopA=new QAction(QIcon(":/Picture/stop.png"),tr("Stop"),this);
	connect(playA,&QAction::triggered,[this](){
		if(opened){
			setPlaying(!playing);
		}
		emit play();
	});
	connect(stopA,&QAction::triggered,[this](){emit stop();});
	this->addAction(playA);
	this->addAction(stopA);
	connect(playB,&QPushButton::clicked,playA,&QAction::trigger);
	connect(stopB,&QPushButton::clicked,stopA,&QAction::trigger);
	playA->setShortcut(QKeySequence(Qt::Key_Space));
	durT=new QLabel(this);
	durT->setAlignment(Qt::AlignRight|Qt::AlignBottom);
	durT->setGeometry(QRect(70,15,120,25));
	durT->setText("00:00/00:00");
	danmV=new QTableView(this);
	danmV->setWordWrap(false);
	danmV->setSelectionMode(QAbstractItemView::SingleSelection);
	danmV->setSelectionBehavior(QAbstractItemView::SelectRows);
	danmV->verticalHeader()->hide();
	danmV->setAlternatingRowColors(true);
	danmV->setContextMenuPolicy(Qt::CustomContextMenu);
	danmV->setModel(Danmaku::instance());
	danmV->horizontalHeader()->setSectionResizeMode(0,QHeaderView::ResizeToContents);
	danmV->horizontalHeader()->setSectionResizeMode(1,QHeaderView::Stretch);
	danmV->horizontalHeader()->setHighlightSections(false);
	connect(danmV->model(),&QAbstractItemModel::layoutChanged,[this](){
		danmV->horizontalHeader()->setSectionResizeMode(0,QHeaderView::ResizeToContents);
	});
	connect(danmV,&QWidget::customContextMenuRequested,[this](QPoint p){
		QMenu menu(this);
		QModelIndex index=danmV->currentIndex();
		if(index.isValid()){
			connect(menu.addAction(tr("Eliminate The Sender")),&QAction::triggered,[this,index](){
				QList<QString> &list=Shield::shieldU;
				QString sender=index.data(Qt::UserRole).toString();
				if(!list.contains(sender)){
					list.append(sender);
				}
				Shield::cacheS.clear();
			});
		}
		connect(menu.addAction(tr("Edit Blocking List")),&QAction::triggered,[this](){
			Config config(parentWidget(),2);
			config.exec();
			Danmaku::instance()->parse(0x2);
		});
		if(danmV->model()->rowCount()){
			connect(menu.addAction(tr("Edit Danmaku Pool")),&QAction::triggered,[this](){
				Editor editor(parentWidget());
				editor.exec();
			});
			connect(menu.addAction(tr("Clear Danmaku Pool")),&QAction::triggered,[this](){
				Danmaku::instance()->clearPool();
				danmV->setCurrentIndex(QModelIndex());
			});
			connect(menu.addAction(tr("Save Danmaku to File")),&QAction::triggered,[this](){
				QString filter=tr("Danmaku files (*.json)");
				QString lastPath=Utils::getConfig("/Playing/Path",QDir::homePath());
				QString file=QFileDialog::getSaveFileName(parentWidget(),tr("Save File"),lastPath,filter);
				if(!file.isEmpty()){
					if(!file.endsWith(".json")){
						file.append(".json");
					}
					Danmaku::instance()->saveToFile(file);
				}
			});
		}
		menu.exec(danmV->viewport()->mapToGlobal(p));
	});
}

void Info::resizeEvent(QResizeEvent *e)
{
	danmV->setGeometry(QRect(10,170,180,e->size().height()-185));
}

void Info::pop()
{
	if(!isPop&&animation->state()==QAbstractAnimation::Stopped){
		animation->setStartValue(pos());
		animation->setEndValue(pos()-QPoint(200,0));
		animation->start();
		isPop=true;
	}
}

void Info::push()
{
	if(isPop&&animation->state()==QAbstractAnimation::Stopped){
		animation->setStartValue(pos());
		animation->setEndValue(pos()+QPoint(200,0));
		animation->start();
		isPop=false;
		parentWidget()->setFocus();
	}
}

void Info::terminate()
{
	if(animation->state()!=QAbstractAnimation::Stopped){
		animation->setCurrentTime(animation->totalDuration());
	}
}

void Info::setTime(qint64 _time)
{
	if(!sliding){
		updating=true;
		timeS->setValue(_time*400/duration);
		updating=false;
	}
	int c=_time/1000;
	int s=duration/1000;
	auto to=[](int num){
		QString res=QString::number(num);
		res.prepend(QString(2-res.length(),'0'));
		return res;
	};
	QString t;
	t+=to(c/60)+':'+to(c%60);
	t+='/';
	t+=to(s/60)+':'+to(s%60);
	durT->setText(t);
}

void Info::setOpened(bool _opened)
{
	opened=_opened;
	setPlaying(opened);
}

void Info::setPlaying(bool _playing)
{
	playing=_playing;
	playB->setIcon(QIcon(playing?":/Picture/pause.png":":/Picture/play.png"));
	playA->setIcon(QIcon(playing?":/Picture/pause.png":":/Picture/play.png"));
	playA->setText(playing?tr("Pause"):tr("Play"));
}

void Info::setDuration(qint64 _duration)
{
	if(_duration>0){
		duration=_duration;
		timeS->setRange(0,400);
	}
	else{
		duration=100;
		timeS->setValue(0);
		timeS->setRange(0,0);
		durT->setText("00:00/00:00");
	}
}
