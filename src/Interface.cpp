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
#include "APlayer.h"
#include "Config.h"
#include "Danmaku.h"
#include "Info.h"
#include "Load.h"
#include "Local.h"
#include "Menu.h"
#include "Next.h"
#include "Post.h"
#include "Render.h"
#include "Shield.h"
#include <functional>

Interface::Interface(QWidget *parent):
	QMdiSubWindow(parent)
{
	setAcceptDrops(true);
	setMinimumSize(480,360);
	setObjectName("Interface");
	setWindowIcon(QIcon(":/Picture/icon.png"));
	setCenter(QSize(),true);
	Local::objects["Interface"]=this;

	aplayer=APlayer::instance();
	danmaku=Danmaku::instance();
	Local::objects["Danmaku"]=danmaku;
	Local::objects["APlayer"]=aplayer;

	render=Render::instance(this);
	Local::objects["Render"]=render;

	menu=new Menu(this);
	info=new Info(this);
	post=Post::instance();
	next=Next::instance();
	load=Load::instance();
	Local::objects["Info"]=info;
	Local::objects["Menu"]=menu;
	Local::objects["Next"]=next;
	Local::objects["Post"]=post;
	Local::objects["Load"]=load;

	timer=new QTimer(this);
	delay=new QTimer(this);
	timer->start(1000);
	connect(timer,&QTimer::timeout,[this](){
		QPoint cur=mapFromGlobal(QCursor::pos());
		int x=cur.x(),y=cur.y(),w=width(),h=height();
		if(isActiveWindow()){
			if(y<-60||y>h+60||x<-100||x>w+100){
				menu->push();
				info->push();
			}
		}
	});
	connect(delay,&QTimer::timeout,[this](){
		if(aplayer->getState()==APlayer::Play&&!menu->isVisible()&&!info->isVisible()){
			setCursor(QCursor(Qt::BlankCursor));
		}
		if(!sliding){
			showprg=false;
			render->setDisplayTime(0);
		}
	});
	connect(danmaku,SIGNAL(layoutChanged()),render,SLOT(draw()));
	connect(aplayer,&APlayer::begin,[this](){
		if(!isFullScreen()&&geo.isEmpty()){
			geo=saveGeometry();
			sca->setEnabled(true);
			setCenter(render->getPreferredSize(),false);
		}
		rat->setEnabled(true);
		render->setDisplayTime(0);
		setWindowFilePath(aplayer->getMedia());
	});
	connect(aplayer,&APlayer::reach,[this](){
		danmaku->resetTime();
		danmaku->clearCurrent();
		rat->defaultAction()->setChecked(true);
		rat->setEnabled(false);
		sca->defaultAction()->setChecked(true);
		sca->setEnabled(false);
		render->setVideoAspectRatio(0);
		if(!geo.isEmpty()&&next->getNext().isEmpty()){
			if(isFullScreen()){
				fullA->toggle();
			}
			restoreGeometry(geo);
			geo.clear();
		}
		render->setDisplayTime(0);
		setWindowFilePath(QString());
	});

	showprg=sliding=false;
	connect(aplayer,&APlayer::timeChanged,[this](qint64 t){
		if(!sliding&&aplayer->getState()!=APlayer::Stop){
			render->setDisplayTime(showprg?t/(double)aplayer->getDuration():-1);
		}
	});

	addActions(menu->actions());
	addActions(info->actions());

	quitA=new QAction(tr("Quit"),this);
	quitA->setObjectName("Quit");
	quitA->setShortcut(Config::getValue("/Shortcut/Quit",QString("Ctrl+Q")));
	addAction(quitA);
	connect(quitA,&QAction::triggered,this,&Interface::close);

	fullA=new QAction(tr("Full Screen"),this);
	fullA->setObjectName("Full");
	fullA->setCheckable(true);
	fullA->setChecked(false);
	fullA->setShortcut(Config::getValue("/Shortcut/Full",QString("F")));
	addAction(fullA);
	connect(fullA,&QAction::toggled,[this](bool b){
		if(!b){
			showNormal();
			if(aplayer->getState()!=APlayer::Stop){
				sca->setEnabled(true);
			}
		}
		else{
			showFullScreen();
			sca->setEnabled(false);
		}
	});

	confA=new QAction(tr("Config"),this);
	confA->setObjectName("Conf");
	confA->setShortcut(Config::getValue("/Shortcut/Conf",QString("Ctrl+I")));
	addAction(confA);
	connect(confA,&QAction::triggered,[](){
		Config::exec(Local::mainWidget());
	});

	toggA=new QAction(tr("Block All"),this);
	toggA->setObjectName("Togg");
	toggA->setCheckable(true);
	toggA->setChecked(Shield::shieldG[7]);
	toggA->setShortcut(Config::getValue("/Shortcut/Togg",QString("Ctrl+T")));
	addAction(toggA);
	connect(toggA,&QAction::triggered,[this](bool b){
		Shield::shieldG[7]=b;
		danmaku->parse(0x2);
	});
	connect(danmaku,&Danmaku::layoutChanged,[this](){
		toggA->setChecked(Shield::shieldG[7]);
	});

	postA=new QAction(tr("Post Danmaku"),this);
	postA->setObjectName("Post");
	postA->setEnabled(false);
	postA->setShortcut(Config::getValue("/Shortcut/Post",QString("Ctrl+P")));
	addAction(postA);
	connect(postA,&QAction::triggered,[this](){
		menu->push();
		info->push();
		post->show();
	});
	connect(danmaku,&Danmaku::modelReset,[this](){
		postA->setEnabled(post->isValid());
	});

	QAction *fwdA=new QAction(tr("Forward"),this);
	fwdA->setObjectName("Fowd");
	fwdA->setShortcut(Config::getValue("/Shortcut/Fowd",QString("Right")));
	connect(fwdA,&QAction::triggered,[this](){
		aplayer->setTime(aplayer->getTime()+Config::getValue("/Interface/Interval",10)*1000);
	});
	QAction *bwdA=new QAction(tr("Backward"),this);
	bwdA->setObjectName("Bkwd");
	bwdA->setShortcut(Config::getValue("/Shortcut/Bkwd",QString("Left")));
	connect(bwdA,&QAction::triggered,[this](){
		aplayer->setTime(aplayer->getTime()-Config::getValue("/Interface/Interval",10)*1000);
	});
	addAction(fwdA);
	addAction(bwdA);

	QAction *delA=new QAction(tr("Delay"),this);
	delA->setObjectName("Dely");
	delA->setShortcut(Config::getValue("/Shortcut/Dely",QString("Ctrl+Right")));
	connect(delA,&QAction::triggered,std::bind(&Danmaku::delayAll,Danmaku::instance(),+1000));
	QAction *ahdA=new QAction(tr("Ahead"),this);
	ahdA->setObjectName("Ahed");
	ahdA->setShortcut(Config::getValue("/Shortcut/Ahed",QString("Ctrl+Left")));
	connect(ahdA,&QAction::triggered,std::bind(&Danmaku::delayAll,Danmaku::instance(),-1000));
	addAction(delA);
	addAction(ahdA);

	QAction *escA=new QAction(this);
	escA->setShortcut(Qt::Key_Escape);
	connect(escA,&QAction::triggered,[this](){
		if(isFullScreen()){
			fullA->toggle();
		}
	});
	addAction(escA);

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
			render->setVideoAspectRatio(0);
		}
		else{
			QStringList l=action->text().split(':');
			render->setVideoAspectRatio(l[0].toDouble()/l[1].toDouble());
		}
		if(aplayer->getState()==APlayer::Pause){
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
		if(render->getPreferredSize().isValid()){
			QStringList l=action->text().split(':');
			QSize s=render->getPreferredSize()*l[0].toInt()/l[1].toInt();
			setCenter(s,false);
		}
	});

	if(Config::getValue("/Interface/Frameless",false)){
		setWindowFlags(Qt::CustomizeWindowHint);
	}
	if(Config::getValue("/Interface/Top",false)){
		setWindowFlags(windowFlags()|Qt::WindowStaysOnTopHint);
	}

	checkForUpdate();
}

