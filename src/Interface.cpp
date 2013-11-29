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
#include "Panel.h"
#include "Shield.h"
#include "Config.h"
#include "Printer.h"
#include "VPlayer.h"
#include "Danmaku.h"

Interface::Interface(QWidget *parent):
	QWidget(parent)
{
	setAcceptDrops(true);
	setMinimumSize(550,390);
	setWindowIcon(QIcon(":/Picture/icon.png"));
	background=QPixmap(Utils::getConfig("/Interface/Background",QString()));
	vplayer=new VPlayer(this);
	danmaku=new Danmaku(this);
	printer=new Printer(this);
	menu=new Menu(this);
	info=new Info(this);
	panel=new Panel(this);
	setCenter(QSize(),true);
	tv=new QLabel(this);
	tv->setMovie(new QMovie(":/Picture/tv.gif"));
	tv->setFixedSize(QSize(94,82));
	me=new QLabel(this);
	me->setPixmap(QPixmap(":/Picture/version.png"));
	me->setFixedSize(me->pixmap()->size());
	tv->lower();
	me->lower();
	tv->movie()->start();
	tv->setAttribute(Qt::WA_TransparentForMouseEvents);
	me->setAttribute(Qt::WA_TransparentForMouseEvents);
	timer=new QTimer(this);
	power=new QTimer(this);
	delay=new QTimer(this);
	timer->start(200);
	power->setTimerType(Qt::PreciseTimer);
	connect(timer,&QTimer::timeout,[this](){
		QPoint cur=mapFromGlobal(QCursor::pos());
		int x=cur.x(),y=cur.y(),w=width(),h=height();
		if(isActiveWindow()){
			if(y<-50||y>h+50){
				menu->push();
				info->push();
				panel->fadeOut();
				setFocus();
			}
			else{
				if(x<-100){
					menu->push();
					setFocus();
				}
				if(x>=0&&x<50){
					menu->pop();
				}
				if(x>250){
					menu->push();
					if(!info->isPopped()&&!panel->isShown()){
						setFocus();
					}
				}
				if(x<w-250){
					info->push();
					if(!menu->isPopped()&&!panel->isShown()){
						setFocus();
					}
				}
				if(x>w-50&&x<=w){
					info->pop();
				}
				if(x>w+100){
					info->push();
					setFocus();
				}
				if(x>200&&x<w-200){
					if(y>h-65){
						panel->fadeIn();
					}
					if(y<h-85){
						panel->fadeOut();
					}
				}
				else{
					panel->fadeOut();
				}
			}
		}
		if(cur!=pre){
			pre=cur;
			if(!menu->isPopped()&&!info->isPopped()){
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
			panel->setTime(time);
			danmaku->setTime(time);
		}
	});
	connect(power,&QTimer::timeout,[this](){
		if(vplayer->getState()==VPlayer::Play){
			update();
		}
	});
	connect(delay,&QTimer::timeout,[this](){
		if(vplayer->getState()==VPlayer::Play){
			setCursor(QCursor(Qt::BlankCursor));
		}
	});
	connect(danmaku,SIGNAL(currentCleared()),this,SLOT(update()));
	connect(vplayer,&VPlayer::begin,[this](){
		tv->hide();
		me->hide();
		if(isFullScreen()){
			vplayer->setSize(size());
		}
		else{
			sca->setEnabled(true);
			setCenter(vplayer->getSize(),false);
		}
		tra->setEnabled(true);
		int cur;
		QMap<int,QString> map;
		auto set=[&](QMenu *m){
			m->clear();
			if(!map.isEmpty()){
				m->setEnabled(true);
				QActionGroup *group=new QActionGroup(m);
				group->setExclusive(true);
				for(auto iter=map.begin();iter!=map.end();++iter){
					QAction *action=group->addAction(iter.value());
					action->setCheckable(true);
					action->setData(iter.key());
					action->setChecked(cur==iter.key());
				}
				m->addActions(group->actions());
			}
		};
		cur=vplayer->getSubtitle();
		map=vplayer->getSubtitles();
		set(sub);
		cur=vplayer->getVideoTrack();
		map=vplayer->getVideoTracks();
		set(vid);
		cur=vplayer->getAudioTrack();
		map=vplayer->getAudioTracks();
		set(aud);
		rat->setEnabled(true);
	});
	connect(vplayer,&VPlayer::reach,[this](){
		tv->show();
		me->show();
		danmaku->resetTime();
		danmaku->clearCurrent();
		sub->clear();
		sub->setEnabled(false);
		vid->clear();
		vid->setEnabled(false);
		aud->clear();
		aud->setEnabled(false);
		tra->setEnabled(false);
		rat->defaultAction()->setChecked(true);
		rat->setEnabled(false);
		sca->defaultAction()->setChecked(true);
		sca->setEnabled(false);
		vplayer->setRatio(0);
		if(!isFullScreen()){
			setCenter(QSize(),false);
		}
		update();
	});
	connect(vplayer,&VPlayer::decode,[this](){
		if(!power->isActive()){
			update();
		}
	});
	connect(vplayer,&VPlayer::jumped,danmaku,&Danmaku::jumpToTime);
	connect(menu,&Menu::open,info,&Info::setFilePath);
	connect(menu,&Menu::open,vplayer,&VPlayer::setFile);
	connect(menu,&Menu::power,[this](qint16 _power){
		if(_power>=0)
			power->start(_power);
		else
			power->stop();
	});
	connect(info,&Info::time,vplayer,&VPlayer::setTime);
	connect(info,&Info::play,vplayer,&VPlayer::play);
	connect(info,&Info::stop,vplayer,&VPlayer::stop);
	connect(info,&Info::volume,vplayer,&VPlayer::setVolume);
	connect(panel,&Panel::time,vplayer,&VPlayer::setTime);

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

	toggA=new QAction(tr("Block All"),this);
	toggA->setCheckable(true);
	toggA->setChecked(Shield::block[5]);
	toggA->setShortcut(QKeySequence("Ctrl+T"));
	addAction(toggA);
	connect(toggA,&QAction::toggled,[this](bool b){
		Shield::block[5]=b;
		danmaku->parse(0x2);
		danmaku->clearCurrent();
	});

	confA=new QAction(tr("Config"),this);
	confA->setShortcut(QKeySequence("Ctrl+I"));
	addAction(confA);
	connect(confA,&QAction::triggered,[this](){
		Config config(this);
		config.exec();
		danmaku->parse(0x2);
	});

	sub=new QMenu(tr("Subtitle"),this);
	sub->setEnabled(false);
	connect(sub,&QMenu::triggered,[this](QAction *action){
		vplayer->setSubTitle(action->data().toInt());
	});
	vid=new QMenu(tr("Video Track"),this);
	vid->setEnabled(false);
	connect(vid,&QMenu::triggered,[this](QAction *action){
		vplayer->setVideoTrack(action->data().toInt());
	});
	aud=new QMenu(tr("Audio Track"),this);
	aud->setEnabled(false);
	connect(aud,&QMenu::triggered,[this](QAction *action){
		vplayer->setAudioTrack(action->data().toInt());
	});
	tra=new QMenu(tr("Track"),this);
	tra->addMenu(sub);
	tra->addMenu(vid);
	tra->addMenu(aud);
	tra->setEnabled(false);

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
		if(vplayer->getSize().isValid()){
			QSize c=vplayer->getSize(VPlayer::Destinate);
			if(action->text()==tr("Default")){
				vplayer->setRatio(0);
			}
			else{
				QStringList l=action->text().split(':');
				vplayer->setRatio(l[0].toDouble()/l[1].toDouble());
			}
			QSize n=vplayer->getSize(VPlayer::Scaled);
			if(!isFullScreen()){
				setCenter(QSize(c.height()*n.width()/n.height(),c.height()),false);
			}
			else{
				vplayer->setSize(size());
			}
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
			QSize s=vplayer->getSize(VPlayer::Scaled)*l[0].toInt()/l[1].toInt();
			setCenter(s,false);
		}
	});

	setContextMenuPolicy(Qt::CustomContextMenu);
	connect(this,&QWidget::customContextMenuRequested,[this](QPoint p){
		bool flag=true;
		flag=flag&&!(menu->isPopped()&&menu->geometry().contains(p));
		flag=flag&&!(info->isPopped()&&info->geometry().contains(p));
		if(flag){
			QMenu top(this);
			const Comment *cur=danmaku->commentAt(p);
			if(cur){
				QAction *text=new QAction(&top);
				top.addAction(text);
				text->setText(top.fontMetrics().elidedText(cur->string,Qt::ElideRight,top.actionGeometry(text).width()));
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
			top.addMenu(tra);
			top.addMenu(sca);
			top.addMenu(rat);
			top.addAction(confA);
			top.addAction(quitA);
			top.exec(mapToGlobal(p));
		}
	});
	if(Utils::getConfig("/Interface/Frameless",false)){
		setWindowFlags(Qt::CustomizeWindowHint);
	}
	if(Utils::getConfig("/Interface/Top",false)){
		setWindowFlags(windowFlags()|Qt::WindowStaysOnTopHint);
	}
	if(QApplication::arguments().count()>=2){
		Utils::delayExec(this,0,[this](){
			for(const QString &file:QApplication::arguments().mid(1)){
				menu->openLocal(file);
			}
		});
	}
	setFocus();
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

void Interface::dropEvent(QDropEvent *e)
{
	if(e->mimeData()->hasFormat("text/uri-list")){
		for(const QString &item:QString(e->mimeData()->data("text/uri-list")).split('\n')){
			menu->openLocal(QUrl(item).toLocalFile().trimmed());
		}
	}
}

void Interface::paintEvent(QPaintEvent *e)
{
	QPainter painter;
	painter.begin(this);
	painter.setRenderHint(QPainter::SmoothPixmapTransform);
	if(vplayer->getState()==VPlayer::Stop){
		QRect to=rect();
		to.setSize(background.size().scaled(to.size(),Qt::KeepAspectRatioByExpanding));
		to.moveCenter(rect().center());
		painter.drawPixmap(to,background);
	}
	vplayer->draw(&painter,rect());
	danmaku->draw(&painter,vplayer->getState()==VPlayer::Play);
	painter.end();
	QWidget::paintEvent(e);
}

void Interface::resizeEvent(QResizeEvent *e)
{
	vplayer->setSize(e->size());
	danmaku->setSize(e->size());
	int w=e->size().width(),h=e->size().height();
	QMetaObject::invokeMethod(this,"saveSize",Qt::QueuedConnection);
	tv->move((w-tv->width())/2,(h-tv->height())/2-40);
	me->move((w-me->width())/2,(h-me->height())/2+40);
	menu->terminate();
	info->terminate();
	menu->setGeometry(menu->isPopped()?0:0-200,0,200,h);
	info->setGeometry(info->isPopped()?w-200:w,0,200,h);
	panel->setGeometry(qMax(400,w-800)/2,h-65,qMin(800,w-400),50);
	printer->setGeometry(10,10,qBound<int>(300,w/2.5,500),150);
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
		menu->push(true);
		info->push(true);
		setFocus();
	}
	sta=wgd=QPoint();
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
	if(!menu->isPopped()&&!info->isPopped()&&!panel->isShown()){
		fullA->toggle();
	}
	QWidget::mouseDoubleClickEvent(e);
}

void Interface::saveSize()
{
	if(vplayer->getState()==VPlayer::Stop&&!isFullScreen()&&!isMaximized()){
		Utils::setConfig("/Interface/Size",QString("%1,%2").arg(width()).arg(height()));
	}
}
