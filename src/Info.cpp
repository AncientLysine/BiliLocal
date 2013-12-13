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
#include "Utils.h"
#include "Shield.h"
#include "Config.h"
#include "Editor.h"
#include "Danmaku.h"
#include "VPlayer.h"

Info::Info(QWidget *parent):
	QWidget(parent)
{
	isPop=false;
	isStay=false;
	updating=false;
	Utils::setGround(this,Qt::white);
	duration=-1;
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
		if(duration!=-1&&!updating){
			VPlayer::instance()->setTime(duration*_time/400);
		}
	});
	connect(volmS,&QSlider::valueChanged,[this](int _volm){
		Utils::setConfig("Playing/Volume",_volm);
		VPlayer::instance()->setVolume(_volm);
	});
	playB=new QPushButton(this);
	stopB=new QPushButton(this);
	playB->setGeometry(QRect(10,15,25,25));
	stopB->setGeometry(QRect(40,15,25,25));
	playI=QIcon(":/Picture/play.png");
	stopI=QIcon(":/Picture/stop.png");
	playB->setIcon(playI);
	stopB->setIcon(stopI);
	pauseI=QIcon(":/Picture/pause.png");
	playA=new QAction(playI,tr("Play"),this);
	stopA=new QAction(stopI,tr("Stop"),this);
	addAction(playA);
	addAction(stopA);
	connect(playA,&QAction::triggered,VPlayer::instance(),&VPlayer::play);
	connect(stopA,&QAction::triggered,VPlayer::instance(),&VPlayer::stop);
	connect(playB,&QPushButton::clicked,playA,&QAction::trigger);
	connect(stopB,&QPushButton::clicked,stopA,&QAction::trigger);
	playA->setShortcut(QKeySequence(Qt::Key_Space));
	durT=new QLabel(this);
	durT->setAlignment(Qt::AlignRight|Qt::AlignBottom);
	durT->setGeometry(QRect(70,15,120,25));
	durT->setText("00:00/00:00");
	danmV=new QTableView(this);
	danmV->setWordWrap(false);
	danmV->setSelectionBehavior(QAbstractItemView::SelectRows);
	danmV->setSelectionMode(QAbstractItemView::ExtendedSelection);
	danmV->verticalHeader()->hide();
	danmV->setAlternatingRowColors(true);
	danmV->setContextMenuPolicy(Qt::CustomContextMenu);
	danmV->setModel(Danmaku::instance());
	QHeaderView *header=danmV->horizontalHeader();
	header->setSectionResizeMode(0,QHeaderView::Fixed);
	header->setSectionResizeMode(1,QHeaderView::Stretch);
	header->setHighlightSections(false);
	resizeHeader();
	connect(Danmaku::instance(),&Danmaku::modelReset,this,&Info::resizeHeader);
	connect(danmV,&QTableView::doubleClicked,[this](QModelIndex index){
		VPlayer::instance()->setTime(((Comment *)(index.data(Qt::UserRole).value<quintptr>()))->time);
	});
	connect(danmV,&QTableView::customContextMenuRequested,[this](QPoint p){
		QMenu menu(this);
		QList<const Comment *>selected;
		for(const QModelIndex &index:danmV->selectionModel()->selectedRows()){
			selected.append((Comment *)index.data(Qt::UserRole).value<quintptr>());
		}
		if(!selected.isEmpty()){
			connect(menu.addAction(tr("Copy Danmaku")),&QAction::triggered,[&](){
				QStringList list;
				for(const Comment *c:selected){
					list.append(c->string);
				}
				qApp->clipboard()->setText(list.join('\n'));
			});
			connect(menu.addAction(tr("Eliminate The Sender")),&QAction::triggered,[&](){
				QList<QString> &list=Shield::shieldU;
				for(const Comment *c:selected){
					QString sender=c->sender;
					if(!sender.isEmpty()&&!list.contains(sender)){
						list.append(sender);
					}
				}
				Danmaku::instance()->parse(0x2);
			});
		}
		connect(menu.addAction(tr("Edit Blocking List")),&QAction::triggered,[this](){
			Config config(parentWidget(),2);
			config.exec();
			Danmaku::instance()->parse(0x2);
		});
		if(danmV->model()->rowCount()){
			connect(menu.addAction(tr("Edit Danmaku Pool")),&QAction::triggered,[this](){
				int state=VPlayer::instance()->getState();
				if(state==VPlayer::Play) VPlayer::instance()->play();
				Editor::exec(parentWidget());
				Danmaku::instance()->parse(0x1|0x2);
				if(state==VPlayer::Play) VPlayer::instance()->play();
			});
			connect(menu.addAction(tr("Clear Danmaku Pool")),&QAction::triggered,Danmaku::instance(),&Danmaku::clearPool);
			connect(menu.addAction(tr("Save Danmaku to File")),&QAction::triggered,[this](){
				QString path=VPlayer::instance()->getFile();
				if(!path.isEmpty()){
					QFileInfo info(path);
					path=info.absolutePath()+'/'+info.baseName()+".json";
				}
				QString file=QFileDialog::getSaveFileName(parentWidget(),
														  tr("Save File"),
														  path,
														  "",
														  0,
														  QFileDialog::DontUseNativeDialog);
				if(!file.isEmpty()){
					if(!file.endsWith(".json")){
						file.append(".json");
					}
					Danmaku::instance()->saveToFile(file);
				}
			});
		}
		isStay=1;
		menu.exec(danmV->viewport()->mapToGlobal(p));
		isStay=0;
	});
	connect(VPlayer::instance(),&VPlayer::begin,[this](){
		setDuration(VPlayer::instance()->getDuration());
	});
	connect(VPlayer::instance(),&VPlayer::reach,[this](){
		setDuration(-1);
	});
	connect(VPlayer::instance(),&VPlayer::stateChanged,[this](int state){
		bool playing=state==VPlayer::Play;
		playB->setIcon(playing?pauseI:playI);
		playA->setIcon(playing?pauseI:playI);
		playA->setText(playing?tr("Pause"):tr("Play"));
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

void Info::push(bool force)
{
	if(isPop&&animation->state()==QAbstractAnimation::Stopped&&(!isStay||force)){
		animation->setStartValue(pos());
		animation->setEndValue(pos()+QPoint(200,0));
		animation->start();
		isPop=false;
	}
}

void Info::trigger()
{
	playA->trigger();
}

void Info::terminate()
{
	if(animation->state()!=QAbstractAnimation::Stopped){
		animation->setCurrentTime(animation->totalDuration());
	}
}

void Info::resizeHeader()
{
	Danmaku *d=Danmaku::instance();
	QStringList list;
	list.append(d->headerData(0,Qt::Horizontal,Qt::DisplayRole).toString());
	int c=d->rowCount()-1,i;
	for(i=0;i<=c;++i){
		if(d->data(d->index(i,0),Qt::ForegroundRole).value<QColor>()!=Qt::red){
			list.append(d->data(d->index(i,0),Qt::DisplayRole).toString());
			break;
		}
	}
	for(i=c;i>=0;--i){
		if(d->data(d->index(i,0),Qt::ForegroundRole).value<QColor>()!=Qt::red){
			list.append(d->data(d->index(i,0),Qt::DisplayRole).toString());
			break;
		}
	}
	int m=0;
	for(QString item:list){
		m=qMax(m,danmV->fontMetrics().width(item)+8);
	}
	danmV->horizontalHeader()->resizeSection(0,m);
}

void Info::setTime(qint64 _time)
{
	if(!timeS->isSliderDown()){
		updating=1;
		timeS->setValue(_time*400/duration);
		updating=0;
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

void Info::setDuration(qint64 _duration)
{
	if(_duration>0){
		duration=_duration;
		timeS->setRange(0,400);
	}
	else{
		duration=-1;
		timeS->setValue(0);
		timeS->setRange(0,0);
		durT->setText("00:00/00:00");
	}
}