void Interface::tryLocal(QString p)
{
	QFileInfo info(p);
	if(!info.exists()){
		return;
	}
	QString suffix=info.suffix().toLower();
	if(Utils::getSuffix(Utils::Video|Utils::Audio).contains(suffix)){
		aplayer->setMedia(p);
	}
	else if(Utils::getSuffix(Utils::Danmaku).contains(suffix)){
		load->loadDanmaku(p);
	}
	else if(Utils::getSuffix(Utils::Subtitle).contains(suffix)&&aplayer->getState()!=APlayer::Stop){
		aplayer->addSubtitle(p);
	}
}

void Interface::tryLocal(QStringList p)
{
	for(const QString &i:p){
		tryLocal(i);
	}
}

void Interface::dropEvent(QDropEvent *e)
{
	if(e->mimeData()->hasFormat("text/uri-list")){
		for(const QString &item:QString(e->mimeData()->data("text/uri-list")).split('\n')){
			tryLocal(QUrl(item).toLocalFile().trimmed());
		}
	}
}

void Interface::closeEvent(QCloseEvent *e)
{
	if(aplayer->getState()==APlayer::Stop&&!isFullScreen()&&!isMaximized()){
		QString size=Config::getValue("/Interface/Size",QString("960,540"));
		Config::setValue("/Interface/Size",size.endsWith(' ')?size.trimmed():QString("%1,%2").arg(width()).arg(height()));
	}
	danmaku->release();
	if(!update.isNull()){
		update->abort();
	}
	QWidget::closeEvent(e);
}

