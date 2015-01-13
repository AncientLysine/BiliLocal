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
#include <functional>

Load *Load::ins=nullptr;

Load *Load::instance()
{
	return ins?ins:new Load(qApp);
}

namespace
{
QList<Comment> parseComment(QByteArray data,Utils::Site site)
{
	QList<Comment> list;
	switch(site){
	case Utils::Bilibili:
	{
		QStringList l=QString(data).split("<d p=\"");
		l.removeFirst();
		for(const QString &item:l){
			Comment comment;
			int sta=0;
			int len=item.indexOf("\"");
			QStringList args=item.mid(sta,len).split(',');
			if (args.size()<=6){
				continue;
			}
			comment.time=args[0].toDouble()*1000+0.5;
			comment.date=args[4].toInt();
			comment.mode=args[1].toInt();
			comment.font=args[2].toInt();
			comment.color=args[3].toInt();
			comment.sender=args[6];
			sta=item.indexOf(">")+1;
			len=item.indexOf("<",sta)-sta;
			comment.string=Utils::decodeXml(item.mid(sta,len),true);
			list.append(comment);
		}
		break;
	}
	case Utils::AcFun:
	{
		QQueue<QJsonArray> queue;
		queue.append(QJsonDocument::fromJson(data).array());
		while(!queue.isEmpty()){
			for(const QJsonValue &i:queue.head()){
				if(i.isArray()){
					queue.append(i.toArray());
				}
				else{
					Comment comment;
					QJsonObject item=i.toObject();
					QStringList args=item["c"].toString().split(',');
					if (args.size()<6){
						continue;
					}
					comment.time=args[0].toDouble()*1000+0.5;
					comment.date=args[5].toInt();
					comment.mode=args[2].toInt();
					comment.font=args[3].toInt();
					comment.color=args[1].toInt();
					comment.sender=args[4];
					comment.string=item["m"].toString();
					list.append(comment);
				}
			}
			queue.dequeue();
		}
		break;
	}
	case Utils::AcPlay:
	{
		QJsonArray a=QJsonDocument::fromJson(data).object()["Comments"].toArray();
		for(const QJsonValue &i:a){
			Comment comment;
			QJsonObject item=i.toObject();
			comment.time=item["Time"].toDouble()*1000+0.5;
			comment.date=item["Timestamp"].toInt();
			comment.mode=item["Mode"].toInt();
			comment.font=25;
			comment.color=item["Color"].toInt();
			comment.sender=QString::number(item["UId"].toInt());
			comment.string=item["Message"].toString();
			list.append(comment);
		}
		break;
	}
	case Utils::AcfunLocalizer:
	{
		QStringList l=QString(data).split("<l i=\"");
		l.removeFirst();
		for(const QString &item:l){
			Comment comment;
			int sta=0;
			int len=item.indexOf("\"");
			QStringList args=item.mid(sta,len).split(',');
			if (args.size()<6){
				continue;
			}
			comment.time=args[0].toDouble()*1000+0.5;
			comment.date=args[5].toInt();
			comment.mode=1;
			comment.font=25;
			comment.color=args[2].toInt();
			comment.sender=args[4];
			sta=item.indexOf("<![CDATA[")+9;
			len=item.indexOf("]]>",sta)-sta;
			comment.string=Utils::decodeXml(item.mid(sta,len),true);
			list.append(comment);
		}
		break;
	}
	case Utils::Niconico:
	{
		QStringList l=QString(data).split("<chat ");
		l.removeFirst();
		for(const QString &item:l){
			Comment comment;
			QString key,val;
			/* 0 wait for key
			 * 1 wait for left quot
			 * 2 wait for value
			 * 3 wait for comment
			 * 4 finsihed */
			int state=0;
			QMap<QString,QString> args;
			for(const QChar &c:item){
				switch(state){
				case 0:
					if(c=='='){
						state=1;
					}
					else if(c=='>'){
						state=3;
					}
					else if(c!=' '){
						key.append(c);
					}
					break;
				case 1:
					if(c=='\"'){
						state=2;
					}
					break;
				case 2:
					if(c=='\"'){
						state=0;
						args.insert(key,val);
						key=val=QString();
					}
					else{
						val.append(c);
					}
					break;
				case 3:
					if(c=='<'){
						state=4;
					}
					else{
						comment.string.append(c);
					}
					break;
				}
			}
			if(state!=4){
				continue;
			}
			comment.time=args["vpos"].toLongLong()*10;
			comment.date=args["date"].toLongLong();
			QStringList ctrl=args["mail"].split(' ',QString::SkipEmptyParts);
			comment.mode=ctrl.contains("shita")?4 :(ctrl.contains("ue") ?5 :1 );
			comment.font=ctrl.contains("small")?15:(ctrl.contains("big")?36:25);
			comment.color=0xFFFFFF;
			for(const QString &name:ctrl){
				QColor color(name);
				if(color.isValid()){
					comment.color=color.rgb();
					break;
				}
			}
			comment.sender=args["user_id"];
			list.append(comment);
		}
		break;
	}
	default:
		break;
	}
	return list;
}
}

