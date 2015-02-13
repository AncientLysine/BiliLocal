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
#include "Editor.h"
#include "Info.h"
#include "Jump.h"
#include "List.h"
#include "Load.h"
#include "Local.h"
#include "Menu.h"
#include "Post.h"
#include "Render.h"
#include "Shield.h"
#include <functional>

class Message:public QMessageBox
{
public:
	explicit Message(QWidget *parent):
		QMessageBox(parent)
	{
		setIcon(Warning);
	}

	void warning(QString title,QString text)
	{
		if (p){
			p->hide();
		}
		setWindowTitle(title);
		setText(text);
		show();
	}

	void setProgress(double progress)
	{
		QWidget *active=lApp->activeWindow();
		if(!p||(p!=active&&p->parent()!=active&&active)){
			if (p){
				delete p;
			}
			p=new QProgressDialog(active);
			p->setMaximum(1000);
			p->setWindowTitle(Interface::tr("Loading"));
			p->setFixedSize(p->sizeHint());
			p->show();
			connect(p,&QProgressDialog::canceled,Load::instance(),&Load::dequeue);
		}
		p->setValue(1000*progress);
	}

private:
	QPointer<QProgressDialog> p;
};

Interface::Interface(QWidget *parent):
	QWidget(parent)
{
	setObjectName("Interface");
	setMouseTracking(true);
	setAcceptDrops(true);
	setWindowIcon(QIcon(":/Picture/icon.png"));
	setMinimumSize(360*logicalDpiX()/72,270*logicalDpiY()/72);
	Local::objects["Interface"]=this;
	message=new Message(this);
	
	aplayer=APlayer::instance();
	danmaku=Danmaku::instance();
	Local::objects["Danmaku"]=danmaku;
	Local::objects["APlayer"]=aplayer;
	
	render=Render::instance();
	Local::objects["Render"]=render;
	Local::objects["Config"]=Config::instance();
	
	menu=new Menu(this);
	info=new Info(this);
	jump=new Jump(this);
	post=new Post(this);
	list=List::instance();
	load=Load::instance();
	Local::objects["Info"]=info;
	Local::objects["Menu"]=menu;
	Local::objects["Next"]=list;
	Local::objects["Post"]=post;
	Local::objects["Load"]=load;
	
	timer=new QTimer(this);
	delay=new QTimer(this);
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
	delay->setSingleShot(true);
	connect(delay,&QTimer::timeout,APlayer::instance(),[this](){
		if(aplayer->getState()==APlayer::Play&&!menu->isVisible()&&!info->isVisible()){
			setCursor(QCursor(Qt::BlankCursor));
		}
		if(!sliding){
			showprg=false;
			render->setDisplayTime(0);
		}
	});

	connect(aplayer,&APlayer::begin,this,[this](){
		if(!isFullScreen()){
			if (geo.isEmpty()){
				geo=saveGeometry();
				setCenter(render->getPreferredSize(),false);
			}
		}
		sca->setEnabled(!isFullScreen());
		rat->setEnabled(true);
		rat->defaultAction()->setChecked(true);
		sca->defaultAction()->setChecked(true);
		render->setDisplayTime(0);
		setWindowFilePath(aplayer->getMedia());
		setWindowFlags();
	});
	connect(aplayer,&APlayer::reach,this,[this](bool m){
		danmaku->resetTime();
		danmaku->clearCurrent();
		rat->setEnabled(false);
		sca->setEnabled(false);
		render->setVideoAspectRatio(0);
		if(!geo.isEmpty()&&(list->finished()||m)){
			if (isFullScreen()){
				fullA->toggle();
			}
			restoreGeometry(geo);
			geo.clear();
		}
		render->setDisplayTime(0);
		setWindowFilePath(QString());
		setWindowFlags();
	});
	connect(aplayer,&APlayer::errorOccurred,[this](int error){
		QString string;
		switch(error){
		case APlayer::ResourceError:
			string=tr("A media resource couldn't be resolved.");
			break;
		case APlayer::FormatError:
			string=tr("The format of a media resource isn't (fully) supported. "
					  "Playback may still be possible, "
					  "but without an audio or video component.");
			break;
		case APlayer::NetworkError:
			string=tr("A network error occurred.");
			break;
		case APlayer::AccessDeniedError:
			string=tr("There are not the appropriate permissions to play a media resource.");
			break;
		case APlayer::ServiceMissingError:
			string=tr("A valid playback service was not found, playback cannot proceed.");
			break;
		default:
			string=tr("An error occurred.");
			break;
		}
		message->warning(tr("Warning"),string);
	});
	
	connect(load   ,&Load::errorOccured,[this](int error){
			QString info=tr("Network error occurred, error code: %1").arg(error);
			QString sugg=Local::instance()->suggestion(error);
			message->warning(tr("Network Error"),sugg.isEmpty()?info:(info+'\n'+sugg));
	});
	connect(load,&Load::progressChanged,message,&Message::setProgress);

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
		Config::exec(lApp->mainWidget());
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

	listA=new QAction(tr("Playlist"),this);
	listA->setObjectName("List");
	listA->setShortcut(Config::getValue("/Shortcut/List",QString("Ctrl+L")));
	addAction(listA);
	connect(listA,&QAction::triggered,std::bind(&Editor::exec,this));
	
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
	
	QAction *vouA=new QAction(tr("VolUp"),this);
	vouA->setObjectName("VoUp");
	vouA->setShortcut(Config::getValue("/Shortcut/VoUp",QString("Up")));
	connect(vouA,&QAction::triggered,[this](){aplayer->setVolume(aplayer->getVolume()+5);});
	QAction *vodA=new QAction(tr("VolDn"),this);
	vodA->setObjectName("VoDn");
	vodA->setShortcut(Config::getValue("/Shortcut/VoDn",QString("Down")));
	connect(vodA,&QAction::triggered,[this](){aplayer->setVolume(aplayer->getVolume()-5);});
	addAction(vouA);
	addAction(vodA);
	
	rat=new QMenu(tr("Ratio"),this);
	rat->setEnabled(false);
	rat->setDefaultAction(rat->addAction(tr("Default")));
	rat->addAction("4:3")   ->setData( 4.0/ 3);
	rat->addAction("16:10") ->setData(16.0/10);
	rat->addAction("16:9")  ->setData(16.0/ 9);
	rat->addAction("1.85:1")->setData(1.85);
	rat->addAction("2.35:1")->setData(2.35);
	connect(rat,&QMenu::triggered,[this](QAction *action){
		render->setVideoAspectRatio(action->data().toDouble());
		if(aplayer->getState()==APlayer::Pause){
			render->draw();
		}
	});
	QActionGroup *g;
	g=new QActionGroup(rat);
	for(QAction *a:rat->actions()){
		g->addAction(a)->setCheckable(true);
	}
	
	sca=new QMenu(tr("Scale"),this);
	sca->setEnabled(false);
	sca->addAction("541*384")->setData(QSize(540,383));
	sca->addAction("672*438")->setData(QSize(671,437));
	sca->addSeparator();
	sca->addAction("1:4");
	sca->addAction("1:2");
	sca->setDefaultAction(sca->addAction("1:1"));
	sca->addAction("2:1");
	connect(sca,&QMenu::triggered,[this](QAction *action){
		if(render->getPreferredSize().isValid()){
			QSize s=action->data().toSize();
			if(s.isEmpty()){
				QStringList l=action->text().split(':');
				s=render->getPreferredSize()*l[0].toInt()/l[1].toInt();
			}
			setCenter(s,false);
		}
	});
	g=new QActionGroup(sca);
	for(QAction *a:sca->actions()){
		g->addAction(a)->setCheckable(true);
	}
	
	setWindowFlags();
	checkForUpdate();
	setCenter(QSize(),true);
}

