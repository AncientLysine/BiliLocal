/*=======================================================================
*
*   Copyright (C) 2013 Lysine.
*
*   Filename:    Menu.cpp
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

#include "Menu.h"
#include "Utils.h"
#include "Search.h"
#include "Cookie.h"
#include "Danmaku.h"
#include "VPlayer.h"

Menu::Menu(QWidget *parent):
	QWidget(parent)
{
	isStay=isPoped=false;
	Utils::setGround(this,Qt::white);
	manager=new QNetworkAccessManager(this);
	manager->setCookieJar(Cookie::instance());
	Cookie::instance()->setParent(NULL);
	fileL=new QLineEdit(this);
	danmL=new QLineEdit(this);
	sechL=new QLineEdit(this);
	fileL->installEventFilter(this);
	danmL->installEventFilter(this);
	sechL->installEventFilter(this);
	fileL->setReadOnly(true);
	danmL->setPlaceholderText(tr("av/ac/dd"));
	fileL->setGeometry(QRect(10,25, 120,25));
	danmL->setGeometry(QRect(10,65, 120,25));
	sechL->setGeometry(QRect(10,105,120,25));
	connect(danmL,&QLineEdit::textEdited,[this](QString text){
		QRegularExpression regexp("[ad]([cvd]((\\d+)([#_])?(\\d+)?)?)?");
		regexp.setPatternOptions(QRegularExpression::CaseInsensitiveOption);
		auto iter=regexp.globalMatch(text);
		QString match;
		while(iter.hasNext()){
			match=iter.next().captured();
		}
		danmL->setText(match.toLower().replace('_','#'));
	});
	danmC=new QCompleter(new QStandardItemModel(this),this);
	danmC->setCompletionMode(QCompleter::UnfilteredPopupCompletion);
	danmC->setCaseSensitivity(Qt::CaseInsensitive);
	danmC->setWidget(danmL);
	QAbstractItemView *popup=danmC->popup();
	popup->setMouseTracking(true);
	connect(popup,SIGNAL(entered(QModelIndex)),popup,SLOT(setCurrentIndex(QModelIndex)));
	connect<void (QCompleter::*)(const QModelIndex &)>(danmC,&QCompleter::activated,[this](const QModelIndex &index){
		if(!index.data(Qt::UserRole+1).toBool()){
			danmL->setText(danmL->text()+QString("#%1").arg(index.row()+1));
		}
		manager->get(QNetworkRequest(index.data(Qt::UserRole).toUrl()));
	});
	fileB=new QPushButton(this);
	sechB=new QPushButton(this);
	danmB=new QPushButton(this);
	fileB->setGeometry(QRect(135,25, 55,25));
	danmB->setGeometry(QRect(135,65, 55,25));
	sechB->setGeometry(QRect(135,105,55,25));
	fileB->setText(tr("Open"));
	danmB->setText(tr("Load"));
	sechB->setText(tr("Search"));
	fileA=new QAction(tr("Open File"),this);
	danmA=new QAction(tr("Load Danmaku"),this);
	sechA=new QAction(tr("Search Danmaku"),this);
	connect(fileA,&QAction::triggered,[this](){
		QString _file=QFileDialog::getOpenFileName(parentWidget(),
												   tr("Open File"),
												   Utils::defaultPath());
		if(!_file.isEmpty()){
			setFile(_file);
		}
	});
	connect(danmA,&QAction::triggered,[this](){
		if(Utils::getConfig("/Danmaku/Local",false)){
			QString _file=QFileDialog::getOpenFileName(parentWidget(),
													   tr("Open File"),
													   Utils::defaultPath(),
													   tr("Danmaku files (*.xml *.json)"));
			if(!_file.isEmpty()){
				setDanmaku(_file);
			}
		}
		else{
			if(danmL->text().isEmpty()){
				pop();
				isStay=true;
				danmL->setFocus();
			}
			else{
				setDanmaku(danmL->text());
			}
		}
	});
	connect(sechA,&QAction::triggered,[this](){
		Search searchBox(parentWidget());
		sechL->setText(sechL->text().simplified());
		if(!sechL->text().isEmpty()){
			searchBox.setKey(sechL->text());
		}
		if(searchBox.exec()) {
			setDanmaku(searchBox.getAid());
		}
		sechL->setText(searchBox.getKey());
	});
	addAction(fileA);
	addAction(danmA);
	addAction(sechA);
	connect(fileB,&QPushButton::clicked,fileA,&QAction::trigger);
	connect(danmB,&QPushButton::clicked,danmA,&QAction::trigger);
	connect(sechB,&QPushButton::clicked,sechA,&QAction::trigger);
	connect(danmL,&QLineEdit::returnPressed,danmA,&QAction::trigger);
	connect(sechL,&QLineEdit::returnPressed,sechA,&QAction::trigger);
	alphaT=new QLabel(this);
	alphaT->setGeometry(QRect(10,145,100,25));
	alphaT->setText(tr("Danmaku Alpha"));
	alphaS=new QSlider(this);
	alphaS->setOrientation(Qt::Horizontal);
	alphaS->setGeometry(QRect(10,170,180,15));
	alphaS->setRange(0,100);
	alphaS->setValue(Utils::getConfig("/Danmaku/Alpha",100));
	connect(alphaS,&QSlider::valueChanged,[this](int _alpha){
		Utils::setConfig("/Danmaku/Alpha",_alpha);
	});
	powerT=new QLabel(this);
	powerT->setGeometry(QRect(10,205,100,20));
	powerT->setText(tr("Danmaku Power"));
	powerL=new QLineEdit(this);
	powerL->setGeometry(QRect(160,205,30,20));
	powerL->setValidator(new QRegularExpressionValidator(QRegularExpression("^\\w*$")));
	powerC=new QTimer(this);
	powerC->setTimerType(Qt::PreciseTimer);
	setPower(Utils::getConfig("/Danmaku/Power",60));
	connect(powerL,&QLineEdit::editingFinished,[this](){
		int p=powerL->text().toInt();
		setPower(p);
		Utils::setConfig("/Danmaku/Power",p);
	});
	localT=new QLabel(this);
	localT->setGeometry(QRect(10,240,100,25));
	localT->setText(tr("Local Danmaku"));
	localC=new QCheckBox(this);
	localC->setGeometry(QRect(168,240,25,25));
	connect(localC,&QCheckBox::stateChanged,[this](int state){
		bool local=state==Qt::Checked;
		danmL->setText("");
		sechL->setText("");
		danmL->setReadOnly(local);
		sechL->setEnabled(!local);
		sechB->setEnabled(!local);
		sechA->setEnabled(!local);
		danmB->setText(local?tr("Open"):tr("Load"));
		danmL->setPlaceholderText(local?QString():tr("av/ac/dd"));
		Utils::setConfig("/Danmaku/Local",local);
	});
	localC->setChecked(Utils::getConfig("/Danmaku/Local",false));
	subT=new QLabel(this);
	subT->setGeometry(QRect(10,275,100,25));
	subT->setText(tr("Protect Sub"));
	subC=new QCheckBox(this);
	subC->setGeometry(QRect(168,275,25,25));
	subC->setChecked(Utils::getConfig("/Danmaku/Protect",false));
	connect(subC,&QCheckBox::stateChanged,[this](int state){
		Utils::setConfig("/Danmaku/Protect",state==Qt::Checked);
	});
	loopT=new QLabel(this);
	loopT->setGeometry(QRect(10,310,100,25));
	loopT->setText(tr("Loop Playback"));
	loopC=new QCheckBox(this);
	loopC->setGeometry(QRect(168,310,25,25));
	loopC->setChecked(Utils::getConfig("/Playing/Loop",false));
	connect(loopC,&QCheckBox::stateChanged,[this](int state){
		Utils::setConfig("/Playing/Loop",state==Qt::Checked);
	});

	animation=new QPropertyAnimation(this,"pos",this);
	animation->setDuration(200);
	animation->setEasingCurve(QEasingCurve::OutCubic);
	connect(animation,&QPropertyAnimation::finished,[this](){
		if(!isPoped){
			hide();
		}
	});

	connect(manager,&QNetworkAccessManager::finished,[this](QNetworkReply *reply){
		auto error=[this](int code=203){
			isStay=false;
			QMessageBox::warning(parentWidget(),tr("Network Error"),tr("Network error occurred, error code: %1").arg(code));
		};
		QString url=reply->url().url();
		if(reply->error()==QNetworkReply::NoError){
			QUrl redirect=reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
			if(redirect.isValid()){
				reply->manager()->get(QNetworkRequest(redirect));
				reply->deleteLater();
				return;
			}
			isStay=false;
			Utils::Site site=Utils::getSite(url);
			QRegularExpression::PatternOption option=QRegularExpression::CaseInsensitiveOption;
			if(reply->url().isLocalFile()||url.indexOf("comment")!=-1){
				Record load;
				load.full=true;
				load.source=url;
				if(url.endsWith("xml",Qt::CaseInsensitive)){
					QByteArray data=reply->readAll();
					if(data.indexOf("<i>")!=-1){
						load.danmaku=Utils::parseComment(data,Utils::Bilibili);
						load.full=reply->url().isLocalFile();
					}
					if(data.indexOf("<c>")!=-1){
						load.danmaku=Utils::parseComment(data,Utils::AcfunLocalizer);
					}
				}
				if(url.endsWith("json",Qt::CaseInsensitive)){
					load.danmaku=Utils::parseComment(reply->readAll(),Utils::AcFun);
				}
				if(url.indexOf("acplay")!=-1){
					load.danmaku=Utils::parseComment(reply->readAll(),Utils::AcPlay);
				}
				Danmaku::instance()->appendToPool(load);
			}
			else if(site==Utils::Bilibili){
				bool flag=true;
				QString api,id,video(reply->readAll());
				if(!url.endsWith("html")){
					int sta;
					if((sta=video.indexOf("<div class=\"alist\">"))!=-1){
						int len=video.indexOf("</select>",sta)-sta+1;
						len=len<0?0:len;
						QString select=video.mid(sta,len);
						QRegExp regex("value\\='[^']+");
						int cur=0;
						api="http://www.bilibili.tv";
						QStandardItemModel *model=dynamic_cast<QStandardItemModel *>(danmC->model());
						model->clear();
						while((cur=regex.indexIn(select,cur))!=-1){
							int sta=select.indexOf('>',cur)+1;
							QStandardItem *item=new QStandardItem();
							item->setData(QUrl(api+regex.cap().mid(7)),Qt::UserRole);
							item->setData(Utils::decodeXml(select.mid(sta,select.indexOf('<',sta)-sta)),Qt::EditRole);
							model->appendRow(item);
							cur+=regex.matchedLength();
						}
						if(model->rowCount()>0){
							if(isVisible()){
								danmC->complete();
							}
							flag=false;
						}
					}
				}
				if(flag){
					id=QRegularExpression("((?<=cid=)|(?<=\"cid\":\"))\\d+",option).match(video).captured();
					if(!id.isEmpty()){
						api="http://comment.bilibili.tv/%1.xml";
						reply->manager()->get(QNetworkRequest(QUrl(api.arg(id))));
					}
					else{
						error();
					}
				}
			}
			else if(site==Utils::AcFun){
				if(url.indexOf("getVideo.aspx")!=-1){
					QJsonObject json=QJsonDocument::fromJson(reply->readAll()).object();
					if(json.contains("danmakuId")){
						QString api="http://comment.acfun.tv/%1.json";
						QUrl jsonUrl(api.arg(json["danmakuId"].toString()));
						reply->manager()->get(QNetworkRequest(jsonUrl));
					}
					else{
						error();
					}
				}
				else{
					QList<QStandardItem *> parts;
					QRegularExpressionMatchIterator match=QRegularExpression("<a data-vid.*?</a>").globalMatch(reply->readAll());
					while(match.hasNext()){
						QStandardItem *item=new QStandardItem;
						QString part=match.next().captured();
						QRegularExpression r;
						r.setPattern("(?<=>)[^>]+?(?=</a>)");
						item->setData(Utils::decodeXml(r.match(part).captured()),Qt::EditRole);
						r.setPattern("(?<=data-vid=\").+?(?=\")");
						item->setData("http://www.acfun.tv/video/getVideo.aspx?id="+r.match(part).captured(),Qt::UserRole);
						parts.append(item);
					}
					if(url.indexOf('_')==-1&&parts.size()>=2){
						QStandardItemModel *model=dynamic_cast<QStandardItemModel *>(danmC->model());
						model->clear();
						for(QStandardItem *item:parts){
							model->appendRow(item);
						}
						if(isVisible()){
							danmC->complete();
						}
					}
					else{
						int i=url.indexOf('_');
						i=(i==-1)?0:(url.mid(i+1).toInt()-1);
						if(i>=0&&i<parts.size()){
							reply->manager()->get(QNetworkRequest(parts[i]->data(Qt::UserRole).toUrl()));
						}
						else{
							error();
						}
						qDeleteAll(parts);
					}
				}
			}
			else if(site==Utils::Letv){
				QString api,video(reply->readAll());
				video=video.mid(video.indexOf("<div class=\"page_box\">"));
				QRegExp regex("cid\\=\"\\d+");
				int cur=0;
				api="http://comment.bilibili.tv/%1.xml";
				QStandardItemModel *model=dynamic_cast<QStandardItemModel *>(danmC->model());
				model->clear();
				while((cur=regex.indexIn(video,cur))!=-1){
					int sta=video.indexOf('>',cur)+1;
					QStandardItem *item=new QStandardItem();
					item->setData(true,Qt::UserRole+1);
					item->setData(QUrl(api.arg(regex.cap().mid(5))),Qt::UserRole);
					item->setData(Utils::decodeXml(video.mid(sta,video.indexOf('<',sta)-sta)),Qt::EditRole);
					model->appendRow(item);
					cur+=regex.matchedLength();
				}
				if(model->rowCount()==0){
					error();
				}
				else if(isVisible()){
					danmC->complete();
				}
			}
			else{
				error();
			}
		}
		else{
			error(reply->error());
		}
		reply->deleteLater();
	});
	hide();
}

bool Menu::eventFilter(QObject *o,QEvent *e)
{
	if(e->type()==QEvent::ContextMenu){
		isStay=1;
		QMenu *m=dynamic_cast<QLineEdit *>(o)->createStandardContextMenu();
		m->exec(dynamic_cast<QContextMenuEvent *>(e)->globalPos());
		delete m;
		isStay=0;
		return 1;
	}
	else{
		return 0;
	}
}

void Menu::pop()
{
	if(!isPoped&&animation->state()==QAbstractAnimation::Stopped){
		show();
		animation->setStartValue(pos());
		animation->setEndValue(pos()+QPoint(200,0));
		animation->start();
		isPoped=true;
	}
}

void Menu::push(bool force)
{
	if(isPoped&&animation->state()==QAbstractAnimation::Stopped&&(!preferStay()||force)){
		if(force){
			isStay=false;
		}
		animation->setStartValue(pos());
		animation->setEndValue(pos()-QPoint(200,0));
		animation->start();
		isPoped=false;
	}
}

void Menu::terminate()
{
	if(animation->state()!=QAbstractAnimation::Stopped){
		animation->setCurrentTime(animation->totalDuration());
	}
}

void Menu::setFile(QString _file)
{
	QFileInfo file(_file);
	fileL->setText(file.fileName());
	fileL->setCursorPosition(0);
	Utils::setConfig("/Playing/Path",file.absolutePath());
	VPlayer::instance()->setMedia(file.absoluteFilePath());
	bool only=Utils::getConfig("/Playing/Clear",true);
	if(Utils::getConfig("/Danmaku/Local",false)&&(Danmaku::instance()->rowCount()==0||only)){
		for(const QFileInfo &info:file.dir().entryInfoList()){
			QString suffix=info.suffix().toLower();
			if((suffix=="xml"||suffix=="json")&&file.baseName()==info.baseName()){
				setDanmaku(info.absoluteFilePath());
				if(only) break;
			}
		}
	}
}

void Menu::setPower(qint16 fps)
{
	if(fps==0){
		powerC->stop();
		powerL->setText("");
	}
	else{
		fps=qBound<qint16>(30,fps,200);
		powerC->start(1000/fps);
		powerL->setText(QString::number(fps));
	}
}

void Menu::openLocal(QString _file)
{
	QFileInfo info(_file);
	if(info.exists()){
		QString suffix=info.suffix().toLower();
		if(suffix=="xml"||suffix=="json"){
			setDanmaku(_file);
		}
		else{
			setFile(_file);
		}
	}
}

void Menu::setDanmaku(QString _code)
{
	QNetworkRequest request;
	int sharp=_code.indexOf("#");
	QString s=_code.mid(0,2);
	QString i=_code.mid(2,sharp-2);
	QString p=sharp==-1?QString():_code.mid(sharp+1);
	if((s=="av"||s=="ac"||s=="dd")&&_code.length()>2){
		QString url;
		if(s=="av"){
			url=QString("http://www.bilibili.tv/video/av%1/").arg(i);
			if(!p.isEmpty()){
				url+=QString("index_%1.html").arg(p);
			}
			request.setUrl(url);
		}
		if(s=="ac"){
			url=QString("http://www.acfun.com/v/ac%1").arg(i);
			if(!p.isEmpty()){
				url+=QString("_%1").arg(p);
			}
			request.setUrl(url);
		}
		if(s=="dd"){
			url=QString("http://api.acplay.net/api/v1/comment/")+i;
			request.setUrl(url);
			request.setRawHeader("Accept","application/json");
		}
		if(Utils::getConfig("/Danmaku/Local",false)){
			localC->toggle();
		}
		danmL->setText(_code);
	}
	else if(QFile::exists(_code)){
		request.setUrl(QUrl::fromLocalFile(_code));
		if(!Utils::getConfig("/Danmaku/Local",false)){
			localC->toggle();
		}
		danmL->setText(QFileInfo(_code).fileName());
		danmL->setCursorPosition(0);
	}
	if(request.url().isValid()){
		if(Utils::getConfig("/Playing/Clear",true)){
			Danmaku::instance()->clearPool();
		}
		isStay=true;
		manager->get(request);
	}
}
