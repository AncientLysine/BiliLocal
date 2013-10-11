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

template<class T>
class Stack
{
public:
	inline T &top()
	{
		if(isEmpty()){
			QT_THROW("Empty");
		}
		return stk.top();
	}

	inline T pop()
	{
		if(isEmpty()){
			QT_THROW("Empty");
		}
		return stk.pop();
	}

	inline void push(const T &i)
	{
		stk.push(i);
	}

	inline bool isEmpty()
	{
		return stk.isEmpty();
	}

private:
	QStack<T> stk;
};

static double evaluate(QString exp)
{
	auto Operator=[](QChar o){
		switch(o.toLatin1())
		{
		case '+':
		case '-':
		case '*':
		case '/':
			return 1;
		default:
			return 0;
		}
	};

	auto Priority=[](QChar o){
		switch(o.toLatin1())
		{
		case '(':
			return 1;
		case '+':
		case '-':
			return 2;
		case '*':
		case '/':
			return 3;
		default:
			return 0;
		}
	};

	QT_TRY{
		QString pst;
		Stack<QChar> opt;
		int i=0;
		opt.push('#');
		while(i<exp.length()){
			if(exp[i].isDigit()||exp[i]=='.'){
				pst.append(exp[i]);
			}
			else if(exp[i]=='('){
				opt.push(exp[i]);
			}
			else if(exp[i]==')'){
				while(opt.top()!='('){
					pst.append(opt.pop());
				}
				opt.pop();
			}
			else if(Operator(exp[i])){
				pst.append(' ');
				while(Priority(exp[i])<=Priority(opt.top())){
					pst.append(opt.pop());
				}
				opt.push(exp[i]);
			}
			i++;
		}
		while(!opt.isEmpty()){
			pst.append(opt.pop());
		}
		Stack<double> num;
		i=0;
		while(pst[i]!='#'){
			if(pst[i].isDigit()||pst[i]=='.'){
				double n=0;
				while(pst[i].isDigit()){
					n=n*10+pst[i++].toLatin1()-'0';
				}
				if(pst[i]=='.'){
					++i;
					double d=1;
					while(pst[i].isDigit()){
						n+=(d/=10)*(pst[i++].toLatin1()-'0');
					}
				}
				num.push(n);
			}
			else if(pst[i]==' '){
				i++;
			}
			else if(pst[i]=='+'){
				double r=num.pop(),l=num.pop();
				num.push(l+r);
				i++;
			}
			else if(pst[i]=='-'){
				double r=num.pop(),l=num.pop();
				num.push(l-r);
				i++;
			}
			else if(pst[i]=='*'){
				double r=num.pop(),l=num.pop();
				num.push(l*r);
				i++;
			}
			else if(pst[i]=='/'){
				double r=num.pop(),l=num.pop();
				num.push(l/r);
				i++;
			}
		}
		return num.top();
	}
	QT_CATCH(...){
		Printer::instance()->append(QString("[Dnamaku]error while evaluating \"%1\"").arg(exp));
		return 0;
	}
}

Danmaku::Danmaku(QObject *parent) :
	QAbstractItemModel(parent)
{
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
		if(cache[comment]){
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
			QJsonObject c;
			c["mode"]=comment.mode;
			c["font"]=comment.font;
			c["time"]=(double)comment.time;
			c["date"]=(double)comment.date;
			c["color"]=comment.color;
			c["sender"]=comment.sender;
			c["string"]=comment.string;
			return c;
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
	clearCurrent();
	pool.clear();
	danmaku.clear();
	parse(0x1|0x2);
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
		beginResetModel();
		danmaku.clear();
		for(const auto &record:pool){
			for(const auto &comment:record.danmaku){
				danmaku.append(&comment);
			}
		}
		qStableSort(danmaku.begin(),danmaku.end(),[](const Comment *f,const Comment *s){return *f<*s;});
		jumpToTime(time);
		endResetModel();
	}
	if((flag&0x2)>0){
		cache.clear();
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
				if(++count[clean[end]]>l){
					set.insert(clean[end]);
				}
				if(++end%50==0){
					qApp->processEvents();
				}
			}
		}
		for(int i=0;i<danmaku.size();++i){
			cache[*danmaku[i]]=(l==0?false:set.contains(clean[i]))||Shield::isBlocked(*danmaku[i]);
			if(i%50==0){
				qApp->processEvents();
			}
		}
	}
	emit layoutChanged();
}

void Danmaku::setSize(QSize _size)
{
	size=_size;
}