void Interface::tryLocal(QString p)
{
	QFileInfo info(p);
	QString suffix=info.suffix().toLower();
	if(!info.exists()){
		return;
	}
	else if(Utils::getSuffix(Utils::Danmaku ).contains(suffix)){
		load   ->loadDanmaku(p);
	}
	else if(Utils::getSuffix(Utils::Subtitle).contains(suffix)&&aplayer->getState()!=APlayer::Stop){
		aplayer->addSubtitle(p);
	}
	else{
		switch(Config::getValue("/Interface/Single",1)){
		case 0:
		case 1:
			aplayer->setMedia(p);
			break;
		case 2:
			list->appendMedia(p);
			break;
		}
	}
}

void Interface::tryLocal(QStringList p)
{
	for(const QString &i:p){
		tryLocal(i);
	}
}

void Interface::closeEvent(QCloseEvent *e)
{
	if(aplayer->getState()==APlayer::Stop&&!isFullScreen()&&!isMaximized()){
		QString conf=Config::getValue("/Interface/Size",QString("720,405"));
		QString size=QString("%1,%2").arg(width()*72/logicalDpiX()).arg(height()*72/logicalDpiY());
		Config::setValue("/Interface/Size",conf.endsWith(' ')?conf.trimmed():size);
	}
	QWidget::closeEvent(e);
	lApp->exit();
}

