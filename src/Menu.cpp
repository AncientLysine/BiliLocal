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
#include "Printer.h"
#include "Danmaku.h"

Menu::Menu(QWidget *parent) :
	QWidget(parent)
{
	isPop=false;
	isStay=false;
	setAutoFillBackground(true);
	Utils::setGround(this,Qt::white);
	manager=new QNetworkAccessManager(this);
	manager->setCookieJar(Cookie::instance());
	Cookie::instance()->setParent(NULL);
	animation=new QPropertyAnimation(this,"pos",this);
	animation->setDuration(200);
	animation->setEasingCurve(QEasingCurve::OutCubic);
	fileL=new QLineEdit(this);
	danmL=new QLineEdit(this);
	sechL=new QLineEdit(this);
	fileL->setReadOnly(true);
	danmL->setPlaceholderText(tr("av/ac"));
	fileL->setGeometry(QRect(10,25, 120,25));
	danmL->setGeometry(QRect(10,65, 120,25));
	sechL->setGeometry(QRect(10,105,120,25));
	connect(danmL,&QLineEdit::textEdited,[this](QString text){
		QRegExp regex("a([cv](([0-9]+)(#)?([0-9]+)?)?)?");
		regex.lastIndexIn(text);
		danmL->setText(regex.cap());
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
		QString _file=QFileDialog::getOpenFileName(parentWidget(),tr("Open File"),Utils::getConfig("/Playing/Path",QDir::homePath()));
		if(!_file.isEmpty()){
			setFile(_file);
		}
	});
	connect(danmA,&QAction::triggered,[this](){
		if(Utils::getConfig("/Danmaku/Local",false)){
			QString filter=tr("Danmaku files (*.xml *.json)");
			QString _file=QFileDialog::getOpenFileName(parentWidget(),tr("Open File"),Utils::getConfig("/Playing/Path",QDir::homePath()),filter);
			if(!_file.isEmpty()){
				setDanmaku(_file);
			}
		}
		else{
			setDanmaku(danmL->text());
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
			isStay=true;
		}
		sechL->setText(searchBox.getKey());
	});
	fileA->setShortcut(QKeySequence("Ctrl+O"));
	danmA->setShortcut(QKeySequence("Ctrl+L"));
	sechA->setShortcut(QKeySequence("Ctrl+S"));
	this->addAction(fileA);
	this->addAction(danmA);
	this->addAction(sechA);
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
	alphaS->setValue(Utils::getConfig("/Danmaku/Alpha",1.0)*100);
	connect(alphaS,&QSlider::valueChanged,[this](int _alpha){
		Utils::setConfig("/Danmaku/Alpha",_alpha/100.0);
	});
	powerT=new QLabel(this);
	powerT->setGeometry(QRect(10,205,100,20));
	powerT->setText(tr("Danmaku Power"));
	powerL=new QLineEdit(this);
	powerL->setGeometry(QRect(160,205,30,20));
	Utils::delayExec(this,0,[this](){setPower(Utils::getConfig("/Danmaku/Power",100));});
	connect(powerL,&QLineEdit::textEdited,[this](QString text){
		QRegExp regex("([0-9]+)");
		regex.indexIn(text);
		powerL->setText(regex.cap());
	});
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
	Utils::delayExec(this,0,[this](){localC->setChecked(Utils::getConfig("/Danmaku/Local",false));});
	connect(localC,&QCheckBox::stateChanged,[this](int state){
		if(state==Qt::Checked){
			danmL->setText("");
			danmL->setReadOnly(true);
			danmL->setPlaceholderText("");
			danmB->setText(tr("Open"));
			sechL->setText("");
			sechL->setEnabled(false);
			sechB->setEnabled(false);
		}
		else{
			danmL->setText("");
			danmL->setReadOnly(false);
			danmL->setPlaceholderText(tr("av/ac"));
			danmB->setText(tr("Load"));
			sechL->setEnabled(true);
			sechB->setEnabled(true);
		}
		Utils::setConfig("/Danmaku/Local",state==Qt::Checked);
	});
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
	loopT->setText(tr("Loop"));
	loopC=new QCheckBox(this);
	loopC->setGeometry(QRect(168,310,25,25));
	loopC->setChecked(Utils::getConfig("/Playing/Loop",false));
	connect(loopC,&QCheckBox::stateChanged,[this](int state){
		Utils::setConfig("/Playing/Loop",state==Qt::Checked);
	});

	connect(manager,&QNetworkAccessManager::finished,[this](QNetworkReply *reply){
		auto error=[this](int code){
			isStay=false;
			Printer::instance()->append(QString("[Danmaku]network error %1").arg(code));
			QMessageBox::warning(parentWidget(),tr("Network Error"),tr("Network error occurred, error code: %1").arg(code));
		};

		auto bi=[](const QByteArray &data){
			QStringList l=QString(data).split("<d p=\"");
			l.removeFirst();
			QList<Comment> list;
			for(QString &item:l){
				Comment comment;
				int sta=0;
				int len=item.indexOf("\"");
				QStringList args=item.mid(sta,len).split(',');
				sta=item.indexOf(">")+1;
				len=item.indexOf("<",sta)-sta;
				comment.time=args[0].toDouble()*1000;
				comment.date=args[4].toInt();
				comment.mode=args[1].toInt();
				comment.font=args[2].toInt();
				comment.color=args[3].toInt();
				comment.sender=args[6];
				comment.string=item.mid(sta,len);
				list.append(comment);
			}
			return list;
		};

		auto ac=[](const QByteArray &data){
			QJsonArray a=QJsonDocument::fromJson(data).array();
			QList<Comment> list;
			for(QJsonValue i:a){
				Comment comment;
				QJsonObject item=i.toObject();
				QStringList args=item["c"].toString().split(',');
				comment.time=args[0].toDouble()*1000;
				comment.date=args[5].toInt();
				comment.mode=args[2].toInt();
				comment.font=args[3].toInt();
				comment.color=args[1].toInt();
				comment.sender=args[4];
				comment.string=item["m"].toString();
				list.append(comment);
			}
			return list;
		};

		QString url=reply->url().url();
		Printer::instance()->append(QString("[Danmaku]%1").arg(url));
		if(reply->error()==QNetworkReply::NoError){
			QUrl redirect=reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
			QRegularExpression::PatternOption option=QRegularExpression::CaseInsensitiveOption;
			if(redirect.isValid()){
				reply->manager()->get(QNetworkRequest(redirect));
				return;
			}
			isStay=false;
			if(reply->url().isLocalFile()||url.startsWith("http://comment.")){
				Record load;
				load.source=url;
				if(url.endsWith("xml")){
					load.danmaku=bi(reply->readAll());
				}
				if(url.endsWith("json")){
					load.danmaku=ac(reply->readAll());
				}
				Danmaku::instance()->appendToPool(load);
				Printer::instance()->append(QString("[Danmaku]%1 records loaded").arg(load.danmaku.size()));
			}
			else if(url.startsWith("http://www.bilibili.tv/")){
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
							item->setData(select.mid(sta,select.indexOf('<',sta)-sta),Qt::EditRole);
							model->appendRow(item);
							cur+=regex.matchedLength();
						}
						if(model->rowCount()>0){
							if(isPop){
								danmC->complete();
							}
							flag=false;
						}
					}
				}
				if(flag){
					id=QRegularExpression("((?<=cid=)|(?<=cid:'))\\d+",option).match(video).captured();
					if(!id.isEmpty()){
						api="http://comment.bilibili.tv/%1.xml";
						reply->manager()->get(QNetworkRequest(QUrl(api.arg(id))));
					}
					else{
						error(404);
					}
				}
			}
			else if(url.startsWith("http://www.acfun.tv/")){
				if(url.endsWith(".aspx")){
					QJsonObject json=QJsonDocument::fromJson(reply->readAll()).object();
					if(json.contains("cid")){
						QString api="http://comment.acfun.tv/%1.json";
						QUrl jsonUrl(api.arg(json["cid"].toString()));
						reply->manager()->get(QNetworkRequest(jsonUrl));
					}
					else{
						error(404);
					}
				}
				else{
					bool flag=true;
					QString api,id,video(reply->readAll());
					if(url.indexOf("_")==-1){
						int sta;
						if((sta=video.indexOf("<div id=\"area-pager\""))!=-1){
							int len=video.indexOf("</div>",sta)-sta+1;
							len=len<0?0:len;
							QString select=video.mid(sta,len);
							QRegExp regex("href\\=\"[^\"]+");
							int cur=0;
							api="http://www.acfun.tv";
							QStandardItemModel *model=dynamic_cast<QStandardItemModel *>(danmC->model());
							model->clear();
							while((cur=regex.indexIn(select,cur))!=-1){
								int sta=select.indexOf("i>",cur)+2;
								QStandardItem *item=new QStandardItem();
								item->setData(QUrl(api+regex.cap().mid(6)),Qt::UserRole);
								item->setData(select.mid(sta,select.indexOf('<',sta)-sta),Qt::EditRole);
								model->appendRow(item);
								cur+=regex.matchedLength();
							}
							if(model->rowCount()>0){
								QStandardItem *f=model->item(0);
								f->setData(QUrl(url+"_1"),Qt::UserRole);
								if(isPop){
									danmC->complete();
								}
								flag=false;
							}
						}
					}
					if(flag){
						QRegularExpressionMatch match=QRegularExpression("(?<=\\[video\\])\\d+(?=\\[/video\\])",option).match(video);
						if(match.hasMatch()){
							id=match.captured();
							api="http://www.acfun.tv/api/player/vids/%1.aspx";
						}
						else{
							match=QRegularExpression("(?<=id=)\\w+",option).match(video,video.indexOf("<embed"));
							id=match.captured();
							api="http://comment.acfun.tv/%1.json";
						}
						if(!id.isEmpty()){
							reply->manager()->get(QNetworkRequest(QUrl(api.arg(id))));
						}
						else{
							error(404);
						}
					}
				}
			}
			else if(url.startsWith("http://comic.letv.com/")){
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
					item->setData(video.mid(sta,video.indexOf('<',sta)-sta),Qt::EditRole);
					model->appendRow(item);
					cur+=regex.matchedLength();
				}
				if(model->rowCount()==0){
					error(404);
				}
				else if(isPop){
					danmC->complete();
				}
			}
		}
		else{
			error(reply->error());
		}
	});
}

