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
	setWindowIcon(QIcon(":/Picture/icon.png"));
	vplayer=new VPlayer(this);
	danmaku=new Danmaku(this);
	menu=new Menu(this);
	info=new Info(this);
	poster=new Poster(this);
	poster->hide();
	setCenter(Utils::getConfig("/Interface/Size",QString("960,540")),true);
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
		vplayer->setVolume(Utils::getConfig("/Playing/Volume",100));
		if(isFullScreen()){
			vplayer->setSize(size());
		}
		else{
			sca->setEnabled(true);
			setCenter(vplayer->getSize(),false);
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
		danmaku->clearCurrent();
		info->setDuration(-1);
		sub->clear();
		sub->setEnabled(false);
		rat->defaultAction()->trigger();
		rat->setEnabled(false);
		sca->defaultAction()->setChecked(true);
		sca->setEnabled(false);
		if(!isFullScreen()){
			setCenter(Utils::getConfig("/Interface/Size",QString("960,540")),false);
		}
	});
	connect(vplayer,&VPlayer::decoded,[this](){if(!power->isActive()){update();}});
	connect(vplayer,&VPlayer::jumped,danmaku,&Danmaku::jumpToTime);
	connect(danmaku,&Danmaku::loaded,[this](){
		if(Utils::getConfig("/Playing/Delay",false)){
			;
		}
	});
	connect(menu,&Menu::open, vplayer,&VPlayer::setFile);
	connect(menu,&Menu::load, danmaku,&Danmaku::setDm);
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

	confA=new QAction(tr("Config"),this);
	confA->setShortcut(QKeySequence("Ctrl+I"));
	addAction(confA);
	connect(confA,&QAction::triggered,[this](){
		Config config(this);
		config.exec();
		danmaku->generateShield();
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
		QSize c=vplayer->getSize(VPlayer::Destinate);
		if(action->text()==tr("Default")){
			vplayer->setRatio(0);
		}
		else{
			QStringList l=action->text().split(':');
			vplayer->setRatio(l[0].toDouble()/l[1].toDouble());
		}
		QSize n=vplayer->getSize(VPlayer::Scaled);
		setCenter(QSize(c.height()*n.width()/n.height(),c.height()),false);
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
		QStringList l=action->text().split(':');
		QSize s=vplayer->getSize(VPlayer::Scaled)*l[0].toInt()/l[1].toInt();
		setCenter(s,false);
	});

	top->addActions(info->actions());
	top->addAction(fullA);
	top->addActions(menu->actions());
	top->addMenu(sub);
	top->addMenu(sca);
	top->addMenu(rat);
	top->addAction(confA);
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
	if(Utils::getConfig("/Interface/Top",false)){
		setWindowFlags(windowFlags()|Qt::WindowStaysOnTopHint);
	}
	Search::initDataBase();
}

void Interface::setCenter(QSize _s,bool f)
{
	QSize m=minimumSize();
	int mw=m.width(),mh=m.height(),sw=_s.width(),sh=_s.height();
	QRect r;
	r.setSize(QSize(mw>sw?mw:sw,mh>sh?mh:sh));
	QRect s=QApplication::desktop()->screenGeometry(this);
	QRect t=f?s:geometry();
	s.setTop(s.top()+style()->pixelMetric(QStyle::PM_TitleBarHeight));
	if(r.width()>=s.width()||r.height()>=s.height()){
		fullA->toggle();
		Utils::delayExec(0,[this](){vplayer->setSize(size());});
	}
	else{
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

void Interface::setCenter(QString s,bool f)
{
	QStringList l=s.split(',',QString::SkipEmptyParts);
	if(l.size()==2){
		setCenter(QSize(l[0].toInt(),l[1].toInt()),f);
	}
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
	int jmp=Utils::getConfig("Playing/Interval",10)*1000;
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
		if(y>height()-40&&danmaku->rowCount()>0){
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
