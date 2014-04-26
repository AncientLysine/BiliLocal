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
#include "Config.h"
#include "VPlayer.h"
#include "Graphic.h"

#define qThreadPool QThreadPool::globalInstance()

Danmaku *Danmaku::ins=NULL;

Danmaku *Danmaku::instance()
{
	return ins?ins:new Danmaku(qApp);
}

Danmaku::Danmaku(QObject *parent):
	QAbstractItemModel(parent)
{
	cur=time=0;
	ins=this;
	QThreadPool::globalInstance()->setMaxThreadCount(Config::getValue("/Danmaku/Thread",QThread::idealThreadCount()));
	connect(VPlayer::instance(),&VPlayer::jumped,this,&Danmaku::jumpToTime);
	connect(VPlayer::instance(),&VPlayer::timeChanged,this,&Danmaku::setTime);
}

void Danmaku::draw(QPainter *painter,QRect rect,qint64 move)
{
	size=rect.size();
	QVector<Graphic *> dirty;
	lock.lockForWrite();
	for(auto iter=current.begin();iter!=current.end();){
		Graphic *g=*iter;
		if(g->move(move)){
			dirty.append(g);
			++iter;
		}
		else{
			delete g;
			iter=current.erase(iter);
		}
	}
	lock.unlock();
	for(Graphic *g:dirty){
		g->draw(painter);
	}
}