Load::Load(QObject *parent):
	QObject(parent)
{
	ins=this;
	setObjectName("Load");
	model  =new QStandardItemModel   (this);
	manager=new QNetworkAccessManager(this);
	Config::setManager(manager);
	connect(manager,&QNetworkAccessManager::finished,[this](QNetworkReply *reply){
		remain.remove(reply);
		reply->deleteLater();
		QUrl redirect=reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
		if (reply->error()!=QNetworkReply::NoError){
			if (reply->error()!=QNetworkReply::OperationCanceledError)
				emit stateChanged(reply->error());
			dequeue();
			return;
		}
		if(!redirect.isValid()){
			queue.head().processer->process(reply);
		}
		else{
			forward(QNetworkRequest(redirect));
		}
	});

	auto avProcess=[this](QNetworkReply *reply){
		Task &task=queue.head();
		int sharp=task.code.indexOf(QRegularExpression("[#_]"));
		switch(task.state){
		case None:
		{
			QString i=task.code.mid(2,sharp-2);
			QString p=sharp==-1?QString():task.code.mid(sharp+1);
			QString url("http://www.%1/video/av%2/");
			url=url.arg(Utils::customUrl(Utils::Bilibili)).arg(i);
			if(!p.isEmpty()){
				url+=QString("index_%1.html").arg(p);
			}
			forward(QNetworkRequest(url),Page);
			break;
		}
		case Page:
		{
			model->clear();
			QString api,id,video(reply->readAll());
			int sta;
			if((sta=video.indexOf("<div class=\"alist\">"))!=-1&&sharp==-1){
				int len=video.indexOf("</select>",sta)-sta+1;
				len=len<0?0:len;
				QString select=video.mid(sta,len);
				QRegExp regex("value\\='[^']+");
				int cur=0;
				api="http://www."+Utils::customUrl(Utils::Bilibili);
				while((cur=regex.indexIn(select,cur))!=-1){
					sta=select.indexOf('>',cur)+1;
					cur+=regex.matchedLength();
					QStandardItem *item=new QStandardItem;
					item->setData(QUrl(api+regex.cap().mid(7)),UrlRole);
					item->setData((task.code+"#%1").arg(model->rowCount()+1),StrRole);
					item->setData(Page,NxtRole);
					item->setData(Utils::decodeXml(select.mid(sta,select.indexOf('<',sta)-sta)),Qt::EditRole);
					model->appendRow(item);
				}
			}
			if (model->rowCount()>0){
				emit stateChanged(task.state=Part);
			}
			else{
				QRegularExpression r=QRegularExpression("((?<=cid=)|(?<=\"cid\":\"))\\d+",QRegularExpression::CaseInsensitiveOption);
				QRegularExpressionMatchIterator i=r.globalMatch(video);
				while(i.hasNext()){
					QString m=i.next().captured();
					if (id.isEmpty()){
						id=m;
					}
					else if(id!=m){
						id.clear();
						break;
					}
				}
				if(!id.isEmpty()){
					api="http://comment.%1/%2.xml";
					api=api.arg(Utils::customUrl(Utils::Bilibili));
					forward(QNetworkRequest(api.arg(id)),File);
				}
				else{
					emit stateChanged(203);
					dequeue();
				}
			}
			break;
		}
		case File:
		{
			dumpDanmaku(reply->readAll(),Utils::Bilibili,false);
			emit stateChanged(task.state=None);
			dequeue();
			break;
		}
		}
	};
	auto avRegular=QRegularExpression("a(v(\\d+([#_])?(\\d+)?)?)?");
	avRegular.setPatternOptions(QRegularExpression::CaseInsensitiveOption);
	pool.append({avRegular,0,avProcess});
	
	auto acProcess=[this](QNetworkReply *reply){
		Task &task=queue.head();
		int sharp=task.code.indexOf(QRegularExpression("[#_]"));
		switch(task.state){
		case None:
		{
			QString i=task.code.mid(2,sharp-2);
			QString p=sharp==-1?QString():task.code.mid(sharp+1);
			QString url("http://www.%1/v/ac%2");
			url=url.arg(Utils::customUrl(Utils::AcFun)).arg(i);
			if(!p.isEmpty()){
				url+=QString("_%1").arg(p);
			}
			forward(QNetworkRequest(url),Page);
			break;;
		}
		case Page:
		{
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
				item->setData((task.code+"#%1").arg(model->rowCount()+1),StrRole);
				item->setData(File,NxtRole);
				model->appendRow(item);
			}
			if (sharp==-1&&model->rowCount()>=2){
				emit stateChanged(task.state=Part);
			}
			else{
				QString url=reply->url().url();
				int i=url.indexOf('_');
				i=(i==-1)?0:(url.mid(i+1).toInt()-1);
				if(i>=0&&i<model->rowCount()){
					forward(QNetworkRequest(model->item(i)->data(UrlRole).toUrl()),File);
				}
				else{
					emit stateChanged(203);
					dequeue();
				}
			}
			break;
		}
		case File:
		{
			dumpDanmaku(reply->readAll(),Utils::AcFun,true);
			emit stateChanged(task.state=None);
			dequeue();
			break;
		}
		}
	};
	auto acRegular=QRegularExpression("a(c(\\d+([#_])?(\\d+)?)?)?");
	pool.append({acRegular,0,acProcess});

	auto directProcess=[this](QNetworkReply *reply){
		Task &task=queue.head();
		switch(task.state){
		case None:
		{
			QUrl url(task.code,QUrl::StrictMode);
			task.request.setUrl(url.isValid()?url:QUrl::fromLocalFile(task.code));
			task.code=QFileInfo(task.code).fileName();
			task.state=File;
			forward();
			break;
		}
		case File:
			Record load;
			load.full=true;
			load.source=reply->url().url();
			load.string=task.code;
			load.delay=task.delay;
			if (load.source.endsWith("xml",Qt::CaseInsensitive)){
				QByteArray data=reply->readAll();
				if(data.indexOf("<packet>")!=-1){
					load.danmaku=parseComment(data,Utils::Niconico);
				}
				else if(data.indexOf("<i>")!=-1){
					load.danmaku=parseComment(data,Utils::Bilibili);
				}
				else if(data.indexOf("<c>")!=-1){
					load.danmaku=parseComment(data,Utils::AcfunLocalizer);
				}
			}
			else{
				load.danmaku=parseComment(reply->readAll(),Utils::AcFun);
			}
			if (load.delay!=0){
				for(Comment &c:load.danmaku){
					c.time+=load.delay;
				}
			}
			Danmaku::instance()->appendToPool(&load);
			emit stateChanged(task.state=None);
			dequeue();
			break;
		}
	};
	auto directRegular=QRegularExpression("^.*\\.(xml|json)$");
	directRegular.setPatternOptions(QRegularExpression::CaseInsensitiveOption);
	pool.append({directRegular,0,directProcess});

	auto fullBiProcess=[this](QNetworkReply *reply){
		Task &task=queue.head();
		static QHash<const Task *,QSet<Comment>> progress;
		switch(task.state){
		case None:
		{
			QString api("http://comment.%1/rolldate,%2");
			api=api.arg(Utils::customUrl(Utils::Bilibili));
			task.code=QUrlQuery(task.code.mid(5)).queryItemValue("source");
			forward(QNetworkRequest(api.arg(QFileInfo(task.code).baseName())),Code);
			break;
		}
		case Code:
		{
			QMap<QDate,int> count;
			for(QJsonValue iter:QJsonDocument::fromJson(reply->readAll()).array()){
				QJsonObject obj=iter.toObject();
				QJsonValue time=obj["timestamp"],size=obj["new"];
				count[QDateTime::fromTime_t(time.toVariant().toUInt()).date()]+=size.toVariant().toInt();
			}
			if (count.isEmpty()){
				dequeue();
				return;
			}
			QString url("http://comment.%1/dmroll,%3,%2");
			url=url.arg(Utils::customUrl(Utils::Bilibili)).arg(QFileInfo(task.code).baseName());
			auto getHistory=[=](QDate date){
				return manager->get(QNetworkRequest(url.arg(QDateTime(date).toTime_t())));
			};
			emit stateChanged(task.state=File);
			QNetworkReply *header=getHistory(count.firstKey());
			connect(header,&QNetworkReply::finished,[=,&task](){
				QByteArray data=header->readAll();
				auto &record=progress[&task];
				for(const Comment &c:parseComment(data,Utils::Bilibili)){
					record.insert(c);
				}
				double total=0;
				if (count.size()>=2){
					int max=QRegularExpression("(?<=\\<max_count\\>).+(?=\\</max_count\\>)").match(data).captured().toInt();
					int now=0;
					for(auto iter=count.begin()+1;;++iter){
						now+=iter.value();
						if (iter+1==count.end()){
							remain+=getHistory(iter.key());
							break;
						}
						else if(now+(iter+1).value()>max){
							remain+=getHistory(iter.key());
							now=0;
						}
					}
					total=remain.size()+2;
					emit progressChanged(2/total);
				}
				else{
					progress.remove(&task);
					emit stateChanged(task.state=None);
					dequeue();
				}
				for(QNetworkReply *it:remain){
					connect(it,&QNetworkReply::finished,[&,this,it,total](){
						QByteArray data=it->readAll();
						for(const Comment &c:parseComment(data,Utils::Bilibili)){
							record.insert(c);
						}
						switch(it->error()){
						case QNetworkReply::NoError:
							emit progressChanged((total-remain.size())/total);
						case QNetworkReply::OperationCanceledError:
							if (remain.isEmpty()){
								Record load;
								load.full=true;
								load.danmaku=record.toList();
								load.source=task.code;
								Danmaku::instance()->appendToPool(&load);
								progress.remove(&task);
								emit stateChanged(task.state=None);
								dequeue();
							}
						}
					});
				}
			});
			break;
		}
		}
	};
	auto fullBiRegular=QRegularExpression("^full\\?source=http://comment\\.bilibili\\.com/\\d+\\.xml$");
	pool.append({fullBiRegular,5,fullBiProcess});

	auto histBiProcess=[this](QNetworkReply *reply){
		Task &task=queue.head();
		switch(task.state){
		case None:
		{
			QUrlQuery query(task.code.mid(5));
			task.code=query.queryItemValue("source");
			QString cid=QFileInfo(task.code).baseName(),url;
			QString dat=query.queryItemValue("date");
			if(dat!="0"){
				url=QString("http://comment.%1/dmroll,%2,%3");
				url=url.arg(Utils::customUrl(Utils::Bilibili));
				url=url.arg(dat).arg(cid);
			}
			else{
				url=QString("http://comment.%1/%2.xml").arg(Utils::customUrl(Utils::Bilibili));
				url=url.arg(cid);
			}
			forward(QNetworkRequest(url),File);
			break;
		}
		case File:
		{
			Record load;
			load.danmaku=parseComment(reply->readAll(),Utils::Bilibili);
			load.source=task.code;
			for(Record &iter:Danmaku::instance()->getPool()){
				if (iter.source==load.source){
					iter.danmaku.clear();
					break;
				}
			}
			Danmaku::instance()->appendToPool(&load);
			emit stateChanged(task.state=None);
			dequeue();
			break;
		}
		}
	};
	auto histBiRegular=QRegularExpression("^hist\\?source=http://comment\\.bilibili\\.com/\\d+\\.xml&date=\\d+$");
	pool.append({histBiRegular,5,histBiProcess});

	connect(this,&Load::stateChanged,[this](int code){
		switch(code){
		case None:
		case Page:
		case Part:
		case Code:
		case File:
			break;
		default:
		{
			Task task=queue.head();
			task.request=QNetworkRequest();
			task.state=None;
			QList<const Proc *> list;
			int accept=0;
			for(const Proc &i:pool){
				QRegularExpressionMatchIterator g=i.regular.globalMatch(task.code);
				while(g.hasNext()){
					QRegularExpressionMatch m=g.next();
					if (m.capturedLength()> accept){
						accept= m.capturedLength();
						list.clear();
					}
					if (m.capturedLength()==accept){
						list.append(&i);
					}
				}
			}
			std::stable_sort(list.begin(),list.end(),[](const Proc* f,const Proc *s){return f->priority>s->priority;});
			int offset=list.indexOf(task.processer)+1;
			if (offset<list.size()&&offset){
				task.processer=list[offset];
				queue.enqueue(task);
			}
			else{
				emit errorOccured(code);
			}
			break;
		}
		}
	});
}

