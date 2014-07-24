/*=======================================================================
*
*   Copyright (C) 2013 Lysine.
*
*   Filename:    Load.cpp
*   Time:        2013/04/22
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
	ins=this;
	setObjectName("Load");

	manager=new QNetworkAccessManager(this);
	Config::setManager(manager);
	connect(manager,&QNetworkAccessManager::finished,[this](QNetworkReply *reply){
		auto error=[this](int code){
			if(code==QNetworkReply::OperationCanceledError){
				return;
			}
			dequeue();
			loadTop();
			QEvent e(QEvent::User);
			if(!qApp->sendEvent(this,&e)){
				emit stateChanged(code);
			}
		};
		reply->deleteLater();
		QString url=reply->url().url();
		QString str=reply->request().attribute(QNetworkRequest::User).toString();
		if(reply->error()!=QNetworkReply::NoError){
			error(reply->error());
			return;
		}
		QUrl redirect=reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
		if(redirect.isValid()){
			getReply(QNetworkRequest(redirect));
			loadTop();
			return;
		}
		Utils::Site site=Utils::getSite(url);
		if(reply->url().isLocalFile()||url.indexOf("comment")!=-1){
			Record load;
			load.full=true;
			load.source=url;
			load.string=str;
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
			if(url.endsWith("json",Qt::CaseInsensitive)){
				load.danmaku=Utils::parseComment(reply->readAll(),Utils::AcFun);
			}
			if(url.indexOf("acplay")!=-1){
				load.danmaku=Utils::parseComment(reply->readAll(),Utils::AcPlay);
			}
			dequeue();
			loadTop();
			Danmaku::instance()->appendToPool(load);
			emit stateChanged(None);
			last=QNetworkRequest();
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
					api="http://www.bilibili.com";
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
						dequeue();
						emit stateChanged(Part);
						flag=false;
					}
				}
			}
			if(flag){
				id=QRegularExpression("((?<=cid=)|(?<=\"cid\":\"))\\d+",QRegularExpression::CaseInsensitiveOption).match(video).captured();
				if(!id.isEmpty()){
					api="http://comment.bilibili.com/%1.xml";
					getReply(QNetworkRequest(QUrl(api.arg(id))));
					loadTop();
					emit stateChanged(File);
				}
				else{
					error(203);
				}
			}
		}
		else if(site==Utils::AcFun){
			if(url.indexOf("getVideo.aspx")!=-1){
				QJsonObject json=QJsonDocument::fromJson(reply->readAll()).object();
				if(json.contains("danmakuId")){
					QString api="http://comment.acfun.tv/%1.json";
					QUrl jsonUrl(api.arg(json["danmakuId"].toString()));
					getReply(QNetworkRequest(jsonUrl));
					loadTop();
					emit stateChanged(File);
				}
				else{
					error(203);
				}
			}
			else{
				model->clear();
				QRegularExpressionMatchIterator match=QRegularExpression("<a data-vid.*?</a>").globalMatch(reply->readAll());
				while(match.hasNext()){
					QStandardItem *item=new QStandardItem;
					QString part=match.next().captured();
					QRegularExpression r;
					r.setPattern("(?<=>)[^>]+?(?=</a>)");
					item->setData(Utils::decodeXml(r.match(part).captured()),Qt::EditRole);
					r.setPattern("(?<=data-vid=\").+?(?=\")");
					item->setData("http://www.acfun.tv/video/getVideo.aspx?id="+r.match(part).captured(),UrlRole);
					item->setData((str+"#%1").arg(model->rowCount()+1),StrRole);
					item->setData(Code,NxtRole);
					model->appendRow(item);
				}
				if(url.indexOf('_')==-1&&model->rowCount()>=2){
					dequeue();
					emit stateChanged(Part);
				}
				else{
					int i=url.indexOf('_');
					i=(i==-1)?0:(url.mid(i+1).toInt()-1);
					if(i>=0&&i<model->rowCount()){
						getReply(QNetworkRequest(model->item(i)->data(UrlRole).toUrl()));
						loadTop();
						emit stateChanged(Code);
					}
					else{
						error(203);
					}
				}
			}
		}
		else if(site==Utils::Letv){
			model->clear();
			QRegularExpressionMatchIterator match=QRegularExpression("cid\\=.*?</a>").globalMatch(reply->readAll());
			while(match.hasNext()){
				QStandardItem *item=new QStandardItem;
				QString part=match.next().captured();
				QRegularExpression r;
				r.setPattern("(?<=>)[^>]+?(?=</a>)");
				item->setData(Utils::decodeXml(r.match(part).captured()),Qt::EditRole);
				r.setPattern("(?<=cid=\").+?(?=\")");
				item->setData(File,NxtRole);
				item->setData("http://comment.bilibili.com/"+r.match(part).captured()+".xml",UrlRole);
				item->setData((str+"#%1").arg(model->rowCount()+1),StrRole);
				model->appendRow(item);
			}
			if(str.indexOf('#')==-1&&model->rowCount()>=2){
				dequeue();
				emit stateChanged(Part);
			}
			else{
				int i=str.indexOf('#');
				i=(i==-1)?0:(str.mid(i+1).toInt()-1);
				if(i>=0&&i<model->rowCount()){
					getReply(QNetworkRequest(model->item(i)->data(UrlRole).toUrl()));
					loadTop();
					emit stateChanged(File);
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
		if(!Config::getValue("/Danmaku/Local",false)){
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

bool Load::empty()
{
	return queue.isEmpty();
}

QStandardItemModel *Load::getModel()
{
	return model;
}

void Load::loadTop()
{
	if(!queue.isEmpty()&&queue.head()!=last){
		last=queue.head();
		manager->get(last);
	}
}

void Load::dequeue()
{
	queue.dequeue();
	last=QNetworkRequest();
}

QString Load::getStr()
{
	return queue.isEmpty()?QString():last.attribute(QNetworkRequest::User).toString();
}

QString Load::getUrl()
{
	return queue.isEmpty()?QString():last.url().url();
}

void Load::loadDanmaku(QString code)
{
	QNetworkRequest request;
	int sharp=code.indexOf("#");
	QString s=code.mid(0,2);
	QString i=code.mid(2,sharp-2);
	QString p=sharp==-1?QString():code.mid(sharp+1);
	if((s=="av"||s=="ac"||s=="dd")&&code.length()>2){
		QString url;
		if(s=="av"){
			url=QString("http://www.bilibili.com/video/av%1/").arg(i);
			if(!p.isEmpty()){
				url+=QString("index_%1.html").arg(p);
			}
			request.setUrl(url);
		}
		if(s=="ac"){
			url=QString("http://www.acfun.tv/v/ac%1").arg(i);
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
	}
	else if(QFile::exists(code)){
		request.setUrl(QUrl::fromLocalFile(code));
		code=QFileInfo(code).fileName();
	}
	else{
		return;
	}
	getReply(request,code);
	loadTop();
	if(!request.url().isLocalFile()){
		emit stateChanged(Page);
	}
	else{
		emit stateChanged(File);
	}
	if(Config::getValue("/Playing/Clear",true)){
		Danmaku::instance()->clearPool();
	}
}

void Load::loadDanmaku(const QModelIndex &index)
{
	if(index.isValid()){
		QVariant u=index.data(UrlRole),s=index.data(StrRole);
		if(u.isValid()&&s.isValid()){
			getReply(QNetworkRequest(u.toUrl()),s.toString());
			loadTop();
			emit stateChanged(index.data(NxtRole).toInt());
		}
	}
	else{
		int c=model->rowCount();
		for(int i=0;i<c;++i){
			loadDanmaku(model->index(i,0));
		}
	}
}

void Load::getReply(QNetworkRequest request,QString code)
{
	for(QNetworkRequest &r:queue){
		if(r.attribute(QNetworkRequest::User).toString()==code){
			return;
		}
	}
	if(!code.isEmpty()){
		request.setAttribute(QNetworkRequest::User,code);
		queue.enqueue(request);
	}
	else{
		request.setAttribute(QNetworkRequest::User,queue.head().attribute(QNetworkRequest::User));
		queue.head()=request;
	}
}
