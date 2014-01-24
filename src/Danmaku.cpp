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
#include "VPlayer.h"
#include "Graphic.h"
#include <unordered_set>

Danmaku *Danmaku::ins=NULL;

Danmaku::Danmaku(QObject *parent) :
	QAbstractItemModel(parent)
{
	cur=time=0;
	ins=this;
	connect(VPlayer::instance(),&VPlayer::jumped,this,&Danmaku::jumpToTime);
	connect(VPlayer::instance(),&VPlayer::timeChanged,this,&Danmaku::setTime);
}

void Danmaku::draw(QPainter *painter,QRect rect,qint64 move)
{
	size=rect.size();
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
			return Utils::splitString(comment.string,400);
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
	for(Graphic *iter:current){
		delete iter;
	}
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
		qStableSort(danmaku.begin(),danmaku.end(),[](const Comment *f,const Comment *s){return f->time<s->time;});
		jumpToTime(time);
		endResetModel();
	}
	if((flag&0x2)>0){
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
				if(++end%50==0){
					qApp->processEvents();
				}
			}
		}
		for(int i=0;i<danmaku.size();++i){
			danmaku[i]->blocked=(l==0?false:set.contains(clean[i]))||Shield::isBlocked(*danmaku[i]);
			if(i%50==0){
				qApp->processEvents();
			}
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
	QList<const Comment *> buffer;
	for(;cur<danmaku.size()&&danmaku[cur]->time<time;++cur){
		buffer.append(danmaku[cur]);
	}
	appendToCurrent(buffer);
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

namespace std
{
template<>
class hash<Comment>
{
public:
	inline uint operator ()(const Comment &c) const
	{
		uint h=qHash(c.mode);
		h=(h<<1)^qHash(c.font);
		h=(h<<1)^qHash(c.color);
		h=(h<<1)^qHash(c.time);
		h=(h<<1)^qHash(c.date);
		h=(h<<1)^qHash(c.sender);
		h=(h<<1)^qHash(c.string);
		return h;
	}
};
template<>
class equal_to<Comment>
{
public:
	inline bool operator ()(const Comment &f,const Comment &s) const
	{
		return f.mode==s.mode&&f.font==s.font&&f.color==s.color&&f.sender==s.sender&&f.string==s.string&&f.time==s.time&&f.date==s.date;
	}
};
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
	std::unordered_set<Comment> set(d.begin(),d.end());
	for(Comment c:record.danmaku){
		if(set.count(c)==0){
			c.time+=append->delay-record.delay;
			set.insert(c);
			append->danmaku.append(c);
		}
	}
	parse(0x1|0x2);
}

Graphic *Danmaku::render(const Comment &comment)
{
	Graphic *graphic=NULL;
	switch(comment.mode){
	case 1:
		graphic=new Mode1(comment,current,size);
		break;
	case 4:
		graphic=new Mode4(comment,current,size);
		break;
	case 5:
		graphic=new Mode5(comment,current,size);
		break;
	case 6:
		graphic=new Mode6(comment,current,size);
		break;
	case 7:
		graphic=new Mode7(comment,current,size);
		break;
	}
	if(graphic!=NULL&&!graphic->isEnabled()){
		delete graphic;
		return NULL;
	}
	return graphic;
}

void Danmaku::appendToCurrent(const Comment *comment,bool isLocal)
{
	int l=Utils::getConfig("/Shield/Density",100);
	if(!comment->blocked&&(comment->mode==7||l==0||current.size()<l)){
		Graphic *graphic=render(*comment);
		if(graphic){
			if(isLocal){
				graphic->setSource(NULL);
			}
			current.append(graphic);
		}
	}
}

void Danmaku::appendToCurrent(const QList<const Comment *> &comments,bool isLocal)
{
	QList<Graphic *> waiting;
	int l=Utils::getConfig("/Shield/Density",100);
	for(const Comment *comment:comments){
		if(!comment->blocked&&(comment->mode==7||l==0||current.size()+waiting.size()<l)){
			Graphic *graphic=render(*comment);
			if(graphic){
				if(isLocal){
					graphic->setSource(NULL);
				}
				graphic->setEnabled(false);
				waiting.append(graphic);
				current.append(graphic);
			}
			qApp->processEvents();
		}
	}
	for(Graphic *graphic:waiting){
		graphic->setEnabled(true);
	}
}
