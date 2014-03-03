/*=======================================================================
*
*   Copyright (C) 2013 Lysine.
*
*   Filename:    Editor.cpp
*   Time:        2013/06/30
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

#include "Editor.h"
#include "Cookie.h"
#include "Danmaku.h"
#include "VPlayer.h"
#include "History.h"

namespace{
class Resizer:public QObject
{
public:
	Resizer(QWidget *src,QWidget *dst):
		QObject(src),src(src),dst(dst)
	{
		src->installEventFilter(this);
	}

	bool eventFilter(QObject *o, QEvent *e)
	{
		if(e->type()==QEvent::Resize){
			dst->resize(src->width(),dst->height());
			return false;
		}
		else{
			return QObject::eventFilter(o,e);
		}
	}

private:
	QWidget *src;
	QWidget *dst;
};
}

int Editor::exec(QWidget *parent)
{
	QDialog dialog(parent);
	auto layout=new QGridLayout(&dialog);
	auto scroll=new QScrollArea(&dialog);
	auto widget=new Editor(&dialog);
	new Resizer(scroll->viewport(),widget);
	layout->addWidget(scroll);
	scroll->setWidget(widget);
	scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	dialog.resize(640,450);
	dialog.setMinimumSize(300,200);
	dialog.setWindowTitle(tr("Editor"));
	Utils::setCenter(&dialog);
	return dialog.exec();
}

static QMap<QDate,int> parseCount(QByteArray data)
{
	QMap<QDate,int> count;
	for(QJsonValue iter:QJsonDocument::fromJson(data).array()){
		QJsonObject obj=iter.toObject();
		QJsonValue time=obj["timestamp"],size=obj["new"];
		count[QDateTime::fromTime_t(time.toVariant().toInt()).date()]+=size.toVariant().toInt();
	}
	return count;
}

Editor::Editor(QWidget *parent):
	QWidget(parent)
{
	scale=0;
	length=100;
	current=VPlayer::instance()->getTime();
	manager=new QNetworkAccessManager(this);
	manager->setCookieJar(Cookie::instance());
	Cookie::instance()->setParent(NULL);
	load();
	setContextMenuPolicy(Qt::CustomContextMenu);
	connect(this,&Editor::customContextMenuRequested,[this](QPoint p){
		int i=p.y()/length;
		Record &r=Danmaku::instance()->getPool()[i];
		QMenu menu(this);
		if(!r.full){
			connect(menu.addAction(tr("Full")),&QAction::triggered,[this,&r](){
				QString cid=QFileInfo(r.source).baseName();
				QString api("http://comment.bilibili.tv/rolldate,%1");
				Utils::getReply(manager,QNetworkRequest(api.arg(cid)),[=,&r](QNetworkReply *reply){
					QMap<QDate,int> count=parseCount(reply->readAll());
					if(count.isEmpty()){
						return;
					}
					QString url("http://comment.bilibili.tv/dmroll,%2,%1");
					url=url.arg(cid);
					QSet<Comment> set;
					QProgressDialog progress(this);
					progress.setFixedSize(progress.sizeHint());
					progress.setWindowTitle(tr("Loading"));
					QNetworkAccessManager *manager=new QNetworkAccessManager(this);
					manager->setCookieJar(Cookie::instance());
					Cookie::instance()->setParent(NULL);
					connect(manager,&QNetworkAccessManager::finished,[&](QNetworkReply *reply){
						QStringList s=reply->url().url().split(',');
						if(s.size()==3){
							QDate c=QDateTime::fromTime_t(s[1].toUInt()).date();
							QByteArray data=reply->readAll();
							if(c==count.lastKey()&&count.size()>=2){
								int sta=data.indexOf("<max_count>")+11,end=data.indexOf("</max_count>");
								int max=data.mid(sta,end-sta).toInt(),now=0,all=0;
								for(auto iter=--count.end();;--iter){
									now+=iter.value();
									if(now>max||iter==count.begin()){
										now=iter.value();
										++all;
										manager->get(QNetworkRequest(url.arg(QDateTime(iter.key()).toTime_t())));
										if(iter==count.begin()){
											break;
										}
									}
								}
								progress.setMaximum(all);
							}
							for(const Comment &c:Utils::parseComment(data,Utils::Bilibili,true)){
								set.insert(c);
							}
							progress.setValue(progress.value()+1);
						}
						reply->deleteLater();
					});
					manager->get(QNetworkRequest(url.arg(QDateTime(count.lastKey()).toTime_t())));
					progress.exec();
					delete manager;
					Record load;
					load.full=true;
					load.danmaku=set.toList();
					load.source=r.source;
					Danmaku::instance()->appendToPool(load);
				});
			});
			menu.addSeparator();
		}
		connect(menu.addAction(tr("History")),&QAction::triggered,[=,&r](){
			History history(this);
			QMap<QDate,int> count;
			QDate c=r.limit==0?QDate::currentDate().addDays(1):QDateTime::fromTime_t(r.limit).date();
			history.setCurrentDate(c);
			if(r.full){
				for(const Comment &c:r.danmaku){
					++count[QDateTime::fromTime_t(c.date).date()];
				}
				count[QDate::currentDate().addDays(1)]=0;
				count.remove(count.firstKey());
				history.setCount(count);
				history.setCurrentPage(c);
			}
			else{
				QString cid=QFileInfo(r.source).baseName();
				QString api("http://comment.bilibili.tv/rolldate,%1");
				Utils::getReply(manager,QNetworkRequest(api.arg(cid)),[&](QNetworkReply *reply){
					count=parseCount(reply->readAll());
					count[QDate::currentDate().addDays(1)]=0;
					history.setCount(count);
					history.setCurrentPage(c);
				});
			}
			if(history.exec()==QDialog::Accepted){
				QDate selected=history.selectedDate();
				r.limit=selected.isValid()?QDateTime(selected).toTime_t():0;
				if(r.full){
					Danmaku::instance()->parse(0x2);
					update(0,i*length,width(),length);
				}
				else{
					QString url,cid=QFileInfo(r.source).baseName();;
					QDate selected=history.selectedDate();
					if(selected.isValid()){
						url=QString("http://comment.bilibili.tv/dmroll,%1,%2").arg(QDateTime(selected).toTime_t()).arg(cid);
					}
					else{
						url=QString("http://comment.bilibili.tv/%1.xml").arg(cid);
					}
					Utils::getReply(manager,QNetworkRequest(url),[=,&r](QNetworkReply *reply){
						r.danmaku.clear();
						Record load;
						load.full=false;
						load.danmaku=Utils::parseComment(reply->readAll(),Utils::Bilibili,true);
						load.source=r.source;
						Danmaku::instance()->appendToPool(load);
					});
				}
			}
		});
		connect(menu.addAction(tr("Delete")),&QAction::triggered,[this,i](){
			auto &p=Danmaku::instance()->getPool();
			p.removeAt(i);
			Danmaku::instance()->parse(0x1|0x2);
		});
		menu.exec(mapToGlobal(p));
	});
	connect(Danmaku::instance(),&Danmaku::modelReset,this,&Editor::load);
}

void Editor::load()
{
	duration=VPlayer::instance()->getDuration();
	const QList<Record> &pool=Danmaku::instance()->getPool();
	for(QLineEdit *iter:time){
		delete iter;
	}
	time.clear();
	for(int i=0;i<pool.size();++i){
		const Record &line=pool[i];
		bool f=true;
		qint64 t;
		for(const Comment &c:line.danmaku){
			t=f?c.time:qMax(c.time,t);
			f=false;
		}
		if(!f){
			duration=qMax(duration,t-line.delay);
		}
		QLineEdit *edit=new QLineEdit(tr("Delay: %1s").arg(line.delay/1000),this);
		edit->setGeometry(0,73+i*length,98,25);
		edit->setFrame(false);
		edit->setAlignment(Qt::AlignCenter);
		edit->show();
		connect(edit,&QLineEdit::editingFinished,[this,edit](){
			int index=time.indexOf(edit);
			QString expression=QRegularExpression("-?[\\d\\.\\+\\-*/()]+").match(edit->text()).captured();
			const Record &r=Danmaku::instance()->getPool()[index];
			if(!expression.isEmpty()){
				delayRecord(index,Utils::evaluate(expression)*1000-r.delay);
			}
			else{
				edit->setText(tr("Delay: %1s").arg(r.delay/1000));
			}
		});
		time.append(edit);
	}
	magnet<<0<<current<<duration;
	QRect rect=geometry();
	resize(width(),pool.count()*length);
	update();
	parentWidget()->update(rect);
}