void Interface::dragEnterEvent(QDragEnterEvent *e)
{
	if (e->mimeData()->hasFormat("text/uri-list")){
		e->acceptProposedAction();
	}
	QWidget::dragEnterEvent(e);
}

void Interface::dropEvent(QDropEvent *e)
{
	if(e->mimeData()->hasFormat("text/uri-list")){
		for(const QString &item:QString(e->mimeData()->data("text/uri-list")).split('\n',QString::SkipEmptyParts)){
			tryLocal(QUrl(item).toLocalFile().trimmed());
		}
	}
}

void Interface::mouseDoubleClickEvent(QMouseEvent *e)
{
	if(!menu->isShown()&&!info->isShown()){
		fullA->toggle();
	}
	QWidget::mouseDoubleClickEvent(e);
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
		QPoint p=e->pos();
		if(!info->geometry().contains(p)&&!menu->geometry().contains(p)){
			if(height()-p.y()<=25&&height()>=p.y()){
				sliding=true;
				render->setDisplayTime(e->x()/(double)width());
			}
			else if(Config::getValue("/Interface/Sensitive",false)){
				aplayer->play();
			}
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
		jump->hide();
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

void Interface::resizeEvent(QResizeEvent *e)
{
	render->resize(e->size());
	int w=e->size().width(),h=e->size().height();
	menu->terminate();
	info->terminate();
	int l=Config::getValue("/Interface/Popup/Width",16*font().pointSizeF())*logicalDpiX()/72;
	menu->setGeometry(menu->isShown()?0:0-l,0,l,h);
	info->setGeometry(info->isShown()?w-l:w,0,l,h);
	post->move(width()/2-post->width()/2,height()-post->height()-2);
	jump->move(width()/2-post->width()/2,2);
	QWidget::resizeEvent(e);
}

void Interface::setWindowFlags()
{
	QFlags<Qt::WindowType> flags=windowFlags();
	if (Config::getValue("/Interface/Frameless",false)){
		flags = Qt::CustomizeWindowHint;
	}
	if((Config::getValue("/Interface/Top",0)+(aplayer->getState()!=APlayer::Stop))>=2){
		flags|= Qt::WindowStaysOnTopHint;
	}
	else{
		flags&=~Qt::WindowStaysOnTopHint;
	}
	if(!testAttribute(Qt::WA_WState_Created)){
		QWidget::setWindowFlags(flags);
	}
	else{
		emit windowFlagsChanged(flags);
	}
}

void Interface::checkForUpdate()
{
	QNetworkAccessManager *manager=new QNetworkAccessManager(this);
	QNetworkRequest request(QUrl("https://raw.githubusercontent.com/AncientLysine/BiliLocal/master/res/INFO"));
	manager->get(request);
	connect(manager,&QNetworkAccessManager::finished,[=](QNetworkReply *info){
		QUrl redirect=info->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
		if(redirect.isValid()){
			manager->get(QNetworkRequest(redirect));
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

void Interface::setCenter(QSize _s,bool _f)
{
	double x=logicalDpiX()/72.0,y=logicalDpiY()/72.0;
	if(!_s.isValid()){
		QStringList l=Config::getValue("/Interface/Size",QString("720,405")).trimmed().split(',',QString::SkipEmptyParts);
		if(l.size()>=2){
			_s=QSize(l[0].toInt()*x,l[1].toInt()*y);
		}
	}
	QSize m=minimumSize();
	QRect r;
	r.setSize(QSize(qMax(m.width(),_s.width()),qMax(m.height(),_s.height())));
	QRect s=QApplication::desktop()->screenGeometry(this);
	QRect t=_f?s:geometry();
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
			r.setSize(QSize(720*x,405*y));
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
		top.addAction(listA);
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