void Interface::dragEnterEvent(QDragEnterEvent *e)
{
	if(e->mimeData()->hasFormat("text/uri-list")){
		QStringList accept=Utils::getSuffix(Utils::Video|Utils::Audio|Utils::Subtitle|Utils::Danmaku);
		for(const QString &item:QString(e->mimeData()->data("text/uri-list")).split('\n')){
			QString suffix=QFileInfo(QUrl(item).toLocalFile().trimmed()).suffix().toLower();
			if(std::binary_search(accept.begin(),accept.end(),suffix)){
				e->acceptProposedAction();
				break;
			}
		}
	}
	QWidget::dragEnterEvent(e);
}

void Interface::resizeEvent(QResizeEvent *e)
{
	render->getWidget()->resize(e->size());
	int w=e->size().width(),h=e->size().height();
	menu->terminate();
	info->terminate();
	menu->setGeometry(menu->isShown()?0:0-200,0,200,h);
	info->setGeometry(info->isShown()?w-200:w,0,200,h);
	QWidget::resizeEvent(e);
}

void Interface::mouseMoveEvent(QMouseEvent *e)
{
	QPoint cur=e->pos();
	int x=cur.x(),y=cur.y(),w=width(),h=height();
	if(isActiveWindow()){
		if(x>250){
			menu->push();
		}
		if(x<w-250){
			info->push();
		}
		if(y<=h-50){
			if(x>=0&&x<50){
				menu->pop();
			}
			if(x>w-50&&x<=w){
				info->pop();
			}
		}
	}
	if(!showprg&&aplayer->getState()!=APlayer::Stop){
		render->setDisplayTime(aplayer->getTime()/(double)aplayer->getDuration());
	}
	showprg=true;
	if(cursor().shape()==Qt::BlankCursor){
		unsetCursor();
	}
	delay->start(4000);
	if(sliding){
		render->setDisplayTime(x/(double)w);
	}
	else if(!sta.isNull()&&(windowFlags()&Qt::CustomizeWindowHint)!=0&&!isFullScreen()){
		move(wgd+e->globalPos()-sta);
	}
	QWidget::mouseMoveEvent(e);
}

void Interface::mousePressEvent(QMouseEvent *e)
{
	if(e->button()==Qt::LeftButton){
		if(sta.isNull()){
			sta=e->globalPos();
			wgd=pos();
		}
		int s=height()-e->y();
		if(s<=25&&s>=0){
			sliding=true;
			render->setDisplayTime(e->x()/(double)width());
		}
	}
	QWidget::mousePressEvent(e);
}

