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
#include "Utils.h"
#include "Config.h"
#include "Danmaku.h"
#include "APlayer.h"

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

	manager=new QNetworkAccessManager(this);
	Config::setManager(manager);
	connect(manager,&QNetworkAccessManager::finished,[this](QNetworkReply *reply){
		auto error=[this](int code){
			if(code!=QNetworkReply::OperationCanceledError){
				emit stateChanged(code);
			}
		};
		QString url=reply->url().url();
		QString str=reply->request().attribute(QNetworkRequest::User).toString();
		if(reply->error()==QNetworkReply::NoError){
			QUrl redirect=reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
			if(redirect.isValid()){
				getReply(QNetworkRequest(redirect),"");
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
				Danmaku::instance()->appendToPool(load);
				emit stateChanged(None);
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
							int sta=select.indexOf('>',cur)+1;
							QStandardItem *item=new QStandardItem;
							item->setData(QUrl(api+regex.cap().mid(7)),UrlRole);
							item->setData((str+"#%1").arg(model->rowCount()+1),StrRole);
							item->setData(Utils::decodeXml(select.mid(sta,select.indexOf('<',sta)-sta)),Qt::EditRole);
							model->appendRow(item);
							cur+=regex.matchedLength();
						}
						if(model->rowCount()>0){
							emit stateChanged(Part);
							flag=false;
						}
					}
				}
				if(flag){
					id=QRegularExpression("((?<=cid=)|(?<=\"cid\":\"))\\d+",QRegularExpression::CaseInsensitiveOption).match(video).captured();
					if(!id.isEmpty()){
						api="http://comment.bilibili.com/%1.xml";
						getReply(QNetworkRequest(QUrl(api.arg(id))),"");
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
						getReply(QNetworkRequest(jsonUrl),"");
						emit stateChanged(Code);
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
						model->appendRow(item);
					}
					if(url.indexOf('_')==-1&&model->rowCount()>=2){
						emit stateChanged(Part);
					}
					else{
						int i=url.indexOf('_');
						i=(i==-1)?0:(url.mid(i+1).toInt()-1);
						if(i>=0&&i<model->rowCount()){
							getReply(QNetworkRequest(model->item(i)->data(UrlRole).toUrl()),"");
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
					item->setData("http://comment.bilibili.com/"+r.match(part).captured()+".xml",UrlRole);
					item->setData((str+"#%1").arg(model->rowCount()+1),StrRole);
					model->appendRow(item);
				}
				if(str.indexOf('#')==-1&&model->rowCount()>=2){
					emit stateChanged(Part);
				}
				else{
					int i=str.indexOf('#');
					i=(i==-1)?0:(str.mid(i+1).toInt()-1);
					if(i>=0&&i<model->rowCount()){
						getReply(QNetworkRequest(model->item(i)->data(UrlRole).toUrl()),"");
					}
					else{
						error(203);
					}
				}
			}
			else{
				error(203);
			}
		}
		else{
			error(reply->error());
		}
		reply->deleteLater();
	});

	connect(APlayer::instance(),&APlayer::mediaChanged,[this](QString _file){
		QFileInfo info(_file);
		bool only=Config::getValue("/Playing/Clear",true);
		if(Config::getValue("/Danmaku/Local",false)&&(Danmaku::instance()->rowCount()==0||only)){
			for(const QFileInfo &iter:info.dir().entryInfoList()){
				if(Utils::getSuffix(Utils::Danmaku).contains(iter.suffix().toLower())&&info.baseName()==iter.baseName()){
					loadDanmaku(iter.absoluteFilePath());
					if(only) break;
				}
			}
		}
	});
}

void Load::getReply(QNetworkRequest request,QString string)
{
	if(!current.isNull()){
		if(!current->isFinished()){
			current->abort();
		}
		else if(!string.isNull()&&string.isEmpty()){
			request.setAttribute(QNetworkRequest::User,current->request().attribute(QNetworkRequest::User));
		}
		current->deleteLater();
	}
	if(!string.isEmpty()){
		request.setAttribute(QNetworkRequest::User,string);
	}
	current=manager->get(request);
	if(Utils::getSuffix(Utils::Danmaku).contains(QFileInfo(request.url().url()).suffix().toLower())){
		emit stateChanged(File);
	}
}

QString Load::getStr()
{
	return current?current->request().attribute(QNetworkRequest::User).toString():QString();
}

QString Load::getUrl()
{
	return current?current->request().url().url():QString();
}

QStandardItemModel *Load::getModel()
{
	return model;
}

void Load::loadDanmaku(QString _code)
{
	QNetworkRequest request;
	int sharp=_code.indexOf("#");
	QString s=_code.mid(0,2);
	QString i=_code.mid(2,sharp-2);
	QString p=sharp==-1?QString():_code.mid(sharp+1);
	if((s=="av"||s=="ac"||s=="dd")&&_code.length()>2){
		QString url;
		if(s=="av"){
			url=QString("http://www.bilibili.com/video/av%1/").arg(i);
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
	}
	else if(QFile::exists(_code)){
		request.setUrl(QUrl::fromLocalFile(_code));
		_code=QFileInfo(_code).fileName();
	}
	else{
		return;
	}
	getReply(request,_code);
	if(!request.url().isLocalFile()){
		emit stateChanged(Page);
	}
	if(Config::getValue("/Playing/Clear",true)){
		Danmaku::instance()->clearPool();
	}
}
