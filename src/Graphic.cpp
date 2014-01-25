/*=======================================================================
*
*   Copyright (C) 2013 Lysine.
*
*   Filename:    Graphic.cpp
*   Time:        2013/10/19
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

#include "Graphic.h"

namespace{
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
		return 0;
	}
}
}

class Mode1:public Graphic
{
public:
	Mode1(const Comment &comment,const QList<Graphic *> &current,const QSize &size);
	bool move(qint64 time);
	void draw(QPainter *painter);
	uint intersects(Graphic *other);
	QRectF currentRect(){return rect;}

private:
	QRectF rect;
	double speed;
	QPixmap cache;
};

class Mode4:public Graphic
{
public:
	Mode4(const Comment &comment,const QList<Graphic *> &current,const QSize &size);
	bool move(qint64 time);
	void draw(QPainter *painter);
	uint intersects(Graphic *other);
	QRectF currentRect(){return rect;}

private:
	QRectF rect;
	double life;
	QPixmap cache;
};

class Mode5:public Graphic
{
public:
	Mode5(const Comment &comment,const QList<Graphic *> &current,const QSize &size);
	bool move(qint64 time);
	void draw(QPainter *painter);
	uint intersects(Graphic *other);
	QRectF currentRect(){return rect;}

private:
	QRectF rect;
	double life;
	QPixmap cache;
};

class Mode6:public Graphic
{
public:
	Mode6(const Comment &comment,const QList<Graphic *> &current,const QSize &size);
	bool move(qint64 time);
	void draw(QPainter *painter);
	uint intersects(Graphic *other);
	QRectF currentRect(){return rect;}

private:
	QRectF rect;
	double speed;
	QPixmap cache;
	const QSize &size;
};

class Mode7:public Graphic
{
public:
	Mode7(const Comment &comment,const QList<Graphic *> &current,const QSize &size);
	bool move(qint64 time);
	void draw(QPainter *painter);
	uint intersects(Graphic *other);

private:
	QPointF bPos;
	QPointF ePos;
	double bAlpha;
	double eAlpha;
	double zRotate;
	double yRotate;
	QPixmap cache;
	double wait;
	double stay;
	double life;
	double time;
};

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

static QFont getFont(int pixelSize,QString family=Utils::getConfig("/Danmaku/Font",QFont().family()))
{
	QFont font;
	font.setBold(Utils::getConfig("/Danmaku/Effect",5)%2);
	font.setFamily(family);
	font.setPixelSize(pixelSize);
	return font;
}

static QSize getSize(QString string,QFont font)
{
	QStringList lines=string.split('\n');
	for(QString &line:lines){
		QChar h=' ',f(0x3000);
		int hc=line.count(h),fc=line.count(f);
		line.remove(h).prepend(QString(hc,h));
		line.remove(f).prepend(QString(fc,f));
	}
	return QFontMetrics(font).size(0,lines.join('\n'))+QSize(4,4);
}

static QSizeF getPlayer(qint64 date)
{
	if(date<=1384099200){
		return QSizeF(545,388);
	}
	else{
		return QSizeF(672,438);
	}
}

static double getScale(int mode,qint64 date,QSize size)
{
	int scale=Utils::getConfig("/Danmaku/Scale",0x1);
	if(mode<=6&&(scale&0x2)==0) return 1;
	if(mode==7&&(scale&0x1)==0) return 0;
	QSizeF player=getPlayer(date);
	return qMin(size.width()/player.width(),size.height()/player.height());
}

static QPixmap getCache(QString string,
						int color,
						QFont font,
						QSize size,
						int effect=Utils::getConfig("/Danmaku/Effect",5)/2,
						int opacity=Utils::getConfig("/Danmaku/Alpha",100))
{
	QPixmap fst(size);
	fst.fill(Qt::transparent);
	QPainter painter;
	painter.begin(&fst);
	painter.setFont(font);
	auto draw=[&](QColor c,QPoint p){
		QRect r(p+QPoint(2,2),size-QSize(4,4));
		painter.setPen(c);
		painter.drawText(r,string);
	};
	int base=color;
	QColor edge=qGray(base)<30?Qt::white:Qt::black;
	switch(effect){
	case 0:
		draw(edge,QPoint(+1,0));
		draw(edge,QPoint(-1,0));
		draw(edge,QPoint(0,+1));
		draw(edge,QPoint(0,-1));
		draw(base,QPoint(0,0));
		break;
	case 1:
		draw(edge,QPoint(2,2));
		draw(edge,QPoint(1,1));
		draw(base,QPoint(0,0));
		break;
	case 2:
	{
		draw(base,QPoint(0,0));
		painter.end();
		QPixmap src;
		QPixmapDropShadowFilter f;
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
	default:
		draw(base,QPoint(0,0));
		break;
	}
	if(Utils::getConfig("/Interface/Debug",false)){
		painter.setPen(Qt::blue);
		painter.setBrush(Qt::NoBrush);
		painter.drawRect(fst.rect().adjusted(0,0,-1,-1));
	}
	painter.end();
	if(opacity==100){
		return fst;
	}
	else{
		QPixmap sec(size);
		sec.fill(Qt::transparent);
		painter.begin(&sec);
		painter.setOpacity(opacity/100.0);
		painter.drawPixmap(QPoint(0,0),fst);
		painter.end();
		return sec;
	}
}

static double getOverlap(double ff,double fs,double sf,double ss)
{
	if(sf<=ff&&ss>=fs){
		return fs-ff;
	}
	if(sf>=ff&&sf<=fs){
		return qMin(fs-sf,ss-sf);
	}
	if(ss<=fs&&ss>=ff){
		return qMin(ss-ff,ss-sf);
	}
	return 0;
}

Graphic *Graphic::create(const Comment &comment,
						 const QSize &size,
						 const QList<Graphic *> &current)
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

#define MIN 10
#define MAX 360

Mode1::Mode1(const Comment &comment,const QList<Graphic *> &current,const QSize &size)
{
	if(comment.mode!=1){
		return;
	}
	QFont font=getFont(comment.font*getScale(comment.mode,comment.date,size));
	QSize bound=getSize(comment.string,font);
	QString exp=Utils::getConfig<QString>("/Danmaku/Speed","125+%{width}/5");
	exp.replace("%{width}",QString::number(bound.width()),Qt::CaseInsensitive);
	if((speed=evaluate(exp))==0){
		return;
	}
	rect=QRectF(QPointF(size.width(),5),bound);
	if(comment.font*(comment.string.count("\n")+1)<MAX){
		double m=0;
		QRectF r;
		int limit=size.height()-(Utils::getConfig("/Danmaku/Protect",false)?80:0)-rect.height();
		for(int height=5;height<limit;height+=MIN){
			rect.moveTop(height);
			double c=0;
			for(Graphic *iter:current){
				c+=intersects(iter);
			}
			if(r.isNull()||c<m){
				m=c;
				r=rect;
				if(c==0){
					break;
				}
			}
		}
		rect=r;
	}
	enabled=true;
	source=&comment;
	cache=getCache(comment.string,comment.color,font,bound);
}

bool Mode1::move(qint64 time)
{
	if(enabled){
		rect.moveLeft(rect.left()-speed*time/1000.0);
	}
	return rect.right()>=0;
}

void Mode1::draw(QPainter *painter)
{
	painter->drawPixmap(rect.topLeft(),cache);
}

uint Mode1::intersects(Graphic *other)
{
	if(other->getMode()!=1){
		return false;
	}
	const Mode1 &f=*dynamic_cast<Mode1 *>(other);
	const Mode1 &s=*this;
	int w=0;
	if(f.rect.intersects(s.rect)){
		if(f.speed>s.speed){
			w=getOverlap(f.rect.left(),f.rect.right(),s.rect.left(),s.rect.right());
		}
		else{
			w=qMin(f.rect.width(),s.rect.width());
		}
	}
	else{
		w=qMax<double>(f.rect.right()-f.speed*s.rect.left()/s.speed,0);
	}
	return getOverlap(f.rect.top(),f.rect.bottom(),s.rect.top(),s.rect.bottom())*w;
}

Mode4::Mode4(const Comment &comment,const QList<Graphic *> &current,const QSize &size)
{
	if(comment.mode!=4){
		return;
	}
	QFont font=getFont(comment.font*getScale(comment.mode,comment.date,size));
	QSize bound=getSize(comment.string,font);
	QString exp=Utils::getConfig<QString>("/Danmaku/Life","5");
	exp.replace("%{width}",QString::number(bound.width()),Qt::CaseInsensitive);
	if((life=evaluate(exp))==0){
		return;
	}
	rect=QRectF(QPointF(0,0),bound);
	rect.moveCenter(QPoint(size.width()/2,0));
	rect.moveBottom(size.height()-(Utils::getConfig("/Danmaku/Protect",false)?size.height()/10:5));
	if(comment.font*(comment.string.count("\n")+1)<MAX){
		double m=0;
		QRectF r;
		int limit=rect.height();
		for(int height=rect.bottom();height>limit;height-=MIN){
			rect.moveBottom(height);
			double c=0;
			for(Graphic *iter:current){
				c+=intersects(iter);
			}
			if(r.isNull()||c<m){
				m=c;
				r=rect;
				if(c==0){
					break;
				}
			}
		}
		rect=r;
	}
	enabled=true;
	source=&comment;
	cache=getCache(comment.string,comment.color,font,bound);
}

bool Mode4::move(qint64 time)
{
	if(enabled){
		life-=time/1000.0;
	}
	return life>0;
}

void Mode4::draw(QPainter *painter)
{
	painter->drawPixmap(rect.topLeft(),cache);
}

uint Mode4::intersects(Graphic *other)
{
	if(other->getMode()!=4){
		return false;
	}
	const Mode4 &f=*this;
	const Mode4 &s=*dynamic_cast<Mode4 *>(other);
	return getOverlap(f.rect.top(),f.rect.bottom(),s.rect.top(),s.rect.bottom())*qMin(f.rect.width(),s.rect.width());
}

Mode5::Mode5(const Comment &comment,const QList<Graphic *> &current,const QSize &size)
{
	if(comment.mode!=5){
		return;
	}
	QFont font=getFont(comment.font*getScale(comment.mode,comment.date,size));
	QSize bound=getSize(comment.string,font);
	QString exp=Utils::getConfig<QString>("/Danmaku/Life","5");
	exp.replace("%{width}",QString::number(bound.width()),Qt::CaseInsensitive);
	if((life=evaluate(exp))==0){
		return;
	}
	rect=QRectF(QPointF(0,0),bound);
	rect.moveCenter(QPoint(size.width()/2,0));
	rect.moveTop(5);
	if(comment.font*(comment.string.count("\n")+1)<MAX){
		double m=0;
		QRectF r;
		int limit=size.height()-(Utils::getConfig("/Danmaku/Protect",false)?80:0)-rect.height();
		for(int height=5;height<limit;height+=MIN){
			rect.moveTop(height);
			double c=0;
			for(Graphic *iter:current){
				c+=intersects(iter);
			}
			if(r.isNull()||c<m){
				m=c;
				r=rect;
				if(c==0){
					break;
				}
			}
		}
		rect=r;
	}
	enabled=true;
	source=&comment;
	cache=getCache(comment.string,comment.color,font,bound);
}

bool Mode5::move(qint64 time)
{
	if(enabled){
		life-=time/1000.0;
	}
	return life>0;
}

void Mode5::draw(QPainter *painter)
{
	painter->drawPixmap(rect.topLeft(),cache);
}

uint Mode5::intersects(Graphic *other)
{
	if(other->getMode()!=5){
		return false;
	}
	const Mode5 &f=*this;
	const Mode5 &s=*dynamic_cast<Mode5 *>(other);
	return getOverlap(f.rect.top(),f.rect.bottom(),s.rect.top(),s.rect.bottom())*qMin(f.rect.width(),s.rect.width());
}

Mode6::Mode6(const Comment &comment,const QList<Graphic *> &current,const QSize &size):
	size(size)
{
	if(comment.mode!=6){
		return;
	}
	QFont font=getFont(comment.font*getScale(comment.mode,comment.date,size));
	QSize bound=getSize(comment.string,font);
	QString exp=Utils::getConfig<QString>("/Danmaku/Speed","125+%{width}/5");
	exp.replace("%{width}",QString::number(bound.width()),Qt::CaseInsensitive);
	if((speed=evaluate(exp))==0){
		return;
	}
	rect=QRectF(QPointF(-bound.width(),5),bound);
	if(comment.font*(comment.string.count("\n")+1)<MAX){
		double m=0;
		QRectF r;
		int limit=size.height()-(Utils::getConfig("/Danmaku/Protect",false)?80:0)-rect.height();
		for(int height=5;height<limit;height+=MIN){
			rect.moveTop(height);
			double c=0;
			for(Graphic *iter:current){
				c+=intersects(iter);
			}
			if(r.isNull()||c<m){
				m=c;
				r=rect;
				if(c==0){
					break;
				}
			}
		}
		rect=r;
	}
	enabled=true;
	source=&comment;
	cache=getCache(comment.string,comment.color,font,bound);
}

bool Mode6::move(qint64 time)
{
	if(enabled){
		rect.moveLeft(rect.left()+speed*time/1000.0);
	}
	return rect.left()<=size.width();
}

void Mode6::draw(QPainter *painter)
{
	painter->drawPixmap(rect.topLeft(),cache);
}

uint Mode6::intersects(Graphic *other)
{
	if(other->getMode()!=6){
		return 0;
	}
	const Mode6 &f=*dynamic_cast<Mode6 *>(other);
	const Mode6 &s=*this;
	int w=0;
	if(f.rect.intersects(s.rect)){
		if(f.speed>s.speed){
			w=getOverlap(f.rect.left(),f.rect.right(),s.rect.left(),s.rect.right());
		}
		else{
			w=qMin(f.rect.width(),s.rect.width());
		}
	}
	else{
		w=qMax<double>(f.rect.left()-f.speed*s.rect.right()/s.speed,0);
	}
	return getOverlap(f.rect.top(),f.rect.bottom(),s.rect.top(),s.rect.bottom())*w;
}

Mode7::Mode7(const Comment &comment,const QList<Graphic *> &,const QSize &size)
{
	if(comment.mode!=7){
		return;
	}
	QJsonArray data=QJsonDocument::fromJson(comment.string.toUtf8()).array();
	int l;
	if((l=data.size())<5){
		return;
	}
	auto getDouble=[&data](int i){return data.at(i).toVariant().toDouble();};
	double scale=getScale(comment.mode,comment.date,size);
	bPos=QPointF(getDouble(0),getDouble(1));
	ePos=l<8?bPos:QPointF(getDouble(7),getDouble(8));
	int w=size.width(),h=size.height();
	if(bPos.x()<1&&bPos.y()<1&&ePos.x()<1&&ePos.y()<1){
		bPos.rx()*=w;
		ePos.rx()*=w;
		bPos.ry()*=h;
		ePos.ry()*=h;
		scale=1;
	}
	else if(scale==0){
		scale=1;
	}
	else{
		QSizeF player=getPlayer(comment.date);
		QPoint offset=QPoint((w-player.width()*scale)/2,(h-player.height()*scale)/2);
		bPos=bPos*scale+offset;
		ePos=ePos*scale+offset;
	}
	QStringList alpha=data[2].toString().split('-');
	bAlpha=alpha[0].toDouble();
	eAlpha=alpha[1].toDouble();
	life=getDouble(3);
	QJsonValue v=l<12?QJsonValue(true):data[11];
	int effect=(v.isString()?v.toString()=="true":v.toVariant().toBool())?Utils::getConfig("/Danmaku/Effect",5)/2:-1;
	QFont font=getFont(scale?comment.font*scale:comment.font,l<13?Utils::defaultFont(true):data[12].toString());
	QString string=data[4].toString();
	cache=getCache(string,comment.color,font,getSize(string,font),effect,100);
	zRotate=l<6?0:getDouble(5);
	yRotate=l<7?0:getDouble(6);
	wait=l<11?0:getDouble(10)/1000;
	stay=l<10?0:life-wait-getDouble(9)/1000;
	enabled=true;
	source=&comment;
	time=0;
}

bool Mode7::move(qint64 time)
{
	if(enabled){
		this->time+=time/1000.0;
	}
	return (this->time)<=life;
}

void Mode7::draw(QPainter *painter)
{
	QPointF cPos=bPos+(ePos-bPos)*qBound<double>(0,(time-wait)/(life-stay),1);
	QTransform rotate;
	rotate.translate(+cPos.x(),+cPos.y());
	rotate.rotate(yRotate,Qt::YAxis);
	rotate.rotate(zRotate,Qt::ZAxis);
	rotate.translate(-cPos.x(),-cPos.y());
	painter->save();
	painter->setTransform(rotate);
	painter->setOpacity(bAlpha+(eAlpha-bAlpha)*time/life);
	painter->drawPixmap(cPos,cache);
	painter->restore();
}

uint Mode7::intersects(Graphic *)
{
	return 0;
}
