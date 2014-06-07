/*=======================================================================
*
*   Copyright (C) 2013 Lysine.
*
*   Filename:    Next.cpp
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

#include "Next.h"
#include "Load.h"
#include "Local.h"
#include "Config.h"
#include "VPlayer.h"
#include "Danmaku.h"
#include <functional>

Next *Next::ins=NULL;

Next *Next::instance()
{
	return ins?ins:new Next(Local::mainWidget());
}

Next::Next(QWidget *parent):
	QDialog(parent,Qt::FramelessWindowHint)
{
	duration=0;
	ins=this;
	setFixedSize(480,25);
	setAttribute(Qt::WA_TranslucentBackground);
	setObjectName("Next");
	setWindowOpacity(Config::getValue("/Interface/Floating/Alpha",60)/100.0);
	moveWithParent();
	parent->installEventFilter(this);
	auto layout=new QHBoxLayout(this);
	layout->setMargin(0);
	layout->setSpacing(0);

	fileL=new QLineEdit(this);
	fileL->setReadOnly(true);
	fileL->setFocusPolicy(Qt::NoFocus);
	layout->addWidget(fileL);

	nextM=new QMenu(this);
	nextM->addAction(tr("play immediately"));
	nextM->addAction(tr("inherit danmaku"));
	nextM->addAction(tr("wait until ended"));
	nextM->addAction(tr("do not continnue"));
	QList<QAction *> optA=nextM->actions();
	optA[0]->setObjectName("Imme");
	optA[1]->setObjectName("Inhe");
	optA[2]->setObjectName("Wait");
	optA[3]->setObjectName("Dont");
	connect(optA[0],&QAction::triggered,[this](){
		done(PlayImmediately);
		VPlayer::instance()->stop(false);
	});
	connect(optA[1],&QAction::triggered,std::bind(&QDialog::done,this,InheritDanmaku));
	connect(optA[2],&QAction::triggered,std::bind(&QDialog::done,this,WaitUntilEnded));
	connect(optA[3],&QAction::triggered,std::bind(&QDialog::done,this,DoNotContinnue));
	connect(nextM,&QMenu::aboutToShow,[this](){
		nextM->setDefaultAction(nextM->actions()[Config::getValue("/Playing/Continue",true)?2:3]);
	});

	nextB=new QPushButton(tr("Action"),this);
	nextB->setMenu(nextM);
	layout->addWidget(nextB);

	connect(VPlayer::instance(),&VPlayer::timeChanged,[this](qint64 _time){
		duration=_time;
		if(_time>=0&&
				_time>=VPlayer::instance()->getDuration()*0.9&&
				!fileN.isEmpty()&&
				fileC!=VPlayer::instance()->getMedia()&&
				!Config::getValue("/Playing/Loop",false)){
			show();
			fileC=VPlayer::instance()->getMedia();
		}
	});
	connect(VPlayer::instance(),&VPlayer::begin,this,&Next::parse);
	connect(VPlayer::instance(),&VPlayer::reach,this,&Next::shift);
}

QString Next::getNext()
{
	return result()==DoNotContinnue?QString():fileN;
}

bool Next::eventFilter(QObject *,QEvent *e)
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

static bool diffAtNum(QString f,QString s)
{
	for(int i=0;i<f.size()&&i<s.size();++i){
		if(f[i]!=s[i]){
			return f[i].isNumber()&&s[i].isNumber();
		}
	}
	return false;
}

void Next::parse()
{
	clear();
	QFileInfo info(VPlayer::instance()->getMedia()),next;
	for(QFileInfo iter:info.absoluteDir().entryInfoList()){
		if(iter.isFile()&&iter.suffix()==info.suffix()){
			QString n=iter.absoluteFilePath(),c=info.absoluteFilePath();
			if(diffAtNum(n,c)&&n>c&&(!next.isFile()||n<next.absoluteFilePath())){
				next=iter;
			}
		}
	}
	if(!next.isFile()){
		return;
	}
	fileN=next.absoluteFilePath();
	emit nextChanged(fileN);
	fileL->setText(next.fileName());
	fileL->setCursorPosition(0);
}

void Next::clear()
{
	hide();
	setResult(Config::getValue("/Playing/Continue",true));
	fileC=fileN=QString();
	emit nextChanged(fileN);
	fileL->clear();
}

void Next::shift()
{
	if(!fileN.isEmpty()){
		switch(result()){
		case InheritDanmaku:
			for(Record &r:Danmaku::instance()->getPool()){
				r.delay-=duration;
				for(Comment &c:r.danmaku){
					c.time-=duration;
				}
			}
			Danmaku::instance()->parse(0x2);
			VPlayer::instance()->setMedia(fileN,false);
			VPlayer::instance()->play();
			break;
		case WaitUntilEnded:
		case PlayImmediately:
			for(const Record &r:Danmaku::instance()->getPool()){
				if(QUrl(r.source).isLocalFile()){
					continue;
				}
				QString code=r.string;
				int sharp=code.indexOf("#");
				if (sharp!=-1) {
					QString i=code.mid(0,sharp);
					QString p=code.mid(sharp+1);
					Load::instance()->loadDanmaku((i+"#%1").arg(p.toInt()+1));
				}
			}
			Danmaku::instance()->clearPool();
			VPlayer::instance()->setMedia(fileN,false);
			VPlayer::instance()->play();
			break;
		}
	}
	hide();
}

void Next::moveWithParent()
{
	QRect p=parentWidget()->geometry(),c=geometry();
	move(p.center().x()-c.width()/2,p.top()+2);
}
