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

Interface::Interface(QWidget *parent):
	QWidget(parent)
{
	setAcceptDrops(true);
	setMouseTracking(true);
	setMinimumSize(520,390);
	setWindowIcon(QIcon(":/Interfacce/icon.png"));
	vplayer=new VPlayer(this);
	danmaku=new Danmaku(this);
	menu=new Menu(this);
	info=new Info(this);
	info->setModel(danmaku);
	poster=new Poster(this);
	poster->setDanmaku(danmaku);
	poster->setVplayer(vplayer);
	poster->hide();
	setCenter(QSize(960,540),true);
	tv=new QLabel(this);
	tv->setMovie(new QMovie(":Interface/tv.gif"));
	tv->setFixedSize(QSize(94,82));
	me=new QLabel(this);
	me->setPixmap(QPixmap(":Interface/version.png"));
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
		QPoint pos=this->mapFromGlobal(QCursor::pos());
		if(pos.x()<-100){
			menu->push();
			setFocus();
		}
		if(pos.x()>width()+100){
			info->push();
			setFocus();
		}
		if(pos.y()<-50||pos.y()>height()+50){
			menu->push();
			info->push();
			poster->fadeOut();
			setFocus();
		}
		if(vplayer->getState()==VPlayer::Play){
			qint64 time=vplayer->getTime();
			info->setTime(time);
			danmaku->setTime(time);
		}
	});
	connect(power,&QTimer::timeout,[this](){update();});
	connect(delay,&QTimer::timeout,[this](){
		if(vplayer->getState()==VPlayer::Play){
			setCursor(QCursor(Qt::BlankCursor));
		}
	});
	connect(vplayer,&VPlayer::opened,[this](){
		Utils::setBack(this,Qt::black);
		info->setDuration(vplayer->getDuration());
		info->setOpened(true);
		tv->hide();
		me->hide();
		vplayer->setVolume(info->getVolume());
		if(isFullScreen()){
			vplayer->setSize(size());
		}
		else{
			sca->setEnabled(true);
			QRect v(QPoint(0,0),vplayer->getSize());
			QRect s(QDesktopWidget().availableGeometry(this));
			s.setTop(s.top()+style()->pixelMetric(QStyle::PM_TitleBarHeight));
			if(v.width()>=s.width()||v.height()>=s.height()){
				fullA->toggle();
				Utils::delayExec(0,[this](){vplayer->setSize(size());});
			}
			else{
				v.moveCenter(geometry().center());
				if(v.top()<s.top()){
					v.moveTop(s.top());
				}
				if(v.bottom()>s.bottom()){
					v.moveBottom(s.bottom());
				}
				if(v.left()<s.left()){
					v.moveLeft(s.left());
				}
				if(v.right()>s.right()){
					v.moveRight(s.right());
				}
				setGeometry(v);
			}
		}
		sub->clear();
		if(!vplayer->getSubtitles().isEmpty()){
			sub->setEnabled(true);
			QActionGroup *group=new QActionGroup(sub);
			group->setExclusive(true);
			QString current=vplayer->getSubtitle();
			for(QString title:vplayer->getSubtitles()){
				QAction *action=group->addAction(title);
				action->setCheckable(true);
				action->setChecked(current==title);
			}
			sub->addActions(group->actions());
		}
		rat->setEnabled(true);
	});
	connect(vplayer,&VPlayer::ended,[this](){
		setPalette(QPalette());
		info->setOpened(false);
		tv->show();
		me->show();
		danmaku->reset();
		info->setDuration(-1);
		sub->clear();
		sub->setEnabled(false);
		rat->defaultAction()->trigger();
		rat->setEnabled(false);
		sca->defaultAction()->setChecked(true);
		sca->setEnabled(false);
		if(!isFullScreen()){
			setCenter(QSize(960,540),false);
		}
	});
	connect(vplayer,&VPlayer::decoded,[this](){update();});
	connect(vplayer,&VPlayer::paused,danmaku,&Danmaku::setLast);
	connect(vplayer,&VPlayer::jumped,danmaku,&Danmaku::jumpToTime);
	connect(danmaku,&Danmaku::loaded,[this](){menu->setDelay(vplayer->getTime());});
	connect(menu,&Menu::open, vplayer,&VPlayer::setFile);
	connect(menu,&Menu::load, danmaku,&Danmaku::setDm);
	connect(menu,&Menu::dfont,danmaku,&Danmaku::setFont);
	connect(menu,&Menu::alpha,danmaku,&Danmaku::setAlpha);
	connect(menu,&Menu::power,[this](qint16 _power){
		if(_power>=0)
			power->start(_power);
		else
			power->stop();
	});
	connect(menu,&Menu::delay,[this](qint64 _delay){
		danmaku->setDelay(_delay);
		danmaku->jumpToTime(vplayer->getTime());
	});
	connect(menu,&Menu::protect,danmaku,&Danmaku::setProtect);
	connect(info,&Info::time,vplayer,&VPlayer::setTime);
	connect(info,&Info::play,vplayer,&VPlayer::play);
	connect(info,&Info::stop,vplayer,&VPlayer::stop);
	connect(info,&Info::volume,vplayer,&VPlayer::setVolume);
	setFocus();

	quitA=new QAction(tr("Quit"),this);
	quitA->setShortcut(QKeySequence("Ctrl+Q"));
	addAction(quitA);
	connect(quitA,&QAction::triggered,this,&QWidget::close);

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


	top=new QMenu(this);
	sub=new QMenu(tr("Subtitle"),top);
	sub->setEnabled(false);
	connect(sub,&QMenu::triggered,[this](QAction *action){
		vplayer->setSubTitle(action->text());
	});

	QActionGroup *g;
	rat=new QMenu(tr("Ratio"),top);
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
		vplayer->setSize(size());
	});

	sca=new QMenu(tr("Scale"),top);
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
		QSize s;
		if(action->text()=="1:1"){
			s=vplayer->getSize();
		}
		else{
			QStringList l=action->text().split(':');
			s=vplayer->getSize()*l[0].toInt()/l[1].toInt();
		}
		setCenter(s,false);
	});

	top->addActions(info->actions());
	top->addAction(fullA);
	top->addActions(menu->actions());
	top->addMenu(sub);
	top->addMenu(sca);
	top->addMenu(rat);
	top->addAction(quitA);
	setContextMenuPolicy(Qt::CustomContextMenu);
	connect(this,&QWidget::customContextMenuRequested,[this](QPoint p){
		bool flag=true;
		flag=flag&&!(menu->isPopped()&&menu->geometry().contains(p));
		flag=flag&&!(info->isPopped()&&info->geometry().contains(p));
		if(flag){
			top->exec(mapToGlobal(p));
		}
	});


	Search::initDataBase();
}