void Menu::pop()
{
	if(!isPop&&animation->state()==QAbstractAnimation::Stopped){
		animation->setStartValue(pos());
		animation->setEndValue(pos()+QPoint(200,0));
		animation->start();
		isPop=true;
	}
}

void Menu::push(bool force)
{
	if(isPop&&animation->state()==QAbstractAnimation::Stopped&&((!isStay&&danmC->popup()->isHidden())||force)){
		animation->setStartValue(pos());
		animation->setEndValue(pos()-QPoint(200,0));
		animation->start();
		isPop=false;
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
	Utils::setConfig("/Playing/Path",file.absolutePath());
	emit open(QDir::toNativeSeparators(file.absoluteFilePath()));
	bool only=Utils::getConfig("/Playing/Clear",true);
	if(Utils::getConfig("/Danmaku/Local",false)&&(Danmaku::instance()->rowCount()==0||only)){
		for(const QFileInfo &info:file.dir().entryInfoList()){
			QString suffix=info.suffix();
			if((suffix=="xml"||suffix=="json")&&file.baseName()==info.baseName()){
				setDanmaku(info.absoluteFilePath());
				if(only){
					break;
				}
			}
		}
	}
}

void Menu::setPower(qint16 fps)
{
	if(fps==0){
		powerL->setText("");
	}
	else{
		fps=qBound<qint16>(30,fps,200);
		powerL->setText(QString::number(fps));
	}
	emit power(fps==0?-1:1000/fps);
}

void Menu::openLocal(QString _file)
{
	QFileInfo info(_file);
	if(info.exists()){
		QString suffix=info.suffix();
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
	QUrl url;
	int sharp=_code.indexOf("#");
	QString s=_code.mid(0,2);
	QString i=_code.mid(2,sharp-2);
	QString p=sharp==-1?QString():_code.mid(sharp+1);
	if((s=="av"||s=="ac")&&_code.length()>2){
		QString u;
		if(s=="av"){
			u=QString("http://www.bilibili.tv/video/av%1/").arg(i);
			if(!p.isEmpty()){
				u+=QString("index_%1.html").arg(p);
			}
		}
		if(s=="ac"){
			u=QString("http://www.acfun.tv/v/ac%1").arg(i);
			if(!p.isEmpty()){
				u+=QString("_%1").arg(p);
			}
		}
		url=QUrl(u);
		if(Utils::getConfig("/Danmaku/Local",false)){
			localC->toggle();
		}
		danmL->setText(_code);
	}
	else if(QFile::exists(_code)){
		url=QUrl::fromLocalFile(_code);
		if(!Utils::getConfig("/Danmaku/Local",false)){
			localC->toggle();
		}
		danmL->setText(QFileInfo(_code).fileName());
	}
	if(url.isValid()){
		if(Utils::getConfig("/Playing/Clear",true)){
			Danmaku::instance()->clearPool();
		}
		manager->get(QNetworkRequest(url));
	}
}
