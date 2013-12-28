/*=======================================================================
*
*   Copyright (C) 2013 Lysine.
*
*   Filename:    Interface.cpp
*   Time:        2013/03/18
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

#include "Interface.h"
#include "Utils.h"
#include "Menu.h"
#include "Info.h"
#include "Post.h"
#include "Render.h"
#include "Shield.h"
#include "Config.h"
#include "VPlayer.h"
#include "Danmaku.h"

Interface::Interface(QWidget *parent):
	QWidget(parent)
{
	setAcceptDrops(true);
	setMinimumSize(550,390);
	setWindowIcon(QIcon(":/Picture/icon.png"));
	setCenter(QSize(),true);
	vplayer=new VPlayer(this);
	danmaku=new Danmaku(this);
	menu=new Menu(this);
	info=new Info(this);
	post=new Post(this);
	menu->hide();
	info->hide();
	render=new Render;
	render->installEventFilter(this);
	manager=createWindowContainer(render,this);
	timer=new QTimer(this);
	delay=new QTimer(this);
	timer->start(200);
	connect(timer,&QTimer::timeout,[this](){
		QPoint cur=mapFromGlobal(QCursor::pos());
		int x=cur.x(),y=cur.y(),w=width(),h=height();
		if(isActiveWindow()&&y>-50&&y<h+50){
			if(x>=0&&x<50){
				setIndex(1);
			}
			if(x>250&&x<w-250){
				setIndex(0);
			}
			if(x>w-50&&x<=w){
				setIndex(-1);
			}
			if(x>200&&x<w-200){
				if(y>h-65&&(vplayer->getState()!=VPlayer::Stop||post->isValid())){
					post->fadeIn();
				}
				if(y<h-85){
					post->fadeOut();
				}
			}
			else{
				post->fadeOut();
			}
		}
		if(cur!=pre){
			pre=cur;
			if(!menu->isVisible()&&!info->isVisible()){
				if(cursor().shape()==Qt::BlankCursor){
					unsetCursor();
				}
				delay->start(2000);
			}
			else{
				delay->stop();
			}
		}
		if(vplayer->getState()==VPlayer::Play){
			qint64 time=vplayer->getTime();
			info->setTime(time);
			post->setTime(time);
			danmaku->setTime(time);
		}
	});
	connect(delay,&QTimer::timeout,[this](){
		if(vplayer->getState()==VPlayer::Play){
			setCursor(QCursor(Qt::BlankCursor));
		}
	});
	connect(menu->getPower(),&QTimer::timeout,this,&Interface::drawPowered);
	connect(danmaku,&Danmaku::layoutChanged,render,&Render::draw);
	connect(vplayer,&VPlayer::begin,[this](){
		if(!isFullScreen()){
			sca->setEnabled(true);
			setCenter(vplayer->getSize(),false);
		}
		rat->setEnabled(true);
	});
	connect(vplayer,&VPlayer::reach,[this](){
		danmaku->resetTime();
		danmaku->clearCurrent();
		rat->defaultAction()->setChecked(true);
		rat->setEnabled(false);
		sca->defaultAction()->setChecked(true);
		sca->setEnabled(false);
		vplayer->setRatio(0);
		if(!isFullScreen()){
			setCenter(QSize(),false);
		}
	});
	connect(vplayer,&VPlayer::decode,this,&Interface::drawDecoded);
	connect(vplayer,&VPlayer::jumped,danmaku,&Danmaku::jumpToTime);

	addActions(menu->actions());
	addActions(info->actions());

	quitA=new QAction(tr("Quit"),this);
	quitA->setShortcut(QKeySequence("Ctrl+Q"));
	addAction(quitA);
	connect(quitA,&QAction::triggered,&QApplication::quit);

	fullA=new QAction(tr("Full Screen"),this);
	fullA->setCheckable(true);
	fullA->setChecked(false);
	fullA->setShortcut(QKeySequence("F"));
	addAction(fullA);
	connect(fullA,&QAction::toggled,[this](bool b){
		if(!b){
			showNormal();
			if(vplayer->getState()!=VPlayer::Stop){
				sca->setEnabled(true);
			}
		}
		else{
			showFullScreen();
			sca->setEnabled(false);
		}
	});

	confA=new QAction(tr("Config"),this);
	confA->setShortcut(QKeySequence("Ctrl+I"));
	addAction(confA);
	connect(confA,&QAction::triggered,[this](){
		Config config(this);
		config.exec();
		danmaku->parse(0x2);
	});
	
	toggA=new QAction(tr("Block All"),this);
	toggA->setCheckable(true);
	toggA->setChecked(Shield::block[7]);
	toggA->setShortcut(QKeySequence("Ctrl+T"));
	addAction(toggA);
	connect(toggA,&QAction::triggered,[this](bool b){
		Shield::block[7]=b;
		danmaku->parse(0x2);
	});
	connect(danmaku,&Danmaku::layoutChanged,[this](){
		toggA->setChecked(Shield::block[7]);
	});

	QActionGroup *g;
	rat=new QMenu(tr("Ratio"),this);
	rat->setEnabled(false);
	g=new QActionGroup(rat);
	rat->setDefaultAction(g->addAction(tr("Default")));
	g->addAction("4:3");
	g->addAction("16:9");
	g->addAction("16:10");
	for(QAction *a:g->actions()){
		a->setCheckable(true);
	}
	rat->defaultAction()->setChecked(true);
	rat->addActions(g->actions());
	connect(rat,&QMenu::triggered,[this](QAction *action){
		if(action->text()==tr("Default")){
			vplayer->setRatio(0);
		}
		else{
			QStringList l=action->text().split(':');
			vplayer->setRatio(l[0].toDouble()/l[1].toDouble());
		}
		if(vplayer->getState()==VPlayer::Pause){
			render->draw();
		}
	});

	sca=new QMenu(tr("Scale"),this);
	sca->setEnabled(false);
	g=new QActionGroup(sca);
	g->addAction("1:4");
	g->addAction("1:2");
	sca->setDefaultAction(g->addAction("1:1"));
	g->addAction("2:1");
	for(QAction *a:g->actions()){
		a->setCheckable(true);
	}
	sca->defaultAction()->setChecked(true);
	sca->addActions(g->actions());
	connect(sca,&QMenu::triggered,[this](QAction *action){
		if(vplayer->getSize().isValid()){
			QStringList l=action->text().split(':');
			QSize s=vplayer->getSize()*l[0].toInt()/l[1].toInt();
			setCenter(s,false);
		}
	});

	index=0;
	animation=new QPropertyAnimation(manager,"pos",this);
	animation->setDuration(200);
	animation->setEasingCurve(QEasingCurve::OutCubic);
	connect(animation,&QPropertyAnimation::finished,[this](){
		if(index==0){
			menu->hide();
			info->hide();
		}
	});

	if(Utils::getConfig("/Interface/Frameless",false)){
		setWindowFlags(Qt::CustomizeWindowHint);
	}
	if(Utils::getConfig("/Interface/Top",false)){
		setWindowFlags(windowFlags()|Qt::WindowStaysOnTopHint);
	}
	for(const QString &file:QApplication::arguments().mid(1)){
		menu->openLocal(file);
	}
	setFocus();
}

bool Interface::eventFilter(QObject *,QEvent *e)
{
	switch(e->type()){
	case QEvent::Drop:
	case QEvent::KeyPress:
	case QEvent::MouseButtonPress:
	case QEvent::MouseButtonRelease:
	case QEvent::MouseButtonDblClick:
	case QEvent::DragEnter:
		event(e);
		return true;
	default:
		return false;
	}
}

void Interface::dropEvent(QDropEvent *e)
{
	if(e->mimeData()->hasFormat("text/uri-list")){
		for(const QString &item:QString(e->mimeData()->data("text/uri-list")).split('\n')){
			menu->openLocal(QUrl(item).toLocalFile().trimmed());
		}
	}
}

void Interface::resizeEvent(QResizeEvent *e)
{
	manager->resize(e->size());
	int w=e->size().width(),h=e->size().height();
	QMetaObject::invokeMethod(this,"saveSize",Qt::QueuedConnection);
	menu->setGeometry(0,    0,200,h);
	info->setGeometry(w-200,0,200,h);
	post->setGeometry(qMax(400,w-800)/2,h-65,qMin(800,w-400),50);
	QWidget::resizeEvent(e);
}

void Interface::keyPressEvent(QKeyEvent *e)
{
	int jmp=Utils::getConfig("Playing/Interval",10)*1000;
	switch(e->key()){
	case Qt::Key_Left:
		jmp=-jmp;
	case Qt::Key_Right:
		if(vplayer->getState()==VPlayer::Play){
			vplayer->setTime(vplayer->getTime()+jmp);
		}
		break;
	case Qt::Key_Escape:
		if(isFullScreen()){
			fullA->toggle();
		}
	}
	QWidget::keyPressEvent(e);
}

void Interface::mouseMoveEvent(QMouseEvent *e)
{
	if(sta.isNull()){
		sta=e->globalPos();
		wgd=pos();
	}
	else if((windowFlags()&Qt::CustomizeWindowHint)!=0&&!isFullScreen()){
		move(wgd+e->globalPos()-sta);
	}
	QWidget::mouseMoveEvent(e);
}

void Interface::mouseReleaseEvent(QMouseEvent *e)
{
	if(!menu->geometry().contains(e->pos())&&!info->geometry().contains(e->pos())){
		setIndex(0);
	}
	sta=wgd=QPoint();
	if(e->button()==Qt::RightButton){
		showMenu(e->pos());
	}
	QWidget::mouseReleaseEvent(e);
}

void Interface::dragEnterEvent(QDragEnterEvent *e)
{
	if(e->mimeData()->hasFormat("text/uri-list")){
		e->acceptProposedAction();
	}
	QWidget::dragEnterEvent(e);
}

void Interface::mouseDoubleClickEvent(QMouseEvent *e)
{
	if(!menu->isVisible()&&!info->isVisible()&&!post->isShown()){
		fullA->toggle();
	}
	QWidget::mouseDoubleClickEvent(e);
}

void Interface::drawDecoded()
{
	if(!menu->getPower()->isActive()){
		render->draw();
	}
}

void Interface::drawPowered()
{
	if(vplayer->getState()==VPlayer::Play){
		render->draw();
	}
}

void Interface::saveSize()
{
	if(vplayer->getState()==VPlayer::Stop&&!isFullScreen()&&!isMaximized()){
		Utils::setConfig("/Interface/Size",QString("%1,%2").arg(width()).arg(height()));
	}
}

void Interface::showMenu(QPoint p)
{
	bool flag=true;
	flag=flag&&!(menu->isVisible()&&menu->geometry().contains(p));
	flag=flag&&!(info->isVisible()&&info->geometry().contains(p));
	if(flag){
		QMenu top(this);
		const Comment *cur=danmaku->commentAt(p);
		if(cur){
			QAction *text=new QAction(&top);
			top.addAction(text);
			int w=top.actionGeometry(text).width()-24;
			text->setText(top.fontMetrics().elidedText(cur->string,Qt::ElideRight,w));
			top.addSeparator();
			connect(top.addAction(tr("Copy")),&QAction::triggered,[=](){
				qApp->clipboard()->setText(cur->string);
			});
			connect(top.addAction(tr("Eliminate The Sender")),&QAction::triggered,[=](){
				QList<QString> &list=Shield::shieldU;
				QString sender=cur->sender;
				if(!sender.isEmpty()&&!list.contains(sender)){
					list.append(sender);
				}
				Danmaku::instance()->parse(0x2);
			});
			top.addSeparator();
		}
		top.addActions(info->actions());
		top.addAction(fullA);
		top.addAction(toggA);
		top.addActions(menu->actions());
		QMenu *sub=new QMenu(tr("Subtitle"),this);
		QMenu *vid=new QMenu(tr("Video Track"),this);
		QMenu *aud=new QMenu(tr("Audio Track"),this);
		sub->addActions(vplayer->getSubtitles());
		sub->setEnabled(!sub->isEmpty());
		vid->addActions(vplayer->getVideoTracks());
		vid->setEnabled(!vid->isEmpty());
		aud->addActions(vplayer->getAudioTracks());
		aud->setEnabled(!aud->isEmpty());
		QMenu *tra=new QMenu(tr("Track"),this);
		tra->addMenu(sub);
		tra->addMenu(vid);
		tra->addMenu(aud);
		tra->setEnabled(vplayer->getState()!=VPlayer::Stop);
		top.addMenu(tra);
		top.addMenu(sca);
		top.addMenu(rat);
		top.addAction(confA);
		top.addAction(quitA);
		top.exec(mapToGlobal(p));
	}
}

void Interface::setIndex(int i)
{
	if(animation->state()==QAbstractAnimation::Stopped){
		bool force=false;
		if(qAbs(i)==1){
			if(index==0){
				animation->setStartValue(manager->pos());
				index=i;
				animation->setEndValue(QPoint(200*i,0));
				if(i==1){
					menu->show();
					menu->setFocus();
				}
				else{
					info->show();
					info->setFocus();
				}
				animation->start();
			}
			else if(i!=index){
				force=true;
			}
		}
		if(force||(((index==1&&!menu->preferStay())||(index==-1&&!info->preferStay()))&&i==0)){
			index=0;
			animation->setStartValue(manager->pos());
			animation->setEndValue(QPoint(0,0));
			animation->start();
			setFocus();
		}
	}
}

void Interface::setCenter(QSize _s,bool f)
{
	if(!_s.isValid()){
		QStringList l=Utils::getConfig("/Interface/Size",QString("960,540")).split(QRegExp("\\D"),QString::SkipEmptyParts);
		if(l.size()==2){
			_s=QSize(l[0].toInt(),l[1].toInt());
		}
	}
	QSize m=minimumSize();
	QRect r;
	r.setSize(QSize(qMax(m.width(),_s.width()),qMax(m.height(),_s.height())));
	QRect s=QApplication::desktop()->screenGeometry(this);
	QRect t=f?s:geometry();
	if((windowFlags()&Qt::CustomizeWindowHint)==0){
		s.setTop(s.top()+style()->pixelMetric(QStyle::PM_TitleBarHeight));
	}
	bool flag=true;
	if(r.width()>=s.width()||r.height()>=s.height()){
		if(isVisible()){
			fullA->toggle();
			flag=false;
		}
		else{
			r.setSize(QSize(960,540));
		}
	}
	if(flag){
		r.moveCenter(t.center());
		if(r.top()<s.top()){
			r.moveTop(s.top());
		}
		if(r.bottom()>s.bottom()){
			r.moveBottom(s.bottom());
		}
		if(r.left()<s.left()){
			r.moveLeft(s.left());
		}
		if(r.right()>s.right()){
			r.moveRight(s.right());
		}
		setGeometry(r);
	}
}