void Interface::setCenter(QSize s,bool f)
{
	QRect r;
	r.setSize(s);
	QRect t=f?QApplication::desktop()->screenGeometry():geometry();
	r.moveCenter(t.center());
	setGeometry(r);
}

void Interface::dropEvent(QDropEvent *e)
{
	if(e->mimeData()->hasFormat("text/uri-list")){
		QString drop(e->mimeData()->data("text/uri-list"));
		QStringList list=drop.split('\n');
		for(QString &item:list){
			QString file=QUrl(item).toLocalFile().trimmed();
			if(QFile::exists(file)){
				if(file.endsWith(".xml")||file.endsWith(".json")){
					menu->setDm(file);
				}
				else{
					menu->setFile(file);
				}
			}
		}
	}
}

void Interface::paintEvent(QPaintEvent *e)
{
	QPainter painter;
	painter.begin(this);
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
	tv->move((w-tv->width())/2,(h-tv->height())/2-40);
	me->move((w-me->width())/2,(h-me->height())/2+40);
	menu->setGeometry(menu->isPopped()?0:0-200,0,200,h);
	info->setGeometry(info->isPopped()?w-200:w,0,200,h);
	poster->setGeometry((w-(w>940?540:w-400))/2,h-40,w>940?540:w-400,25);
	QWidget::resizeEvent(e);
}

void Interface::keyPressEvent(QKeyEvent *e)
{
	int key=e->key();
	int jmp=Utils::getSetting<int>("Interval",10000);
	if(key==Qt::Key_Escape&&isFullScreen()){
		fullA->toggle();
	}
	if(key==Qt::Key_Left){
		if(vplayer->getState()==VPlayer::Play){
			vplayer->setTime(vplayer->getTime()-jmp);
		}
	}
	if(key==Qt::Key_Right){
		if(vplayer->getState()==VPlayer::Play){
			vplayer->setTime(vplayer->getTime()+jmp);
		}
	}
	QWidget::keyPressEvent(e);
}

void Interface::mouseMoveEvent(QMouseEvent *e)
{
	int x=e->pos().x(),y=e->pos().y();
	if(x<50){
		menu->pop();
	}
	if(x>250){
		menu->push();
		setFocus();
	}
	if(x>width()-50){
		info->pop();
	}
	if(x<width()-250){
		info->push();
		setFocus();
	}
	if(x>220&&x<width()-220&&y>50&&y<height()-50){
		if(cursor().shape()==Qt::BlankCursor){
			unsetCursor();
		}
		delay->start(2000);
	}
	else{
		delay->stop();
	}
	if(x>200&&x<width()-200){
		if(y>height()-40&&!danmaku->getCid().isEmpty()){
			poster->fadeIn();
		}
		if(y<height()-60){
			poster->fadeOut();
		}
	}
	else{
		poster->fadeOut();
	}
	QWidget::mouseMoveEvent(e);
}

void Interface::dragEnterEvent(QDragEnterEvent *e)
{
	if(e->mimeData()->hasFormat("text/uri-list")){
		e->acceptProposedAction();
	}
}

void Interface::mouseDoubleClickEvent(QMouseEvent *e)
{
	if(!menu->isPopped()&&!info->isPopped()){
		fullA->toggle();
	}
	QWidget::mouseDoubleClickEvent(e);
}
