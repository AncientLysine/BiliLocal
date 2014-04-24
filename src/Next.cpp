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
#include "VPlayer.h"
#include "Danmaku.h"

static qint64 time;

Next::Next(QWidget *parent):
	QDialog(parent,Qt::FramelessWindowHint)
{
	setFixedSize(480,25);
	setAttribute(Qt::WA_TranslucentBackground);
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
	connect(nextM->addAction(tr("play immediately")),&QAction::triggered,[this](){
		done(PlayImmediately);
		VPlayer::instance()->stop();
	});
	connect(nextM->addAction(tr("inherit danmaku")) ,&QAction::triggered,std::bind(&QDialog::done,this,InheritDanmaku));
	connect(nextM->addAction(tr("wait until ended")),&QAction::triggered,std::bind(&QDialog::done,this,WaitUntilEnded));
	connect(nextM->addAction(tr("do not continnue")),&QAction::triggered,[this](){
		done(DoNotContinnue);
		fileN.clear();
	});
	connect(nextM,&QMenu::aboutToShow,[this](){
		nextM->setDefaultAction(nextM->actions()[Utils::getConfig("/Playing/Continue",true)?2:3]);
	});

	nextB=new QPushButton(tr("Action"),this);
	nextB->setMenu(nextM);
	layout->addWidget(nextB);

	connect(VPlayer::instance(),&VPlayer::timeChanged,[this](qint64 _time){
		time=_time;
		if(time>=VPlayer::instance()->getDuration()*0.9&&fileP!=VPlayer::instance()->getFile()){
			showNextDialog();
		}
	});
	connect(VPlayer::instance(),&VPlayer::begin,[this](){
		hide();
		setResult(Utils::getConfig("/Playing/Continue",true));
		fileP=fileN=QString();
	});
	connect(VPlayer::instance(),&VPlayer::reach,[this](){
		if(!fileN.isEmpty()){
			switch(result()){
			case InheritDanmaku:
				for(Record &r:Danmaku::instance()->getPool()){
					r.delay+=time;
					for(Comment &c:r.danmaku){
						c.time+=time;
					}
				}
				Danmaku::instance()->parse(0x2);
				VPlayer::instance()->setMedia(fileN);
				VPlayer::instance()->play();
				break;
			case WaitUntilEnded:
			case PlayImmediately:
				for(const Record &r:Danmaku::instance()->getPool()){
					QString code=r.string;
					int sharp=code.indexOf("#");
					if (sharp!=-1) {
						QString i=code.mid(0,sharp);
						QString p=code.mid(sharp+1);
						Load::instance()->loadDanmaku((i+"#%1").arg(p.toInt()+1));
					}
				}
				Danmaku::instance()->clearPool();
				VPlayer::instance()->setMedia(fileN);
				VPlayer::instance()->play();
				break;
			}
		}
	});
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

void Next::moveWithParent()
{
	QRect p=parentWidget()->geometry(),c=geometry();
	move(p.center().x()-c.width()/2,p.top()+2);
}

bool diffAtNum(QString f,QString s)
{
	for(int i=0;i<f.size()&&i<s.size();++i){
		if(f[i]!=s[i]){
			return f[i].isNumber()&&s[i].isNumber();
		}
	}
	return false;
}

void Next::showNextDialog()
{
	fileP=VPlayer::instance()->getFile();
	QFileInfo info(fileP),next;
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
	fileL->setText(next.fileName());
	fileL->setCursorPosition(0);
	show();
}
