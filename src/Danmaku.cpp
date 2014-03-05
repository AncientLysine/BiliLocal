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
#include "Shield.h"
#include "Render.h"
#include "VPlayer.h"
#include "Graphic.h"

Danmaku *Danmaku::ins=NULL;

class RenderEvent:public QEvent
{
public:
	static Type registeredType;
	RenderEvent():QEvent(registeredType){}
};

QEvent::Type RenderEvent::registeredType=(QEvent::Type)registerEventType();

Danmaku *Danmaku::create(QObject *parent)
{
	return new Danmaku(parent);
}

Danmaku::Danmaku(QObject *parent):
	QAbstractItemModel(parent)
{
	cur=time=0;
	ins=this;
	connect(VPlayer::instance(),&VPlayer::jumped,this,&Danmaku::jumpToTime);
	connect(VPlayer::instance(),&VPlayer::timeChanged,this,&Danmaku::setTime);
}

void Danmaku::draw(QPainter *painter,QRect,qint64 move)
{
	for(auto iter=current.begin();iter!=current.end();){
		Graphic *g=*iter;
		if(g->move(move)){
			g->draw(painter);
			++iter;
		}
		else{
			delete g;
			iter=current.erase(iter);
		}
	}
}

QVariant Danmaku::data(const QModelIndex &index,int role) const
{
	if(index.isValid()){
		const Comment &comment=*danmaku[index.row()];
		if(comment.blocked){
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
				return QString(comment.string).remove("\n");
			}
		}
		if(index.column()==1&&role==Qt::ToolTipRole){
			return Qt::convertFromPlainText(comment.string);
		}
		if(index.column()==0&&role==Qt::TextAlignmentRole){
			return Qt::AlignCenter;
		}
		if(role==Qt::UserRole){
			return (quintptr)&comment;
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

const Comment *Danmaku::commentAt(QPoint point) const
{
	for(Graphic *g:current){
		if(g->currentRect().contains(point)) return g->getSource();
	}
	return NULL;
}

void Danmaku::release()
{
	buffer.clear();
	disconnect(VPlayer::instance(),&VPlayer::timeChanged,this,&Danmaku::setTime);
}

void Danmaku::resetTime()
{
	cur=0;
	time=0;
}

void Danmaku::clearPool()
{
	clearCurrent();
	pool.clear();
	danmaku.clear();
	parse(0x1|0x2);
}

void Danmaku::clearCurrent()
{
	buffer.clear();
	qDeleteAll(current);
	current.clear();
	emit layoutChanged();
}

void Danmaku::parse(int flag)
{
	if((flag&0x1)>0){
		beginResetModel();
		danmaku.clear();
		for(Record &record:pool){
			for(Comment &comment:record.danmaku){
				comment.string.replace("/n","\n");
				danmaku.append(&comment);
			}
		}
		std::stable_sort(danmaku.begin(),danmaku.end(),[](const Comment *f,const Comment *s){return f->time<s->time;});
		jumpToTime(time);
		endResetModel();
	}
	if((flag&0x2)>0){
		for(Record &r:pool){
			for(Comment &c:r.danmaku){
				c.blocked=r.limit!=0&&c.date>r.limit;
			}
		}
		QSet<QString> set;
		int l=Utils::getConfig("/Shield/Limit",5);
		QVector<QString> clean;
		if(l!=0){
			QRegExp r("\\W");
			for(const Comment *c:danmaku){
				clean.append(QString(c->string).remove(r));
			}
			QHash<QString,int> count;
			int sta=0,end=sta;
			while(end!=danmaku.size()){
				while(danmaku[sta]->time+10000<danmaku[end]->time){
					if(--count[clean[sta]]==0){
						count.remove(clean[sta]);
					}
					++sta;
				}
				if(++count[clean[end]]>l&&danmaku[end]->mode<=6){
					set.insert(clean[end]);
				}
				++end;
			}
		}
		for(int i=0;i<danmaku.size();++i){
			Comment &c=*danmaku[i];
			c.blocked=c.blocked||(l==0?false:set.contains(clean[i]))||Shield::isBlocked(c);
		}
		for(auto iter=current.begin();iter!=current.end();){
			const Comment *cur=(*iter)->getSource();
			if(cur&&cur->blocked){
				delete *iter;
				iter=current.erase(iter);
			}
			else{
				++iter;
			}
		}
		emit layoutChanged();
	}
}

void Danmaku::setTime(qint64 _time)
{
	time=_time;
	int l=Utils::getConfig("/Shield/Density",100);
	for(;cur<danmaku.size()&&danmaku[cur]->time<time;++cur){
		const Comment *c=danmaku[cur];
		if(!c->blocked&&(c->mode==7||l==0||current.size()+buffer.size()<l)){
			appendToCurrent(c);
		}
	}
	qApp->postEvent(this,new RenderEvent);
}

bool Danmaku::event(QEvent *e)
{
	if(e->type()==RenderEvent::registeredType){
		processDanmakuInBuffer();
		return true;
	}
	else{
		return QAbstractItemModel::event(e);
	}
}

void Danmaku::processDanmakuInBuffer()
{
	while(!buffer.isEmpty()){
		QList<Graphic *> waiting;
		if(time-buffer.first()->time>2000){
			while(!buffer.isEmpty()&&time-buffer.first()->time>500){
				buffer.removeFirst();
			}
		}
		const Comment *f=buffer.first();
		while(!buffer.isEmpty()&&buffer.first()->time==f->time&&buffer.first()->string==f->string){
			Graphic *g=Graphic::create(*buffer.takeFirst(),Render::instance()->getWidget()->size(),current);
			if(g){
				g->setEnabled(false);
				current.append(g);
				waiting.append(g);
				qApp->removePostedEvents(this,RenderEvent::registeredType);
				qApp->processEvents();
			}
		}
		for(Graphic *g:waiting){
			g->setEnabled(true);
		}
	}
	qApp->removePostedEvents(this,RenderEvent::registeredType);
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
	if(_file.endsWith("xml",Qt::CaseInsensitive)){
		QXmlStreamWriter w(&f);
		w.setAutoFormatting(true);
		w.writeStartDocument();
		w.writeStartElement("i");
		w.writeStartElement("chatserver");
		w.writeCharacters("chat.bilibili.tv");
		w.writeEndElement();
		w.writeStartElement("mission");
		w.writeCharacters("0");
		w.writeEndElement();
		w.writeStartElement("source");
		w.writeCharacters("k-v");
		w.writeEndElement();
		for(const Comment *c:danmaku){
			w.writeStartElement("d");
			QStringList l;
			l<<QString::number(c->time/1000.0)<<
			   QString::number(c->mode)<<
			   QString::number(c->font)<<
			   QString::number(c->color)<<
			   QString::number(c->date)<<
			   "0"<<
			   c->sender<<
			   "0";
			w.writeAttribute("p",l.join(','));
			w.writeCharacters(c->string);
			w.writeEndElement();
		}
		w.writeEndElement();
		w.writeEndDocument();
	}
	else{
		QJsonArray a;
		for(const Comment *c:danmaku){
			QJsonObject o;
			QStringList l;
			l<<QString::number(c->time/1000.0)<<
			   QString::number(c->color)<<
			   QString::number(c->mode)<<
			   QString::number(c->font)<<
			   c->sender<<
			   QString::number(c->date);
			o["c"]=l.join(',');
			o["m"]=c->string;
			a.append(o);
		}
		f.write(QJsonDocument(a).toJson(QJsonDocument::Compact));
	}
	f.close();
}

void Danmaku::appendToPool(const Record &record)
{
	Record *append=NULL;
	for(Record &r:pool){
		if(r.source==record.source){
			append=&r;
			break;
		}
	}
	if(append==NULL){
		Record r;
		r.source=record.source;
		r.delay=Utils::getConfig("/Playing/Delay",false)?time:0;
		pool.append(r);
		append=&pool.last();
	}
	const auto &d=append->danmaku;
	QSet<Comment> set=d.toSet();
	for(Comment c:record.danmaku){
		if(!set.contains(c)){
			c.time+=append->delay-record.delay;
			set.insert(c);
			append->danmaku.append(c);
		}
	}
	if(record.full){
		append->full=true;
	}
	parse(0x1|0x2);
}

void Danmaku::appendToCurrent(const Comment *comment,bool isLocal)
{
	if(isLocal){
		Graphic *graphic=Graphic::create(*comment,Render::instance()->getWidget()->size(),current);
		if(graphic){
			graphic->setSource(NULL);
			current.append(graphic);
		}
	}
	else{
		buffer.append(comment);
	}
}