QVariant Danmaku::data(const QModelIndex &index,int role) const
{
	if(index.isValid()){
		const Comment &comment=*danmaku[index.row()];
		switch(role){
		case Qt::DisplayRole:
			if(index.column()==0){
				if(comment.blocked){
					return tr("Blocked");
				}
				else{
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
			}
			else{
				if(comment.mode==7){
					QJsonArray data=QJsonDocument::fromJson(comment.string.toUtf8()).array();
					return data.size()>=5?data.at(4).toString():QString();
				}
				else{
					return QString(comment.string).remove('\n');
				}
			}
		case Qt::ForegroundRole:
			if(index.column()==0){
				if(comment.blocked){
					return QColor(Qt::red);
				}
			}
			else{
				if(comment.blocked){
					return QColor(Qt::gray);
				}
			}
			break;
		case Qt::ToolTipRole:
			if(index.column()==1){
				return Qt::convertFromPlainText(comment.string);
			}
			break;
		case Qt::TextAlignmentRole:
			if(index.column()==0){
				return Qt::AlignCenter;
			}
			break;
		case Qt::BackgroundRole:
			switch(comment.mode){
			case 7:
				return QColor(200,255,200);
			case 8:
				return QColor(255,255,160);
			default:
				break;
			}
		case Qt::UserRole:
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
	lock.lockForRead();
	for(Graphic *g:current){
		if(g->currentRect().contains(point)){
			lock.unlock();
			return g->getSource();
		}
	}
	lock.unlock();
	return NULL;
}

void Danmaku::release()
{
	disconnect(VPlayer::instance(),&VPlayer::timeChanged,this,&Danmaku::setTime);
	clearCurrent();
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
	qThreadPool->clear();
	qThreadPool->waitForDone();
	lock.lockForWrite();
	qDeleteAll(current);
	current.clear();
	lock.unlock();
	emit layoutChanged();
}

namespace
{
class Compare
{
public:
	inline bool operator ()(const Comment *c,qint64 time)
	{
		return c->time<time;
	}
	inline bool operator ()(qint64 time,const Comment *c)
	{
		return time<c->time;
	}
	inline bool operator ()(const Comment *f,const Comment *s)
	{
		return f->time<s->time;
	}
};
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
		std::stable_sort(danmaku.begin(),danmaku.end(),Compare());
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
		int l=Config::getValue("/Shield/Limit",5);
		QVector<QString> clean;
		clean.reserve(danmaku.size());
		if(l!=0){
			QRegularExpression r("\\W");
			r.setPatternOptions(QRegularExpression::UseUnicodePropertiesOption);
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
		qThreadPool->clear();
		qThreadPool->waitForDone();
		lock.lockForWrite();
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
		lock.unlock();
		emit layoutChanged();
	}
}

static quint64 globalIndex=0;

class Process:public QRunnable
{
public:
	Process(QSize &s,QReadWriteLock *l,QList<Graphic *> &c,const QList<const Comment *> &w):
		size(s),lock(l),current(c),wait(w)
	{
	}

	void run()
	{
		if(wait.isEmpty()||wait.first()->time<VPlayer::instance()->getTime()-500){
			return;
		}
		QList<Graphic *> ready;
		while(!wait.isEmpty()){
			const Comment *c=wait.takeFirst();
			Graphic *g=Graphic::create(*c,size);
			if(g){
				if(c->mode<=6&&c->font*(c->string.count("\n")+1)<360){
					int b=0,e=0,s=0;
					QRectF &r=g->currentRect();
					std::function<bool(int,int)> f;
					switch(c->mode){
					case 1:
					case 5:
					case 6:
						b=r.top();
						e=size.height()*(Config::getValue("/Danmaku/Protect",false)?0.85:1)-r.height();
						s=10;
						f=std::less_equal<int>();
						break;
					case 4:
						b=r.top();
						e=0;
						s=-10;
						f=std::greater_equal<int>();
						break;
					}
					QVector<uint> result(qMax((e-b)/s+1,0),0);
					auto calculate=[&](const QList<Graphic *> &data){
						QRectF t=r;
						int i=0,h=b;
						for(;f(h,e);h+=s,++i){
							r.moveTop(h);
							for(Graphic *iter:data){
								result[i]+=g->intersects(iter);
							}
						}
						r=t;
					};
					lock->lockForRead();
					quint64 lastIndex=current.isEmpty()?0:current.last()->getIndex();
					calculate(current);
					lock->unlock();
					g->setEnabled(false);
					ready.append(g);
					lock->lockForWrite();
					QList<Graphic *> addtion;
					QListIterator<Graphic *> iter(current);
					iter.toBack();
					while(iter.hasPrevious()){
						Graphic *p=iter.previous();
						if(p->getIndex()>lastIndex){
							addtion.prepend(p);
						}
						else break;
					}
					calculate(addtion);
					uint m=UINT_MAX;
					int i=0,h=b;
					for(;f(h,e);h+=s,++i){
						if(m>result[i]){
							r.moveTop(h);
							if(m==0){
								break;
							}
							m=result[i];
						}
					}
				}
				else{
					g->setEnabled(false);
					ready.append(g);
					lock->lockForWrite();
				}
				g->setIndex(globalIndex++);
				current.append(g);
				lock->unlock();
			}
		}
		lock->lockForWrite();
		for(Graphic *g:ready){
			g->setEnabled(true);
		}
		lock->unlock();
	}

private:
	QSize &size;
	QReadWriteLock *lock;
	QList<Graphic *> &current;
	QList<const Comment *> wait;
};

void Danmaku::setTime(qint64 _time)
{
	time=_time;
	int l=Config::getValue("/Shield/Density",100),n=0;
	QMap<qint64,QHash<QString,QList<const Comment *>>> buffer;
	for(;cur<danmaku.size()&&danmaku[cur]->time<time;++cur){
		const Comment *c=danmaku[cur];
		if(!c->blocked&&(c->mode==7||l==0||current.size()+n<l)){
			++n;
			buffer[c->time][c->string].append(c);
		}
	}
	for(const auto &sameTime:buffer){
		for(const auto &sameText:sameTime){
			qThreadPool->start(new Process(size,&lock,current,sameText));
		}
	}
}

void Danmaku::delayAll(qint64 _time)
{
	for(Record &r:pool){
		r.delay+=_time;
		for(Comment &c:r.danmaku){
			c.time+=_time;
		}
	}
	jumpToTime(time);
	emit layoutChanged();
}

void Danmaku::jumpToTime(qint64 _time)
{
	clearCurrent();
	time=_time;
	cur=std::lower_bound(danmaku.begin(),danmaku.end(),time,Compare())-danmaku.begin();
}

static void saveToSingleFile(QString _file,const QList<const Comment *> &data)
{
	QFile f(_file);
	f.open(QIODevice::WriteOnly|QIODevice::Text);
	bool skip=Config::getValue("/Interface/Save/Skip",false);
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
		for(const Comment *c:data){
			if(c->blocked&&skip){
				continue;
			}
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
		for(const Comment *c:data){
			if(c->blocked&&skip){
				continue;
			}
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

void Danmaku::saveToFile(QString _file)
{
	QList<const Comment *> d;
	if(Config::getValue("/Interface/Save/Single",true)){
		for(const Comment *c:danmaku){
			d.append(c);
		}
		saveToSingleFile(_file,d);
	}
	else{
		QFileInfo info(_file);
		for(const Record &r:pool){
			QString s=QFileInfo(r.string).suffix().toLower();
			if(s=="xml"||s=="json"){
				continue;
			}
			d.clear();
			for(const Comment &c:r.danmaku){
				d.append(&c);
			}
			saveToSingleFile(QFileInfo(info.dir(),"["+r.string+"]"+info.fileName()).absoluteFilePath(),d);
		}
	}
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
		r.string=record.string;
		r.delay=Config::getValue("/Playing/Delay",false)?time:0;
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

bool Danmaku::appendToPool(QString source,const Comment &comment)
{
	for(Record &r:pool){
		if(r.source==source){
			r.danmaku.append(comment);
			Comment *c=&r.danmaku.last();
			danmaku.insert(std::upper_bound(danmaku.begin(),danmaku.end(),c,Compare()),c);
			parse(0x2);
			return true;
		}
	}
	return false;
}
