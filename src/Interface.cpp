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

Render::Render(QWidget *parent):
	QWidget(parent)
{
}

void Render::paintEvent(QPaintEvent *e)
{
	QPainter painter;
	painter.begin(this);
	vplayer->draw(&painter,rect());
	danmaku->draw(&painter,vplayer->getState()==VPlayer::Play);
	painter.end();
	QWidget::paintEvent(e);
}

Interface::Interface(QWidget *parent):
	QWidget(parent)
{
	setAcceptDrops(true);
	setMouseTracking(true);
	setMinimumSize(520,390);
	setWindowIcon(QIcon(":/Interfacce/icon.png"));
	render =new Render(this);
	vplayer=new VPlayer(this);
	danmaku=new Danmaku(this);
	render->setVplayer(vplayer);
	render->setDanmaku(danmaku);
	menu=new Menu(this);
	info=new Info(this);
	Utils::setCenter(this,QSize(960,540));
	QMovie *movie=new QMovie(":Interface/tv.gif");
	tv=new QLabel(this);
	tv->setMovie(movie);
	tv->setFixedSize(QSize(94,82));
	me=new QLabel(this);
	me->setFixedSize(QSize(200,40));
	me->setPixmap(QPixmap(":Interface/version.png"));
	tv->lower();
	me->lower();
	render->lower();
	tv->setAttribute(Qt::WA_TransparentForMouseEvents);
	me->setAttribute(Qt::WA_TransparentForMouseEvents);
	render->setAttribute(Qt::WA_TransparentForMouseEvents);
	movie->start();
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
		if(pos.y()<-50||pos.y()>width()+50){
			menu->push();
			info->push();
			setFocus();
		}
		if(vplayer->getState()==VPlayer::Play){
			qint64 time=vplayer->getTime();
			info->setTime(time);
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
			Utils::setCenter(this,vplayer->getSize());
		}
		sub->clear();
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
	});
	connect(vplayer,&VPlayer::ended,[this](){
		setPalette(QPalette());
		info->setOpened(false);
		tv->show();
		me->show();
		danmaku->reset();
		info->setDuration(-1);
		if(!isFullScreen()){
			Utils::setCenter(this,QSize(960,540));
		}
		sub->clear();
		sub->setEnabled(false);
	});
	connect(vplayer,&VPlayer::decoded,[this](){if(!power->isActive()){update();}});
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
	connect(quitA,&QAction::triggered,this,&QWidget::close);

	fullA=new QAction(tr("Full Screen"),this);
	fullA->setCheckable(true);
	fullA->setChecked(false);
	fullA->setShortcut(QKeySequence("F"));
	connect(fullA,&QAction::toggled,[this](bool b){
		if(!b){
			showNormal();
		}
		else{
			showFullScreen();
		}
	});
	top=new QMenu(this);
	top->addActions(info->actions());
	top->addAction(fullA);
	top->addActions(menu->actions());
	top->addAction(quitA);
	setContextMenuPolicy(Qt::CustomContextMenu);
	connect(this,&QWidget::customContextMenuRequested,[this](QPoint p){
		top->exec(mapToGlobal(p));
	});
	sub=new QMenu(tr("Subtitle"),top);
	sub->setEnabled(false);
	top->addMenu(sub);
	connect(sub,&QMenu::triggered,[this](QAction *action){
		vplayer->setSubTitle(action->text());
	});

	Search::initDataBase();
}

void Interface::dropEvent(QDropEvent *e)
{
	if(e->mimeData()->hasFormat("text/uri-list")){
		QString drop(e->mimeData()->data("text/uri-list"));
		QStringList list=drop.split('\n');
		for(QString &item:list){
			QString file=QUrl(item).toLocalFile().simplified();
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

void Interface::resizeEvent(QResizeEvent *e)
{
	render->resize(e->size());
	vplayer->setSize(e->size());
	danmaku->setSize(e->size());
	int w=e->size().width(),h=e->size().height();
	tv->move((w-94)/2, (h-82)/2-60);
	me->move((w-200)/2,(h-82)/2+60);
	menu->setGeometry(QRect(menu->isPopped()?0:0-200,0,200,h));
	info->setGeometry(QRect(info->isPopped()?w-200:w,0,200,h));
	QWidget::resizeEvent(e);
}

void Interface::keyPressEvent(QKeyEvent *e)
{
	int key=e->key();
	if(key==Qt::Key_Escape&&isFullScreen()){
		fullA->toggle();
	}
	if(key==Qt::Key_Left){
		if(vplayer->getState()==VPlayer::Play){
			vplayer->setTime(vplayer->getTime()-10000);
		}
	}
	if(key==Qt::Key_Right){
		if(vplayer->getState()==VPlayer::Play){
			vplayer->setTime(vplayer->getTime()+10000);
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
