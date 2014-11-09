/*=======================================================================
*
*   Copyright (C) 2013 Lysine.
*
*   Filename:    Load.cpp
*   Time:        2014/04/22
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

#include "Load.h"
#include "APlayer.h"
#include "Config.h"
#include "Danmaku.h"
#include "Utils.h"

Load *Load::ins=NULL;

Load *Load::instance()
{
	return ins?ins:new Load(qApp);
}

Load::Load(QObject *parent):
	QObject(parent)
{
	model=new QStandardItemModel(this);
	automated=Config::getValue("/Danmaku/Auto",true);
	ins=this;
	setObjectName("Load");

	manager=new QNetworkAccessManager(this);
	Config::setManager(manager);
	connect(manager,&QNetworkAccessManager::finished,[this](QNetworkReply *reply){
		auto error=[this](int code){
			if(code==QNetworkReply::OperationCanceledError){
				return;
			}
			QEvent e(QEvent::User);
			if(!qApp->sendEvent(this,&e)){
				emit stateChanged(code);
				dequeue();
			}
		};
		Task &task=queue.front();
		reply->deleteLater();
		QString url=reply->url().url(),str=task.code;
		if(reply->error()!=QNetworkReply::NoError){
			error(reply->error());
			return;
		}
		QUrl redirect=reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
		if(redirect.isValid()){
			forward(QNetworkRequest(redirect));
			return;
		}
		Utils::Site site=Utils::parseSite(url);
		if(reply->url().isLocalFile()||url.indexOf("comment")!=-1){
			emit stateChanged(task.state=Pool);
			Record load;
			load.full=true;
			load.source=url;
			load.string=str;
			load.delay=task.delay;
			if(url.endsWith("xml",Qt::CaseInsensitive)){
				QByteArray data=reply->readAll();
				if(data.indexOf("<packet>")!=-1){
					load.danmaku=Utils::parseComment(data,Utils::Niconico);
				}
				else if(data.indexOf("<i>")!=-1){
					load.danmaku=Utils::parseComment(data,Utils::Bilibili);
					load.full=reply->url().isLocalFile();
				}
				else if(data.indexOf("<c>")!=-1){
					load.danmaku=Utils::parseComment(data,Utils::AcfunLocalizer);
				}
			}
			else if(url.indexOf("acfun")!=-1||url.endsWith("json",Qt::CaseInsensitive)){
				load.danmaku=Utils::parseComment(reply->readAll(),Utils::AcFun);
			}
			else if(url.indexOf("acplay")!=-1){
				load.danmaku=Utils::parseComment(reply->readAll(),Utils::AcPlay);
			}
			if(load.delay!=0){
				for(Comment &c:load.danmaku){
					c.time+=load.delay;
				}
			}
			Danmaku::instance()->appendToPool(load);
			emit stateChanged(task.state=None);
			dequeue();
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
					api="http://www."+Utils::customUrl(Utils::Bilibili);
					model->clear();
					while((cur=regex.indexIn(select,cur))!=-1){
						sta=select.indexOf('>',cur)+1;
						cur+=regex.matchedLength();
						QStandardItem *item=new QStandardItem;
						item->setData(QUrl(api+regex.cap().mid(7)),UrlRole);
						item->setData((str+"#%1").arg(model->rowCount()+1),StrRole);
						item->setData(Page,NxtRole);
						item->setData(Utils::decodeXml(select.mid(sta,select.indexOf('<',sta)-sta)),Qt::EditRole);
						model->appendRow(item);
					}
					if(model->rowCount()>0){
						emit stateChanged(task.state=Part);
						flag=false;
					}
				}
			}
			if(flag){
				id=QRegularExpression("((?<=cid=)|(?<=\"cid\":\"))\\d+",QRegularExpression::CaseInsensitiveOption).match(video).captured();
				if(!id.isEmpty()){
					api="http://comment.%1/%2.xml";
					api=api.arg(Utils::customUrl(Utils::Bilibili));
					forward(QNetworkRequest(api.arg(id)),File);
				}
				else{
					error(203);
				}
			}
		}
		else if(site==Utils::AcFun){
			model->clear();
			QRegularExpressionMatchIterator match=QRegularExpression("<a data-vid.*?</a>").globalMatch(reply->readAll());
			while(match.hasNext()){
				QStandardItem *item=new QStandardItem;
				QString part=match.next().captured();
				QRegularExpression r;
				r.setPattern("(?<=>)[^>]+?(?=</a>)");
				item->setData(Utils::decodeXml(r.match(part).captured()),Qt::EditRole);
				r.setPattern("(?<=data-vid=\").+?(?=\")");
				QString next("http://static.comment.acfun.mm111.net/%1");
				next=next.arg(r.match(part).captured());
				item->setData(next,UrlRole);
				item->setData((str+"#%1").arg(model->rowCount()+1),StrRole);
				item->setData(File,NxtRole);
				model->appendRow(item);
			}
			if(url.indexOf('_')==-1&&model->rowCount()>=2){
				emit stateChanged(task.state=Part);
			}
			else{
				int i=url.indexOf('_');
				i=(i==-1)?0:(url.mid(i+1).toInt()-1);
				if(i>=0&&i<model->rowCount()){
					forward(QNetworkRequest(model->item(i)->data(UrlRole).toUrl()),File);
				}
				else{
					error(203);
				}
			}
		}
		else{
			error(203);
		}
	});

	connect(APlayer::instance(),&APlayer::mediaChanged,[this](QString _file){
		if(!automated||!Config::getValue("/Danmaku/Local",false)){
			return;
		}
		QFileInfo info(_file);
		QStringList accept=Utils::getSuffix(Utils::Danmaku);
		bool only=Config::getValue("/Playing/Clear",true);
		if(Danmaku::instance()->rowCount()==0||only){
			for(const QFileInfo &iter:info.dir().entryInfoList()){
				if(accept.contains(iter.suffix().toLower())&&info.baseName()==iter.baseName()){
					loadDanmaku(iter.absoluteFilePath());
					if(only){
						break;
					}
				}
			}
		}
	});
}

bool Load::event(QEvent *e)
{
	return e->type()==QEvent::User?false:QObject::event(e);
}

int  Load::size()
{
	return queue.size();
}

Load::Task Load::codeToTask(QString code)
{
	Task task;
	int sharp=code.indexOf("#");
	QString s=code.mid(0,2);
	QString i=code.mid(2,sharp-2);
	QString p=sharp==-1?QString():code.mid(sharp+1);
	if((s=="av"||s=="ac"||s=="dd")&&code.length()>2){
		QString url;
		if(s=="av"){
			url=QString("http://www.%1/video/av%2/");
			url=url.arg(Utils::customUrl(Utils::Bilibili)).arg(i);\
			if(!p.isEmpty()){
				url+=QString("index_%1.html").arg(p);
			}
			task.request.setUrl(url);
			task.state=Page;
		}
		if(s=="ac"){
			url=QString("http://www.%1/v/ac%2");
			url=url.arg(Utils::customUrl(Utils::AcFun)).arg(i);
			if(!p.isEmpty()){
				url+=QString("_%1").arg(p);
			}
			task.request.setUrl(url);
			task.state=Page;
		}
		if(s=="dd"){
			url=QString("http://api.%1/api/v1/comment/%2");
			url=url.arg(Utils::customUrl(Utils::AcPlay)).arg(i);
			task.request.setUrl(url);
			task.request.setRawHeader("Accept","application/json");
			task.state=File;
		}
	}
	else if(QFile::exists(code)){
		task.request.setUrl(QUrl::fromLocalFile(code));
		code=QFileInfo(code).fileName();
		task.state=File;
	}
	else{
		return task;
	}
	APlayer *aplayer=APlayer::instance();
	task.code=code;
	task.delay=aplayer->getState()!=APlayer::Stop&&Config::getValue("/Playing/Delay",false)?aplayer->getTime():0;
	return task;
}

QStandardItemModel *Load::getModel()
{
	return model;
}

QString Load::getStr()
{
	return queue.isEmpty()?QString():queue.head().code;
}

QString Load::getUrl()
{
	return queue.isEmpty()?QString():queue.head().request.url().url();
}

void Load::setAutoLoad(bool enabled)
{
	Config::setValue("/Danmaku/Auto",automated=enabled);
}

bool Load::autoLoad()
{
	return automated;
}

void Load::loadDanmaku(QString code)
{
	const Task &task=codeToTask(code);
	if(!task.code.isEmpty()){
		enqueue(task);
		if (Config::getValue("/Playing/Clear", true)){
			Danmaku::instance()->clearPool();
		}
	}
}

void Load::loadDanmaku(const QModelIndex &index)
{
	if(index.isValid()){
		QVariant u=index.data(UrlRole),s=index.data(StrRole);
		if(u.isValid()&&s.isValid()){
			Task task;
			task.code=s.toString();
			task.request=QNetworkRequest(u.toUrl());
			task.state=index.data(NxtRole).toInt();
			APlayer *aplayer=APlayer::instance();
			task.delay=aplayer->getState()!=APlayer::Stop&&Config::getValue("/Playing/Delay",false)?aplayer->getTime():0;
			enqueue(task);
		}
	}
	else{
		int c=model->rowCount();
		for(int i=0;i<c;++i){
			loadDanmaku(model->index(i,0));
		}
	}
}

void Load::dequeue()
{
	queue.dequeue();
	if(!queue.isEmpty()){
		forward();
	}
}

bool Load::enqueue(const Task &task)
{
	for(const Task &iter:queue){
		if(iter.code==task.code){
			return false;
		}
	}
	queue.enqueue(task);
	if(queue.size()==1){
		forward();
	}
	return true;
}

void Load::forward()
{
	Task &task=queue.head();
	emit stateChanged(task.state);
	manager->get(task.request);
}

void Load::forward(QNetworkRequest request)
{
	Task &task=queue.head();
	task.request=request;
	emit stateChanged(task.state);
	manager->get(task.request);
}

void Load::forward(QNetworkRequest request,int state)
{
	Task &task=queue.head();
	task.request=request;
	emit stateChanged(task.state=state);
	manager->get(task.request);
}