void Editor::paintEvent(QPaintEvent *e)
{
	QPainter painter;
	painter.begin(this);
	painter.fillRect(rect(),Qt::gray);
	int l=e->rect().bottom()/length+1;
	int s=point.isNull()?-1:point.y()/length;
	for(int i=e->rect().top()/length;i<l;++i){
		const Record &r=Danmaku::instance()->getPool()[i];
		int w=width()-100,h=i*length;
		painter.fillRect(0,h,100-2,length-2,Qt::white);
		painter.drawText(0,h,100-2,length-25,Qt::AlignCenter|Qt::TextWordWrap,QFileInfo(r.source).fileName());
		int m=0,d=duration/(w/5)+1;
		QHash<int,int> c;
		for(const Comment &com:r.danmaku){
			if(com.blocked){
				continue;
			}
			int k=(com.time-r.delay)/d,v=c.value(k,0)+1;
			c.insert(k,v);
			m=v>m?v:m;
		}
		if(m==0){
			continue;
		}
		int o=w*r.delay/duration;
		if(i==s){
			o+=mapFromGlobal(QCursor::pos()).x()-point.x();
			for(qint64 p:magnet){
				p=p*w/duration;
				if(qAbs(o-p)<5){
					o=p;
					break;
				}
			}
		}
		painter.setClipRect(100,h,w,length);
		for(int j=0;j<w;j+=5){
			int he=c[j/5]*(length-2)/m;
			painter.fillRect(o+j+100,h+length-2-he,5,he,Qt::white);
		}
		painter.setClipping(false);
	}
	if(current>=0){
		painter.fillRect(current*(width()-100)/duration+100,0,1,height(),Qt::red);
	}
	painter.end();
	QWidget::paintEvent(e);
}

