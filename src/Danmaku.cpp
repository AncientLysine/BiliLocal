/*=======================================================================
*
*   Copyright (C) 2013 Lysine.
*
*   Filename:    Danmaku.cpp
*   Time:        2013/03/18
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

#include "Danmaku.h"

Danmaku *Danmaku::ins=NULL;

Danmaku::Danmaku(QObject *parent) :
	QAbstractItemModel(parent)
{
	setSize(QSize(960,540));
	cur=time=0;
	ins=this;
	qsrand(QTime::currentTime().msec());
}

void Danmaku::draw(QPainter *painter,bool move)
{
	qint64 etime=0;
	if(move&&!last.isNull()){
		etime=last.elapsed();
		etime=etime>50?0:etime;
	}
	last.start();
	for(int index=0;index<5;++index){
		for(auto iter=current[index].begin();iter!=current[index].end();){
			Static &render=*iter;
			bool invalid=false;
			switch(index){
			case 0:
				render.rect.moveLeft(render.rect.left()-render.speed*etime/1000.0);
				if(render.rect.right()<0){
					invalid=true;
				}
				break;
			case 3:
			case 4:
				render.life-=etime/1000.0;
				if(render.life<=0){
					invalid=true;
				}
			}
			if(invalid){
				iter=current[index].erase(iter);
			}
			else{
				++iter;
				painter->drawPixmap(render.rect.topLeft(),render.text);
			}
		}
	}
}

QVariant Danmaku::data(const QModelIndex &index,int role) const
{
	if(index.isValid()){
		const Comment &comment=*danmaku[index.row()];
		if(Shield::isBlocked(comment)){
			if(index.column()==0){
				if(role==Qt::DisplayRole){
					return tr("Blocked");
				}
				if(role==Qt::ForegroundRole){
					return QColor(Qt::red);
				}
			}
			if(index.column()==1){
				if(role==Qt::DisplayRole){
					return comment.string;
				}
				if(role==Qt::ForegroundRole){
					return QColor(Qt::gray);
				}
			}
		}
		else{
			if(index.column()==0&&role==Qt::DisplayRole){
				QString time("%1:%2");
				qint64 sec=comment.time/1000;
				if(sec<0){
					time.prepend("-");
					sec=-sec;
				}
				time=time.arg(sec/60,2,10,QChar('0'));
				time=time.arg(sec%60,2,10,QChar('0'));
				return time;
			}
			if(index.column()==1&&role==Qt::DisplayRole){
				return comment.string;
			}

		}
		if(index.column()==1&&role==Qt::ToolTipRole){
			return comment.string;
		}
		if(index.column()==0&&role==Qt::TextAlignmentRole){
			return Qt::AlignCenter;
		}
		if(role==Qt::UserRole){
			return comment.sender;
		}
	}
	return QVariant();
}

int Danmaku::rowCount(const QModelIndex &parent) const
{
	return parent.isValid()?0:danmaku.size();
}

int Danmaku::columnCount(const QModelIndex &parent) const
{
	return parent.isValid()?0:2;
}

QModelIndex Danmaku::parent(const QModelIndex &) const
{
	return QModelIndex();
}

QModelIndex Danmaku::index(int row,int colum,const QModelIndex &parent) const
{
	if(!parent.isValid()&&colum<2){
		return createIndex(row,colum);
	}
	return QModelIndex();
}

QVariant Danmaku::headerData(int section,Qt::Orientation orientation,int role) const
{
	if(role==Qt::DisplayRole&&orientation==Qt::Horizontal){
		if(section==0){
			return tr("Time");
		}
		if(section==1){
			return tr("Comment");
		}
	}
	return QVariant();
}

void Danmaku::resetTime()
{
	cur=0;
	time=0;
}

void Danmaku::clearPool()
{
	pool.clear();
	danmaku.clear();
	Shield::cacheS.clear();
	emit layoutChanged();
}

void Danmaku::clearCurrent()
{
	for(QList<Static> &iter:current){
		iter.clear();
	}
}

void Danmaku::parse(int flag)
{
	if((flag&0x1)>0){
		qSort(danmaku.begin(),danmaku.end(),[](const Comment *f,const Comment *s){return *f<*s;});
		jumpToTime(time);
	}
	if((flag&0x2)>0){
		Shield::shieldC.clear();
		int l=Utils::getConfig("/Shield/Limit",5);
		if(l!=0){
			QHash<QString,int> c;
			for(const Comment *com:danmaku){
				QString clean=com->string;
				clean.remove(QRegExp("\\W"));
				c[clean]=c.value(clean,0)+1;
			}
			for(const QString &k:c.keys()){
				if(!k.isEmpty()&&c[k]>l){
					Shield::shieldC.append(k);
				}
			}
		}
	}
	emit layoutChanged();
}

void Danmaku::setDm(QString dm)
{
	auto bi=[this](const QByteArray &data,QList<Comment> &list){
		QStringList l=QString(data).simplified().split("<d p=\"");
		l.removeFirst();
		qint64 delay=Utils::getConfig("/Playing/Delay",false)?time:0;
		for(QString &item:l){
			Comment comment;
			int sta=0;
			int len=item.indexOf("\"");
			QStringList args=item.mid(sta,len).split(',');
			sta=item.indexOf(">")+1;
			len=item.indexOf("<",sta)-sta;
			comment.time=args[0].toDouble()*1000+delay;
			comment.date=args[4].toInt();
			comment.mode=args[1].toInt();
			comment.font=args[2].toInt();
			comment.color=args[3].toInt();
			comment.sender=args[6];
			comment.string=item.mid(sta,len);
			if(!list.contains(comment)){
				list.append(comment);
				danmaku.append(&list.last());
			}
		}
	};

	auto ac=[this](const QByteArray &data,QList<Comment> &list){
		QJsonArray a=QJsonDocument::fromJson(data).array();
		qint64 delay=Utils::getConfig("/Playing/Delay",false)?time:0;
		for(QJsonValue i:a){
			Comment comment;
			QJsonObject item=i.toObject();
			QStringList args=item["c"].toString().split(',');
			comment.time=args[0].toDouble()*1000+delay;
			comment.date=args[5].toInt();
			comment.mode=args[2].toInt();
			comment.font=args[3].toInt();
			comment.color=args[1].toInt();
			comment.sender=args[4];
			comment.string=item["m"].toString();
			if(!list.contains(comment)){
				list.append(comment);
				danmaku.append(&list.last());
			}
		}
	};

	if(Utils::getConfig("/Playing/Clear",true)){
		clearPool();
	}
	int sharp=dm.indexOf("#");
	QString s=dm.mid(0,2);
	QString i=dm.mid(2,sharp-2);
	QString p=sharp==-1?QString():dm.mid(sharp+1);
	if(s=="av"||s=="ac"){
		QString api;
		if(s=="av"){
			api="http://www.bilibili.tv/video/av%1/index_%2.html";
		}
		if(s=="ac"){
			api="http://www.acfun.tv/v/ac%1_%2";
		}
		QUrl apiUrl(api.arg(i).arg(p.isEmpty()?"1":p));
		QNetworkAccessManager *manager=new QNetworkAccessManager(this);
		manager->get(QNetworkRequest(apiUrl));
		connect(manager,&QNetworkAccessManager::finished,[=](QNetworkReply *reply){
			auto error=[this](int code){
				QString info=tr("Network error occurred, error code: %1");
				QMessageBox::warning(NULL,tr("Network Error"),info.arg(code));
			};
			QString url=reply->url().url();
			if(reply->error()==QNetworkReply::NoError){
				if(url.startsWith("http://comment.")){
					if(s=="av"){
						bi(reply->readAll(),pool[url].danmaku);
					}
					if(s=="ac"){
						ac(reply->readAll(),pool[url].danmaku);
					}
					parse(0x1|0x2);
					reply->manager()->deleteLater();
				}
				else if(url.startsWith("http://www.bilibili.tv/")||url.startsWith("http://comic.letv.com/")){
					QUrl redirect=reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
					if(redirect.isValid()){
						reply->manager()->get(QNetworkRequest(redirect));
					}
					else{
						QString api,id,video(reply->readAll());
						QRegExp regex;
						regex.setCaseSensitivity(Qt::CaseInsensitive);
						regex.setPattern("cid\\=\\d+");
						if(regex.indexIn(video)==-1){
							regex.setPattern("cid:'\\d+");
							if(regex.indexIn(video)==-1){
								error(404);
							}
							else{
								id=regex.cap().mid(5);
							}
						}
						else{
							id=regex.cap().mid(4);
						}
						if(!id.isEmpty()){
							api="http://comment.bilibili.tv/%1.xml";
							reply->manager()->get(QNetworkRequest(QUrl(api.arg(id))));
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
						QString api,id,video(reply->readAll());
						QRegExp regex;
						regex.setCaseSensitivity(Qt::CaseInsensitive);
						regex.setPattern("\\d+(?=\\[/video\\])");
						if(regex.indexIn(video)==-1){
							regex.setPattern("id\\=\\w+");
							regex.indexIn(video,video.indexOf("<embed"));
							id=regex.cap().mid(3);
							api="http://comment.acfun.tv/%1.json";
						}
						else{
							id=regex.cap();
							api="http://www.acfun.tv/api/player/vids/%1.aspx";
						}
						reply->manager()->get(QNetworkRequest(QUrl(api.arg(id))));
					}
				}
			}
			else{
				error(reply->error());
			}
		});
	}
	else{
		QFile file(dm);
		if(file.exists()){
			file.open(QIODevice::ReadOnly);
			if(dm.endsWith("xml")){
				bi(file.readAll(),pool[dm].danmaku);
			}
			if(dm.endsWith("json")){
				ac(file.readAll(),pool[dm].danmaku);
			}
			parse(0x1|0x2);
		}
	}
}

void Danmaku::setSize(QSize _size)
{
	size=_size;
}

void Danmaku::setTime(qint64 _time)
{
	auto intersects=[](const Static &first,const Static &second){
		if(first.rect.intersects(second.rect)){
			return true;
		}
		int ft=first.rect.top();
		int fb=first.rect.bottom();
		int st=second.rect.top();
		int sb=second.rect.bottom();
		bool sameHeight=false;
		if(st>=ft&&st<=fb){
			sameHeight=true;
		}
		if(sb<=fb&&sb>=ft){
			sameHeight=true;
		}
		if(st<=ft&&sb>=fb){
			sameHeight=true;
		}
		if(sameHeight){
			int fr,sl;
			fr=first.rect.right()/first.speed;
			sl=second.rect.left()/second.speed;
			return fr>=sl;
		}
		else{
			return false;
		}
	};
	time=_time;
	for(;cur<danmaku.size()&&danmaku[cur]->time<time;++cur){
		const Comment &comment=*danmaku[cur];
		if(Shield::isBlocked(comment)){
			continue;
		}
		int l=Utils::getConfig("/Shield/Density",80);
		if(comment.mode==1&&l!=0){
			QEasingCurve c(QEasingCurve::InExpo);
			qreal r=qrand()%1000;
			qreal v=1000*c.valueForProgress(((qreal)current[0].size())/l);
			if(r<v){
				continue;
			}
		}
		QCoreApplication::processEvents();
		QFont font;
		font.setBold(true);
		font.setFamily(Utils::getConfig("/Danmaku/Font",QFont().family()));
		font.setPixelSize(comment.font*Utils::getConfig("/Danmaku/Scale",1.0));
		QStaticText text;
		text.setText(QString(comment.string).replace("/n","<br>"));
		text.prepare(QTransform(),font);
		QSize textSize=text.size().toSize()+QSize(2,2);
		bool flag=false,sub=Utils::getConfig("/Danmaku/Protect",false);
		Static render;
		switch(comment.mode-1){
		case 0:
		{
			QString exp=Utils::getConfig<QString>("/Danmaku/Speed","125+%{width}/5");
			exp.replace("%{width}",QString::number(textSize.width()),Qt::CaseInsensitive);
			render.speed=engine.evaluate(exp).toNumber();
			render.rect=QRectF(QPointF(0,0),textSize);
			render.rect.moveLeft(size.width());
			int limit=size.height()-(sub?80:0)-render.rect.height();
			for(int height=5;height<limit;height+=10){
				flag=true;
				render.rect.moveTop(height);
				for(Static &iter:current[0]){
					if(intersects(iter,render)){
						flag=false;
						break;
					}
				}
				if(flag){
					break;
				}
			}
			break;
		}
		case 3:
		{
			render.life=Utils::getConfig("/Danmaku/Life",5.0);
			render.rect=QRectF(QPointF(0,0),textSize);
			render.rect.moveCenter(QPoint(size.width()/2,0));
			int limit=render.rect.height();
			for(int height=size.height()-(sub?size.height()/10:5);height>limit;height-=10){
				flag=true;
				render.rect.moveBottom(height);
				for(Static &iter:current[3]){
					if(iter.rect.intersects(render.rect)){
						flag=false;
						break;
					}
				}
				if(flag){
					break;
				}
			}
			break;
		}
		case 4:
		{
			render.life=Utils::getConfig("/Danmaku/Life",5.0);
			render.rect=QRectF(QPointF(0,0),textSize);
			render.rect.moveCenter(QPoint(size.width()/2,0));
			int limit=size.height()-(sub?80:0)-render.rect.height();
			for(int height=5;height<limit;height+=10){
				flag=true;
				render.rect.moveTop(height);
				for(Static &iter:current[4]){
					if(iter.rect.intersects(render.rect)){
						flag=false;
						break;
					}
				}
				if(flag){
					break;
				}
			}
			break;
		}
		}
		if(flag){
			QPixmap fst(textSize);
			fst.fill(Qt::transparent);
			QPainter painter;
			painter.begin(&fst);
			painter.setFont(font);
			auto draw=[&](QColor c,QPoint p){
				painter.setPen(c);
				painter.drawStaticText(p+=QPoint(1,1),text);
			};
			QColor edge=qGray(comment.color)<50?Qt::white:Qt::black;
			draw(edge,QPoint(+1,0));
			draw(edge,QPoint(-1,0));
			draw(edge,QPoint(0,+1));
			draw(edge,QPoint(0,-1));
			draw(comment.color,QPoint(0,0));
			painter.end();
			QPixmap sec(textSize);
			sec.fill(Qt::transparent);
			painter.begin(&sec);
			painter.setOpacity(Utils::getConfig("/Danmaku/Alpha",1.0));
			painter.drawPixmap(QPoint(0,0),fst);
			painter.end();
			render.text=sec;
			current[comment.mode-1].append(render);
		}
	}
}

void Danmaku::jumpToTime(qint64 _time)
{
	clearCurrent();
	time=_time;
	for(cur=0;cur<danmaku.size()&&danmaku[cur]->time<time;++cur);
}

void Danmaku::saveToFile(QString _file)
{
	QFile f(_file);
	f.open(QIODevice::WriteOnly|QIODevice::Text);
	QJsonArray a;
	for(const Comment *c:danmaku){
		QJsonObject o;
		QStringList l;
		l<<QString::number(c->time/1000.0);
		l<<QString::number(c->color);
		l<<QString::number(c->mode);
		l<<QString::number(c->font);
		l<<c->sender;
		l<<QString::number(c->date);
		o["c"]=l.join(',');
		o["m"]=c->string;
		a.append(o);
	}
	f.write(QJsonDocument(a).toJson());
	f.close();
}
