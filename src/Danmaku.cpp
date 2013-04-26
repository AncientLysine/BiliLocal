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
	QFile shield("./Shield.txt");
	int cur=-1;
	if(shield.exists()){
		shield.open(QIODevice::ReadOnly|QIODevice::Text);
		QTextStream stream(&shield);
		QString line;
		while(!stream.atEnd()){
			line=stream.readLine();
			if(line=="[User]"){
				cur=0;
				continue;
			}
			if(line=="[Regex]"){
				cur=1;
				continue;
			}
			if(line=="[String]"){
				cur=2;
				continue;
			}
			if(!line.isEmpty()){
				switch(cur){
				case 0:
					shieldU.append(line);
					break;
				case 1:
					shieldR.append(QRegExp(line));
					break;
				case 2:
					shieldS.append(line);
					break;
				}
			}
		}
		shield.close();
	}
	else{
		shield.open(QIODevice::WriteOnly|QIODevice::Text);
		QTextStream stream(&shield);
		stream<<"[User]"<<endl<<endl<<
				"[Regex]"<<endl<<endl<<
				"[String]"<<endl;
		shield.close();
	}
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
	auto load=[this](QString xml){
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
			comment.time=args[0].toDouble()*1000;
			comment.mode=args[1].toInt();
			comment.font=args[2].toInt();
			comment.color.setRgb(args[3].toInt());
			sta=item.indexOf(">")+1;
			len=item.indexOf("<",sta)-sta;
			comment.content=item.mid(sta,len);
			for(QString &text:shieldS){
				if(comment.content.indexOf(text,Qt::CaseInsensitive)!=-1){
					flag=false;
					break;
				}
			}
			for(QRegExp &reg:shieldR){
				if(reg.indexIn(comment.content)!=-1){
					flag=false;
					break;
				}
			}
			if(flag){
				danmaku.append(comment);
			}
		}
		qSort(danmaku.begin(),danmaku.end(),[](const Comment &first,const Comment &second){
			return first.time<second.time;
		});
		currentIndex=0;
		emit loaded();
	};
	danmaku.clear();
	reset();
	if(dm.startsWith("av")){
		int sharp=dm.indexOf("#");
		QString api="http://api.bilibili.tv/view?type=xml&appkey=0&id=%1&page=%2";
		QUrl apiUrl(api.arg(dm.mid(2,sharp-2)).arg(sharp>0?dm.mid(sharp+1):QString("1")));
		QNetworkAccessManager *manager=new QNetworkAccessManager(this);
		manager->get(QNetworkRequest(apiUrl));
		connect(manager,&QNetworkAccessManager::finished,[=](QNetworkReply *reply){
			if(reply->error()!=QNetworkReply::NoError){
				QString info=tr("Network error occurred, error code: %1");
				info.arg(reply->error());
				QWidget *p=dynamic_cast<QWidget *>(this->parent());
				QMessageBox::warning(p,tr("Network Error"),info);
			}
			else if(reply->url().url().indexOf("api")!=-1){
				QString page(reply->readAll());
				QRegExp regexp("(?!\\<cid\\>)[0-9]*(?=\\</cid\\>)");
				regexp.indexIn(page);
				QUrl xmlUrl("http://comment.bilibili.tv/"+regexp.cap()+".xml");
				manager->get(QNetworkRequest(xmlUrl));
			}
			else{
				load(QString(reply->readAll()).simplified());
			}
		});
	}
	else{
		QFile file(dm);
		if(file.exists()){
			file.open(QIODevice::ReadOnly);
			load(QString(file.readAll()).simplified());
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
			render.speed=100+textSize.width()/5.0+qrand()%50;
			render.rect=QRectF(QPointF(0,0),textSize);
			render.rect.moveLeft(size.width());
			for(int height=5;height<size.height()-(sub?80:0);height+=10){
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
		case 3:
			render.life=5;
			render.rect=QRectF(QPointF(0,0),textSize);
			render.rect.moveCenter(QPoint(size.width()/2,0));
			for(int height=size.height()-(sub?size.height()/10:5);height>0;height-=10){
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
		case 4:
			render.life=5;
			render.rect=QRectF(QPointF(0,0),textSize);
			render.rect.moveCenter(QPoint(size.width()/2,0));
			for(int height=5;height<size.height()-(sub?80:0);height+=10){
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
			if(alpha<1){
				quint32 alpha_255=(0xFFFFFFFF>>8)|(((quint32)(255*alpha))<<24);
				for(int y=0;y<temp.height();++y){
					for(int x=0;x<temp.width();++x){
						temp.setPixel(x,y,temp.pixel(x,y)&alpha_255);
					}
				}
			}
			render.text=QPixmap::fromImage(temp);
			current[comment.mode-1].append(render);
			QCoreApplication::processEvents();
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