Load::Task Load::codeToTask(QString code)
{
	Task task;
	task.processer=getProc(code);
	task.code=code;
	APlayer *aplayer=APlayer::instance();
	task.delay=aplayer->getState()!=APlayer::Stop&&Config::getValue("/Playing/Delay",false)?aplayer->getTime():0;
	return task;
}

QStandardItemModel *Load::getModel()
{
	return model;
}

void Load::addProc(const Load::Proc *proc)
{
	pool.append(*proc);
}

const Load::Proc *Load::getProc(QString &code)
{
	const Proc *p=nullptr;
	QString accept;
	for(const Proc &i:pool){
		QRegularExpressionMatchIterator g=i.regular.globalMatch(code);
		while(g.hasNext()){
			QRegularExpressionMatch m=g.next();
			if (m.capturedLength()>accept.length()||(m.capturedLength()==accept.length()&&(!p||i.priority>p->priority))){
				p=&i;
				accept=m.captured();
			}
		}
	}
	code=accept;
	return p;
}

void Load::loadDanmaku(QString code)
{
	const Task &task=codeToTask(code);
	if (task.processer){
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
		if (u.isValid()&&s.isValid()){
			Task task;
			task.code=s.toString();
			task.processer=getProc(task.code);
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

void Load::fullDanmaku(QString source)
{
	QUrlQuery query;
	query.addQueryItem("source",source);
	const Task &task=codeToTask("full?"+query.toString());
	if (task.processer){
		enqueue(task);
	}
}

void Load::loadHistory(QString source,QDate date)
{
	QUrlQuery query;
	query.addQueryItem("source",source);
	query.addQueryItem("date",QString::number(date.isValid()?QDateTime(date).toTime_t():0));
	const Task &task=codeToTask("hist?"+query.toString());
	if (task.processer){
		enqueue(task);
	}
}

void Load::dumpDanmaku(QByteArray data,int site,bool full)
{
	Task &task=queue.head();
	Record load;
	load.full=full;
	load.source=task.request.url().url();
	load.string=task.code;
	load.delay=task.delay;
	load.danmaku=parseComment(data,(Utils::Site)site);
	if (load.delay!=0){
		for(Comment &c:load.danmaku){
			c.time+=load.delay;
		}
	}
	Danmaku::instance()->appendToPool(&load);
}

Load::Task *Load::getHead()
{
	return queue.isEmpty()?nullptr:&queue.head();
}

void Load::dequeue()
{
	static bool flag;
	if (flag){
		return; 
	}
	flag=1;
	for(QNetworkReply *r:QSet<QNetworkReply *>(remain)){
		r->abort();
	}
	flag=0;
	queue.dequeue();
	if(!queue.isEmpty()){
		forward();
	}
}

bool Load::enqueue(const Task &task)
{
	for(const Task &iter:queue){
		if (iter.code==task.code){
			return false;
		}
	}
	queue.enqueue(task);
	if (queue.size()==1){
		forward();
	}
	return true;
}

void Load::forward()
{
	Task &task=queue.head();
	emit stateChanged(task.state);
	if (task.state==None){
		task.processer->process(0);
	}
	else{
		remain.insert(manager->get(task.request));
	}
}

void Load::forward(QNetworkRequest request)
{
	Task &task=queue.head();
	task.request=request;
	forward();
}

void Load::forward(QNetworkRequest request,int state)
{
	Task &task=queue.head();
	task.request=request;task.state=state;
	forward();
}
