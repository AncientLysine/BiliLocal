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

Danmaku::Danmaku(QObject *parent) :
	QAbstractItemModel(parent)
{
	setSize(QSize(960,540));
	cur=0;
	delay=0;
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
		const Comment &comment=danmaku[index.row()];
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
					return comment.content;
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
				time=time.arg(sec/60,2,10,QChar('0'));
				time=time.arg(sec%60,2,10,QChar('0'));
				return time;
			}
			if(index.column()==1&&role==Qt::DisplayRole){
				return comment.content;
			}

		}
		if(index.column()==1&&role==Qt::ToolTipRole){
			return comment.content;
		}
		if(index.column()==0&&role==Qt::TextAlignmentRole){
			return Qt::AlignCenter;
		}
		if(role==Qt::UserRole){
			QVariant variant;
			variant.setValue(comment);
			return variant;
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

bool Danmaku::removeRows(int row,int count,const QModelIndex &parent)
{
	if(!parent.isValid()){
		if(row+count<=danmaku.size()){
			danmaku.remove(row,count);
			if(danmaku.isEmpty()){
				cid.clear();
			}
			emit layoutChanged();
			return true;
		}
	}
	return false;
}

void Danmaku::clearCurrent()
{
	for(auto &pool:current){
		pool.clear();
	}
	cur=0;
}

void Danmaku::setDm(QString dm)
{
	auto bi=[this](QByteArray data){
		QString xml=QString(data).simplified();
		QStringList list=xml.split("<d p=\"");
		list.removeFirst();
		for(QString &item:list){
			Comment comment;
			int sta=0;
			int len=item.indexOf("\"");
			QStringList args=item.mid(sta,len).split(',');
			sta=item.indexOf(">")+1;
			len=item.indexOf("<",sta)-sta;
			comment.content=item.mid(sta,len);
			comment.time=args[0].toDouble()*1000;
			comment.mode=args[1].toInt();
			comment.font=args[2].toInt();
			comment.color.setRgb(args[3].toInt());
			comment.sender=args[6];
			danmaku.append(comment);
		}
	};

	auto ac=[this](QByteArray data){
		QJsonArray array=QJsonDocument::fromJson(data).array();
		for(QJsonValue _item:array){
			QJsonObject item=_item.toObject();
			Comment comment;
			comment.content=item["m"].toString();
			QStringList args=item["c"].toString().split(',');
			comment.time=args[0].toDouble()*1000;
			comment.color.setRgb(args[1].toInt());
			comment.mode=args[2].toInt();
			comment.font=args[3].toInt();
			danmaku.append(comment);
		}
	};

	auto init=[this](){
		qSort(danmaku.begin(),danmaku.end(),[](const Comment &f,const Comment &s){
			return f.time==s.time?f.content<s.content:f.time<s.time;
		});
		cur=0;
		int l=Utils::getConfig("/Shield/Limit",0);
		if(l!=0){
			QHash<QString,int> c;
			for(const Comment &com:danmaku){
				c[com.content]=c.value(com.content,0)+1;
			}
			Shield::shieldC.clear();
			for(const QString &k:c.keys()){
				if(c[k]>l){
					Shield::shieldC.append(k);
				}
			}
		}
		emit layoutChanged();
	};

	if(Utils::getConfig("/Danmaku/Clear",true)){
		danmaku.clear();
		emit layoutChanged();
	}
	int sharp=dm.indexOf("#");
	QString s=dm.mid(0,2);
	QString i=dm.mid(2,sharp-2);
	QString p=sharp==-1?QString():dm.mid(sharp+1);
	if(s=="av"||s=="ac"){
		QString api;
		if(s=="av"){
			api="http://api.bilibili.tv/view?type=json&appkey=0&id=%1&page=%2";
		}
		if(s=="ac"){
			api="http://www.acfun.tv/v/ac%1_%2";
		}
		QUrl apiUrl(api.arg(i).arg(p.isEmpty()?"1":p));
		QNetworkAccessManager *manager=new QNetworkAccessManager(this);
		manager->get(QNetworkRequest(apiUrl));
		connect(manager,&QNetworkAccessManager::finished,[=](QNetworkReply *reply){
			QString url=reply->url().url();
			auto error=[this](int code){
				QString info=tr("Network error occurred, error code: %1");
				QMessageBox::warning(NULL,tr("Network Error"),info.arg(code));
			};
			if(reply->error()==QNetworkReply::NoError){
				if(url.startsWith("http://api.bilibili.tv/")){
					QJsonObject json=QJsonDocument::fromJson(reply->readAll()).object();
					if(json.contains("cid")){
						QString api="http://comment.bilibili.tv/%1.xml";
						cid["Bilibili"]=QString::number(json["cid"].toDouble());
						QUrl xmlUrl(api.arg(cid["Bilibili"]));
						reply->manager()->get(QNetworkRequest(xmlUrl));
					}
					else{
						error(-json["code"].toDouble());
					}
				}
				else if(url.startsWith("http://www.acfun.tv/")){
					if(url.endsWith(".aspx")){
						QJsonObject json=QJsonDocument::fromJson(reply->readAll()).object();
						if(json.contains("cid")){
							QString api="http://comment.acfun.tv/%1.json";
							cid["Acfun"]=json["cid"].toString();
							QUrl jsonUrl(api.arg(cid["Acfun"]));
							reply->manager()->get(QNetworkRequest(jsonUrl));
						}
						else{
							error(404);
						}
					}
					else{
						QString video=QString::fromUtf8(reply->readAll()),api,id;
						QRegExp regex;
						regex.setCaseSensitivity(Qt::CaseInsensitive);
						regex.setPattern("[0-9]+(?=\\[/video\\])");
						regex.indexIn(video);
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
				else{
					if(s=="av"){
						bi(reply->readAll());
					}
					if(s=="ac"){
						ac(reply->readAll());
					}
					init();
					emit loaded();
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
				bi(file.readAll());
			}
			if(dm.endsWith("json")){
				ac(file.readAll());
			}
			init();
			emit loaded();
		}
	}
}

void Danmaku::setTime(qint64 time)
{
	qsrand(time);
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
	for(;cur<danmaku.size()&&danmaku[cur].time+delay<time;++cur){
		QCoreApplication::processEvents();
		const Comment &comment=danmaku[cur];
		if(Shield::isBlocked(comment)){
			continue;
		}
		Static render;
		QFont font;
		font.setBold(true);
		font.setFamily(Utils::getConfig<QString>("/Danmaku/Font"));
		font.setPixelSize(comment.font*Utils::getConfig("/Danmaku/Scale",1.0));
		QStaticText text(comment.content);
		text.prepare(QTransform(),font);
		QSize textSize=text.size().toSize()+QSize(2,2);
		bool flag=false,sub=Utils::getConfig("/Danmaku/Protect",false);
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
			QImage temp(textSize,QImage::Format_ARGB32);
			temp.fill(Qt::transparent);
			QPainter painter;
			painter.begin(&temp);
			painter.setFont(font);
			auto draw=[&](QColor c,QPoint p){
				painter.setPen(c);
				painter.drawStaticText(p+=QPoint(1,1),text);
			};
			QColor edge=qGray(comment.color.rgb())<50?Qt::white:Qt::black;
			draw(edge,QPoint(+1,0));
			draw(edge,QPoint(-1,0));
			draw(edge,QPoint(0,+1));
			draw(edge,QPoint(0,-1));
			draw(comment.color,QPoint(0,0));
			painter.end();
			double alpha=Utils::getConfig("/Danmaku/Alpha",1.0);
			if(alpha!=1){
				int w=temp.width();
				int h=temp.height();
				uchar *bits=temp.bits();
				for(int y=0;y<h;++y){
					for(int x=0;x<w;++x){
						bits[(y*w+x)*4+3]*=alpha;
					}
				}
			}
			render.text=QPixmap::fromImage(temp);
			current[comment.mode-1].append(render);
		}
	}
}

void Danmaku::setSize(QSize _size)
{
	size=_size;
}

void Danmaku::setDelay(qint64 _delay)
{
	delay=_delay;
}

void Danmaku::jumpToTime(qint64 time)
{
	for(cur=0;cur<danmaku.size()&&danmaku[cur].time+delay<time;++cur);
}
