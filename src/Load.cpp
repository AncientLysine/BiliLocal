#include "Load.h"
#include "Utils.h"
#include "Cookie.h"
#include "Danmaku.h"
#include <QtWidgets>

Load *Load::ins=NULL;

Load *Load::instance()
{
	return ins?ins:new Load(qApp);
}

Load::Load(QObject *parent):
	QObject(parent)
{
	manager=new QNetworkAccessManager(this);
	manager->setCookieJar(Cookie::instance());
	Cookie::instance()->setParent(NULL);
	connect(manager,&QNetworkAccessManager::finished,[this](QNetworkReply *reply){
		auto error=[this](int code=203){
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
						api="http://www.bilibili.tv";
						qDeleteAll(parts);
						parts.clear();
						while((cur=regex.indexIn(select,cur))!=-1){
							int sta=select.indexOf('>',cur)+1;
							QStandardItem *item=new QStandardItem;
							item->setData(QUrl(api+regex.cap().mid(7)),Qt::UserRole);
							item->setData((str+"#%1").arg(parts.size()+1),Qt::UserRole+1);
							item->setData(Utils::decodeXml(select.mid(sta,select.indexOf('<',sta)-sta)),Qt::EditRole);
							parts.append(item);
							cur+=regex.matchedLength();
						}
						if(parts.size()>0){
							emit stateChanged(Part);
							flag=false;
						}
					}
				}
				if(flag){
					id=QRegularExpression("((?<=cid=)|(?<=\"cid\":\"))\\d+",QRegularExpression::CaseInsensitiveOption).match(video).captured();
					if(!id.isEmpty()){
						api="http://comment.bilibili.tv/%1.xml";
						getReply(QNetworkRequest(QUrl(api.arg(id))),"");
						emit stateChanged(File);
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
						getReply(QNetworkRequest(jsonUrl),"");
						emit stateChanged(Code);
					}
					else{
						error();
					}
				}
				else{
					qDeleteAll(parts);
					parts.clear();
					QRegularExpressionMatchIterator match=QRegularExpression("<a data-vid.*?</a>").globalMatch(reply->readAll());
					while(match.hasNext()){
						QStandardItem *item=new QStandardItem;
						QString part=match.next().captured();
						QRegularExpression r;
						r.setPattern("(?<=>)[^>]+?(?=</a>)");
						item->setData(Utils::decodeXml(r.match(part).captured()),Qt::EditRole);
						r.setPattern("(?<=data-vid=\").+?(?=\")");
						item->setData("http://www.acfun.tv/video/getVideo.aspx?id="+r.match(part).captured(),Qt::UserRole);
						item->setData((str+"#%1").arg(parts.size()+1),Qt::UserRole+1);
						parts.append(item);
					}
					if(url.indexOf('_')==-1&&parts.size()>=2){
						emit stateChanged(Part);
					}
					else{
						int i=url.indexOf('_');
						i=(i==-1)?0:(url.mid(i+1).toInt()-1);
						if(i>=0&&i<parts.size()){
							getReply(QNetworkRequest(parts[i]->data(Qt::UserRole).toUrl()),"");
							emit stateChanged(File);
						}
						else{
							error();
						}
						qDeleteAll(parts);
					}
				}
			}
			else if(site==Utils::Letv){
				qDeleteAll(parts);
				parts.clear();
				QRegularExpressionMatchIterator match=QRegularExpression("cid\\=.*?</a>").globalMatch(reply->readAll());
				while(match.hasNext()){
					QStandardItem *item=new QStandardItem;
					QString part=match.next().captured();
					QRegularExpression r;
					r.setPattern("(?<=>)[^>]+?(?=</a>)");
					item->setData(Utils::decodeXml(r.match(part).captured()),Qt::EditRole);
					r.setPattern("(?<=cid=\").+?(?=\")");
					item->setData("http://comment.bilibili.tv/"+r.match(part).captured()+".xml",Qt::UserRole);
					item->setData((str+"#%1").arg(parts.size()+1),Qt::UserRole+1);
					parts.append(item);
				}
				if(str.indexOf('#')==-1&&parts.size()>=2){
					emit stateChanged(Part);
				}
				else{
					int i=str.indexOf('#');
					i=(i==-1)?0:(str.mid(i+1).toInt()-1);
					if(i>=0&&i<parts.size()){
						getReply(QNetworkRequest(parts[i]->data(Qt::UserRole).toUrl()),"");
						emit stateChanged(File);
					}
					else{
						error();
					}
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
	ins=this;
}

Load::~Load()
{
	qDeleteAll(parts);
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
}

QString Load::getString()
{
	return current?current->request().attribute(QNetworkRequest::User).toString():QString();
}

QList<QStandardItem *> Load::takeParts()
{
	QList<QStandardItem *> p(parts);
	parts.clear();
	return p;
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
	}
	else if(QFile::exists(_code)){
		request.setUrl(QUrl::fromLocalFile(_code));
		_code=QFileInfo(_code).fileName();
	}
	else{
		return;
	}
	getReply(request,_code);
	emit stateChanged(request.url().isLocalFile()?File:Page);
	if(Utils::getConfig("/Playing/Clear",true)){
		Danmaku::instance()->clearPool();
	}
}