void Danmaku::setTime(qint64 _time)
{
	time=_time;
	for(;cur<danmaku.size()&&danmaku[cur]->time<time;++cur){
		const Comment &comment=*danmaku[cur];
		if(!cache[comment]){
			appendToCurrent(comment);
			qApp->processEvents();
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
		pool.append(Record(record.source,QList<Comment>(),Utils::getConfig("/Playing/Delay",false)?time:0));
		append=&pool.last();
	}
	QSet<Comment> set=append->danmaku.toSet();
	for(Comment c:record.danmaku){
		if(!set.contains(c)){
			c.time+=append->delay-record.delay;
			set.insert(c);
			append->danmaku.append(c);
		}
	}
	parse(0x1|0x2);
}

class QPixmapFilterPrivate;
class Q_WIDGETS_EXPORT QPixmapFilter : public QObject
{
	Q_OBJECT
	Q_DECLARE_PRIVATE(QPixmapFilter)
public:
	virtual ~QPixmapFilter() = 0;

	enum FilterType {
		ConvolutionFilter,
		ColorizeFilter,
		DropShadowFilter,
		BlurFilter,

		UserFilter = 1024
	};

	FilterType type() const;

	virtual QRectF boundingRectFor(const QRectF &rect) const;

	virtual void draw(QPainter *painter, const QPointF &p, const QPixmap &src, const QRectF &srcRect = QRectF()) const = 0;

protected:
	QPixmapFilter(QPixmapFilterPrivate &d, FilterType type, QObject *parent);
	QPixmapFilter(FilterType type, QObject *parent);
};
class QPixmapDropShadowFilterPrivate;
class Q_WIDGETS_EXPORT QPixmapDropShadowFilter : public QPixmapFilter
{
	Q_OBJECT
	Q_DECLARE_PRIVATE(QPixmapDropShadowFilter)

public:
	QPixmapDropShadowFilter(QObject *parent = 0);
	~QPixmapDropShadowFilter();

	QRectF boundingRectFor(const QRectF &rect) const;
	void draw(QPainter *p, const QPointF &pos, const QPixmap &px, const QRectF &src = QRectF()) const;

	qreal blurRadius() const;
	void setBlurRadius(qreal radius);

	QColor color() const;
	void setColor(const QColor &color);

	QPointF offset() const;
	void setOffset(const QPointF &offset);
	inline void setOffset(qreal dx, qreal dy) { setOffset(QPointF(dx, dy)); }
};

void Danmaku::appendToCurrent(const Comment &comment)
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

	int l=Utils::getConfig("/Shield/Density",80);
	if(comment.mode==1&&l!=0&&current[0].size()>l){
		return;
	}
	QFont font;
	font.setBold(Utils::getConfig("/Danmaku/Effect",5)%2);
	font.setFamily(Utils::getConfig("/Danmaku/Font",QFont().family()));
	font.setPixelSize(Utils::getConfig("/Danmaku/Scale",1.0)*comment.font);
	QStaticText text;
	text.setText(QString(comment.string).replace("/n","\n").replace("\n","<br>"));
	text.prepare(QTransform(),font);
	QSize bound=text.size().toSize()+QSize(4,4);
	bool sub=Utils::getConfig("/Danmaku/Protect",false),flag=false;
	Static render;
	switch(comment.mode){
	case 1:
	{
		QString exp=Utils::getConfig<QString>("/Danmaku/Speed","125+%{width}/5");
		exp.replace("%{width}",QString::number(bound.width()),Qt::CaseInsensitive);
		if((render.speed=evaluate(exp))==0){
			break;
		}
		render.rect=QRectF(QPointF(0,0),bound);
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
	case 4:
	{
		QString exp=Utils::getConfig<QString>("/Danmaku/Life","5");
		exp.replace("%{width}",QString::number(bound.width()),Qt::CaseInsensitive);
		if((render.life=evaluate(exp))==0){
			break;
		}
		render.rect=QRectF(QPointF(0,0),bound);
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
	case 5:
	{
		QString exp=Utils::getConfig<QString>("/Danmaku/Life","5");
		exp.replace("%{width}",QString::number(bound.width()),Qt::CaseInsensitive);
		if((render.life=evaluate(exp))==0){
			break;
		}
		render.rect=QRectF(QPointF(0,0),bound);
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
		QPixmap fst(bound);
		fst.fill(Qt::transparent);
		QPainter painter;
		painter.begin(&fst);
		painter.setFont(font);
		auto draw=[&](QColor c,QPoint p){
			painter.setPen(c);
			painter.drawStaticText(p+=QPoint(2,2),text);
		};
		int color=comment.color;
		QColor edge=qGray(color)<30?Qt::white:Qt::black;
		switch(Utils::getConfig("/Danmaku/Effect",5)/2){
		case 0:
			draw(edge,QPoint(+1,0));
			draw(edge,QPoint(-1,0));
			draw(edge,QPoint(0,+1));
			draw(edge,QPoint(0,-1));
			draw(color,QPoint(0,0));
			break;
		case 1:
			draw(edge,QPoint(2,2));
			draw(edge,QPoint(1,1));
			draw(color,QPoint(0,0));
			break;
		case 2:
		{
			draw(color,QPoint(0,0));
			painter.end();
			QPixmap src;
			QPixmapDropShadowFilter f(this);
			f.setColor(edge);
			f.setOffset(0,0);
			f.setBlurRadius(4);
			src=fst;
			fst.fill(Qt::transparent);
			painter.begin(&fst);
			f.draw(&painter,QPointF(),src);
			painter.end();
			src=fst;
			fst.fill(Qt::transparent);
			painter.begin(&fst);
			f.draw(&painter,QPointF(),src);
			break;
		}
		}
		painter.end();
		QPixmap sec(bound);
		sec.fill(Qt::transparent);
		painter.begin(&sec);
		painter.setOpacity(Utils::getConfig("/Danmaku/Alpha",1.0));
		painter.drawPixmap(QPoint(0,0),fst);
		painter.end();
		render.text=sec;
		current[comment.mode-1].append(render);
	}
}
