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
#include "Local.h"
#include "Utils.h"
#include "Shield.h"
#include "Config.h"
#include "Editor.h"
#include "Danmaku.h"
#include "APlayer.h"

Info::Info(QWidget *parent):
	QWidget(parent)
{
	setObjectName("Info");
	isStay=isPoped=updating=false;
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
	volmS->setValue(Config::getValue("/Playing/Volume",50));
	timeS->setTracking(false);
	connect(timeS,&QSlider::valueChanged,[this](int _time){
		if(duration!=-1&&!updating){
			APlayer::instance()->setTime(duration*_time/400);
		}
	});
	connect(volmS,&QSlider::sliderMoved,[this](int _volm){
		QPoint p;
		p.setX(QCursor::pos().x());
		p.setY(volmS->mapToGlobal(volmS->rect().center()).y());
		QToolTip::showText(p,QString::number(_volm));
	});
	connect(volmS,&QSlider::valueChanged,[this](int _volm){
		if(!updating){
			Config::setValue("Playing/Volume",_volm);
			APlayer::instance()->setVolume(_volm);
		}
	});
	playB=new QPushButton(this);
	stopB=new QPushButton(this);
	playB->setGeometry(QRect(10,15,25,25));
	stopB->setGeometry(QRect(40,15,25,25));
	playI=QIcon::fromTheme("media-playback-start",QIcon(":/Picture/play.png"));
	stopI=QIcon::fromTheme("media-playback-stop",QIcon(":/Picture/stop.png"));
	pausI=QIcon::fromTheme("media-playback-pause",QIcon(":/Picture/pause.png"));
	playB->setIcon(playI);
	stopB->setIcon(stopI);
	playA=new QAction(playI,tr("Play"),this);
	stopA=new QAction(stopI,tr("Stop"),this);
	playA->setObjectName("Play");
	stopA->setObjectName("Stop");
	QList<QKeySequence> playS;
	playS<<Config::getValue("/Shortcut/Play",QString("Space"))<<
		   Qt::Key_MediaPlay<<
		   Qt::Key_MediaPause<<
		   Qt::Key_MediaTogglePlayPause;
	playA->setShortcuts(playS);
	stopA->setShortcut(Config::getValue("/Shortcut/Stop",QString()));
	addAction(playA);
	addAction(stopA);
	connect(playA,SIGNAL(triggered()),APlayer::instance(),SLOT(play()));
	connect(stopA,SIGNAL(triggered()),APlayer::instance(),SLOT(stop()));
	connect(playB,&QPushButton::clicked,playA,&QAction::trigger);
	connect(stopB,&QPushButton::clicked,stopA,&QAction::trigger);
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
	Utils::setSelection(danmV);
	QHeaderView *header=danmV->horizontalHeader();
	header->setSectionResizeMode(0,QHeaderView::Fixed);
	header->setSectionResizeMode(1,QHeaderView::Stretch);
	header->setHighlightSections(false);
	resizeHeader();
	connect(Danmaku::instance(),&Danmaku::layoutChanged,this,&Info::resizeHeader);
	connect(danmV,&QTableView::doubleClicked,[this](QModelIndex index){
		APlayer::instance()->setTime(((Comment *)(index.data(Qt::UserRole).value<quintptr>()))->time);
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
				for(const Comment *c:selected){
					QString sender=c->sender;
					if(!sender.isEmpty()){
						Shield::shieldS.insert(sender);
					}
				}
				Danmaku::instance()->parse(0x2);
			});
			bool flag=false;
			for(const Comment *c:selected){
				if(Shield::shieldS.contains(c->sender)){
					flag=true;
					break;
				}
			}
			if(flag){
				connect(menu.addAction(tr("Recover The Sender")),&QAction::triggered,[&](){
					for(const Comment *c:selected){
						Shield::shieldS.remove(c->sender);
					}
					Danmaku::instance()->parse(0x2);
				});
			}
			menu.addSeparator();
		}
		connect(menu.addAction(tr("Edit Blocking List")),&QAction::triggered,[this](){
			Config::exec(Local::mainWidget(),3);
		});
		connect(menu.addAction(tr("Edit Danmaku Pool" )),&QAction::triggered,[this](){
			Editor::exec(Local::mainWidget());
		});
		connect(menu.addAction(tr("Clear Danmaku Pool")),&QAction::triggered,Danmaku::instance(),&Danmaku::clearPool);
		connect(menu.addAction(tr("Save Danmaku to File")),&QAction::triggered,[this](){
			QFileDialog save(Local::mainWidget(),tr("Save File"));
			save.setAcceptMode(QFileDialog::AcceptSave);
			QFileInfo info(APlayer::instance()->getMedia());
			if(info.isFile()){
				save.setDirectory(info.absolutePath());
				save.selectFile(info.baseName());
			}
			else{
				save.setDirectory(Utils::defaultPath());
			}
			save.setDefaultSuffix("json");
			QStringList type;
			type<<tr("AcFun Danmaku Format (*.json)")<<tr("Bilibili Danmaku Format (*.xml)");
			save.setNameFilters(type);
			connect(&save,&QFileDialog::filterSelected,[&](QString filter){
				save.setDefaultSuffix(filter.indexOf("xml")==-1?"json":"xml");
			});
			if(save.exec()==QDialog::Accepted){
				QStringList file=save.selectedFiles();
				if(file.size()==1){
					Danmaku::instance()->saveToFile(file.first());
				}
			}
		});
		isStay=1;
		menu.exec(danmV->viewport()->mapToGlobal(p));
		isStay=0;
	});

	animation=new QPropertyAnimation(this,"pos",this);
	animation->setDuration(200);
	animation->setEasingCurve(QEasingCurve::OutCubic);
	connect(animation,&QPropertyAnimation::finished,[this](){
		if(!isPoped){
			hide();
		}
	});

	connect(APlayer::instance(),&APlayer::timeChanged,this,&Info::setTime);
	connect(APlayer::instance(),&APlayer::volumeChanged,[this](int volume){
		updating=1;
		volmS->setValue(volume);
		updating=0;
	});
	connect(APlayer::instance(),&APlayer::begin,[this](){
		setDuration(APlayer::instance()->getDuration());
	});
	connect(APlayer::instance(),&APlayer::reach,[this](){
		setDuration(-1);
	});
	connect(APlayer::instance(),&APlayer::stateChanged,[this](int state){
		bool playing=state==APlayer::Play;
		playB->setIcon(playing?pausI:playI);
		playA->setIcon(playing?pausI:playI);
		playA->setText(playing?tr("Pause"):tr("Play"));
	});
	hide();
}

void Info::pop()
{
	if(!isPoped&&animation->state()==QAbstractAnimation::Stopped){
		show();
		animation->setStartValue(pos());
		animation->setEndValue(pos()-QPoint(200,0));
		animation->start();
		isPoped=true;
	}
}

void Info::push(bool force)
{
	if(isPoped&&animation->state()==QAbstractAnimation::Stopped&&(!preferStay()||force)){
		if(force){
			isStay=false;
		}
		animation->setStartValue(pos());
		animation->setEndValue(pos()+QPoint(200,0));
		animation->start();
		isPoped=false;
	}
}

void Info::terminate()
{
	if(animation->state()!=QAbstractAnimation::Stopped){
		animation->setCurrentTime(animation->totalDuration());
	}
}

void Info::resizeEvent(QResizeEvent *e)
{
	danmV->setGeometry(QRect(10,170,180,e->size().height()-185));
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
