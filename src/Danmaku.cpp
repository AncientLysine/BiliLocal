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
	QObject(parent)
{
	setSize(QSize(960,540));
	currentIndex=0;
	alpha=1;
	delay=0;
#ifdef Q_OS_LINUX
	font.setFamily("文泉驿正黑");
#endif
#ifdef Q_OS_WIN
	font.setFamily("黑体");
#endif
	sub=false;
	QJsonObject shield=Utils::getConfig("Shield");
	for(const auto &item:shield["User"].toArray()){
		shieldU.append(item.toString());
	}
	for(const auto &item:shield["Regex"].toArray()){
		shieldR.append(QRegExp(item.toString()));
	}
}

Danmaku::~Danmaku()
{
	QJsonObject shield;
	QJsonArray u,r;
	for(auto &item:shieldU){
		u.append(item);
	}
	for(auto &item:shieldR){
		r.append(item.pattern());
	}
	shield.insert("User",u);
	shield.insert("Regex",r);
	Utils::setConfig(shield,"Shield");
}

void Danmaku::draw(QPainter *painter,bool move)
{
	qint64 etime;
	if(move){
		if(last.isNull()){
			etime=40;
			last.start();
		}
		else{
			etime=last.restart();
		}
	}
	else{
		etime=0;
	}
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

void Danmaku::reset()
{
	for(auto &pool:current){
		pool.clear();
	}
	currentIndex=0;
	setLast();
}

void Danmaku::setLast()
{
	last=QTime();
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
			bool flag=true;
			QString user=args[6];
			for(QString &name:shieldU){
				if(name==user){
					flag=false;
					break;
				}
			}
			sta=item.indexOf(">")+1;
			len=item.indexOf("<",sta)-sta;
			comment.content=item.mid(sta,len);
			for(QRegExp &reg:shieldR){
				if(reg.indexIn(comment.content)!=-1){
					flag=false;
					break;
				}
			}
			if(flag){
				comment.time=args[0].toDouble()*1000;
				comment.mode=args[1].toInt();
				comment.font=args[2].toInt();
				comment.color.setRgb(args[3].toInt());
				danmaku.append(comment);
			}
		}
	};

	auto ac=[this](QByteArray data){
		QJsonArray array=QJsonDocument::fromJson(data).array();
		for(QJsonValue _item:array){
			QJsonObject item=_item.toObject();
			Comment comment;
			bool flag=true;
			comment.content=item["m"].toString();
			for(QRegExp &reg:shieldR){
				if(reg.indexIn(comment.content)!=-1){
					flag=false;
					break;
				}
			}
			QStringList args=item["c"].toString().split(',');
			if(flag){
				comment.time=args[0].toDouble()*1000;
				comment.color.setRgb(args[1].toInt());
				comment.mode=args[2].toInt();
				comment.font=args[3].toInt();
				danmaku.append(comment);
			}
		}
	};

	auto sort=[this](){
		qSort(danmaku.begin(),danmaku.end(),[](const Comment &first,const Comment &second){
			return first.time<second.time;
		});
		currentIndex=0;
		emit loaded();
	};

	danmaku.clear();
	reset();
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
			auto error=[this](int code){
				QWidget *p=dynamic_cast<QWidget *>(this->parent());
				QString info=tr("Network error occurred, error code: %1");
				QMessageBox::warning(p,tr("Network Error"),info.arg(code));
			};
			qDebug()<<reply->url().url();
			if(reply->error()==QNetworkReply::NoError){
				QString url=reply->url().url();
				if(url.startsWith("http://api.bilibili.tv/")){
					QJsonObject json=QJsonDocument::fromJson(reply->readAll()).object();
					if(json.contains("cid")){
						QString api="http://comment.bilibili.tv/%1.xml";
						QUrl xmlUrl(api.arg(json["cid"].toDouble()));
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
							QUrl jsonUrl(api.arg(json["cid"].toString().toInt()));
							reply->manager()->get(QNetworkRequest(jsonUrl));
						}
						else{
							error(404);
						}
					}
					else{
						QRegExp regex("(?!\\[video\\])[0-9]+(?=\\[/video\\])");
						QString page=QString::fromUtf8(reply->readAll()).simplified();
						regex.indexIn(page);
						QString api="http://www.acfun.tv/api/player/vids/%1.aspx";
						reply->manager()->get(QNetworkRequest(QUrl(api.arg(regex.cap()))));
					}

				}
				else{
					if(s=="av"){
						bi(reply->readAll());
					}
					if(s=="ac"){
						ac(reply->readAll());
					}
					sort();
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
			sort();
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
	for(;currentIndex<danmaku.size()&&danmaku[currentIndex].time+delay<time;++currentIndex){
		QCoreApplication::processEvents();
		Comment &comment=danmaku[currentIndex];
		Static render;
		font.setBold(true);
		font.setPixelSize(comment.font);
		QStaticText text(comment.content);
		text.prepare(QTransform(),font);
		QSize textSize=text.size().toSize()+QSize(2,2);
		bool flag=false;
		switch(comment.mode-1){
		case 0:
		{
			render.speed=100+textSize.width()/5.0+qrand()%50;
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
			render.life=5;
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
			render.life=5;
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
			QImage temp(text.size().toSize()+=QSize(2,2),QImage::Format_ARGB32);
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

void Danmaku::setFont(QString _font)
{
	font.setFamily(_font);
}

void Danmaku::setAlpha(double _alpha)
{
	alpha=_alpha;
}

void Danmaku::setDelay(qint64 _delay)
{
	delay=_delay;
}

void Danmaku::setProtect(bool enabled)
{
	sub=enabled;
}

void Danmaku::jumpToTime(qint64 time)
{
	for(currentIndex=0;currentIndex<danmaku.size()&&danmaku[currentIndex].time+delay<time;++currentIndex);
}