void Interface::mouseReleaseEvent(QMouseEvent *e)
{
	if(!menu->geometry().contains(e->pos())&&!info->geometry().contains(e->pos())){
		menu->push(true);
		info->push(true);
		post->hide();
	}
	sta=wgd=QPoint();
	if(sliding&&e->button()==Qt::LeftButton){
		sliding=false;
		if(aplayer->getState()!=APlayer::Stop){
			aplayer->setTime(e->x()*aplayer->getDuration()/(width()-1));
		}
	}
	if(e->button()==Qt::RightButton){
		showContextMenu(e->pos());
	}
	QWidget::mouseReleaseEvent(e);
}

void Interface::mouseDoubleClickEvent(QMouseEvent *e)
{
	if(!menu->isShown()&&!info->isShown()){
		fullA->toggle();
	}
	QWidget::mouseDoubleClickEvent(e);
}

void Interface::checkForUpdate()
{
	QNetworkAccessManager *manager=new QNetworkAccessManager(this);
	QNetworkRequest request(QUrl("https://raw.githubusercontent.com/AncientLysine/BiliLocal/master/res/INFO"));
	update=manager->get(request);
	connect(manager,&QNetworkAccessManager::finished,[=](QNetworkReply *info){
		QUrl redirect=info->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
		if(redirect.isValid()){
			update=manager->get(QNetworkRequest(redirect));
			return;
		}
		if(info->error()==QNetworkReply::NoError){
			QFile local(":/Text/DATA");
			local.open(QIODevice::ReadOnly);
			QJsonObject l=QJsonDocument::fromJson(local.readAll()).object();
			QJsonObject r=QJsonDocument::fromJson(info->readAll()).object();
			if(r.contains("Version")&&l["Version"].toString()<r["Version"].toString()){
				QMessageBox::StandardButton button;
				QString information=r["String"].toString();
				button=QMessageBox::information(this,
												tr("Update"),
												information,
												QMessageBox::Ok|QMessageBox::Cancel);
				if(button==QMessageBox::Ok){
					QDesktopServices::openUrl(r["Url"].toString());
				}
			}
		}
		manager->deleteLater();
	});
}

void Interface::setCenter(QSize _s,bool f)
{
	if(!_s.isValid()){
		QStringList l=Config::getValue("/Interface/Size",QString("960,540")).split(',',QString::SkipEmptyParts);
		if(l.size()>=2){
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

void Interface::showContextMenu(QPoint p)
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
				QString sender=cur->sender;
				if(!sender.isEmpty()){
					Shield::shieldS.insert(sender);
				}
				Danmaku::instance()->parse(0x2);
			});
			top.addSeparator();
		}
		top.addActions(info->actions());
		top.addAction(fullA);
		top.addAction(toggA);
		top.addActions(menu->actions());
		top.addAction(postA);
		QMenu sub(tr("Subtitle"),this);
		QMenu vid(tr("Video Track"),this);
		QMenu aud(tr("Audio Track"),this);
		connect(sub.addAction(tr("From File")),&QAction::triggered,[this](){
			QFileInfo info(aplayer->getMedia());
			QString file=QFileDialog::getOpenFileName(this,
													  tr("Open File"),
													  info.absolutePath(),
													  tr("Subtitle files (%1);;All files (*.*)").arg(Utils::getSuffix(Utils::Subtitle,"*.%1").join(' ')));
			if(!file.isEmpty()){
				aplayer->addSubtitle(file);
			}
		});
		sub.addSeparator();
		sub.addActions(aplayer->getTracks(Utils::Subtitle));
		sub.setEnabled(!sub.isEmpty());
		vid.addActions(aplayer->getTracks(Utils::Video));
		vid.setEnabled(!vid.isEmpty());
		aud.addActions(aplayer->getTracks(Utils::Audio));
		aud.setEnabled(!aud.isEmpty());
		QMenu tra(tr("Track"),this);
		tra.addMenu(&sub);
		tra.addMenu(&vid);
		tra.addMenu(&aud);
		tra.setEnabled(aplayer->getState()!=APlayer::Stop);
		top.addMenu(&tra);
		top.addMenu(sca);
		top.addMenu(rat);
		top.addAction(confA);
		top.addAction(quitA);
		top.exec(mapToGlobal(p));
	}
}