void Editor::wheelEvent(QWheelEvent *e)
{
	int i=e->pos().y()/length;
	scale+=e->angleDelta().y();
	if(qAbs(scale)>=120){
		auto &r=Danmaku::instance()->getPool()[i];
		qint64 d=(r.delay/1000)*1000;
		d+=scale>0?-1000:1000;
		d-=r.delay;
		delayRecord(i,d);
		scale=0;
	}
	QWidget::wheelEvent(e);
	e->accept();
}

void Editor::mouseMoveEvent(QMouseEvent *e)
{
	if(point.isNull()){
		point=e->pos();
	}
	else{
		update(100,point.y()/length*length,width()-100,length);
	}
}

void Editor::mouseReleaseEvent(QMouseEvent *e)
{
	if(!point.isNull()){
		int w=width()-100;
		auto &r=Danmaku::instance()->getPool()[point.y()/length];
		qint64 d=(e->x()-point.x())*duration/w;
		for(qint64 p:magnet){
			if(qAbs(d+r.delay-p)<duration/(w/5)+1){
				d=p-r.delay;
				break;
			}
		}
		delayRecord(point.y()/length,d);
	}
	point=QPoint();
}

void Editor::delayRecord(int index,qint64 delay)
{
	auto &r=Danmaku::instance()->getPool()[index];
	r.delay+=delay;
	for(Comment &c:r.danmaku){
		c.time+=delay;
	}
	update(0,index*length,width(),length);
	time[index]->setText(tr("Delay: %1s").arg(r.delay/1000));
}
