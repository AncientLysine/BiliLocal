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

static QFont getFont(const Comment &comment)
{
	QFont font;
	font.setBold(Utils::getConfig("/Danmaku/Effect",5)%2);
	font.setFamily(Utils::getConfig("/Danmaku/Font",QFont().family()));
	font.setPixelSize(Utils::getConfig("/Danmaku/Scale",1.0)*comment.font);
	return font;
}

static QSize getSize(const Comment &comment,QFont font)
{
	return QFontMetrics(font).boundingRect(QRect(),Qt::TextDontClip,comment.string).size()+QSize(4,4);
}

static QPixmap getCache(const Comment &comment,QFont font)
{
	QSize bound=getSize(comment,font);
	QStaticText text;
	text.setText(QString(comment.string).replace("\n","<br>"));
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
	}
	painter.end();
	QPixmap sec(bound);
	sec.fill(Qt::transparent);
	painter.begin(&sec);
	painter.setOpacity(Utils::getConfig("/Danmaku/Alpha",1.0));
	painter.drawPixmap(QPoint(0,0),fst);
	painter.end();
	return sec;
}

Graphic::~Graphic()
{
}

Mode1::Mode1(const Comment &comment,QList<Graphic *> &current,QSize size)
{
	if(comment.mode!=1){
		return;
	}
	QSize bound=getSize(comment,getFont(comment));
	QString exp=Utils::getConfig<QString>("/Danmaku/Speed","125+%{width}/5");
	exp.replace("%{width}",QString::number(bound.width()),Qt::CaseInsensitive);
	if((speed=evaluate(exp))==0){
		return;
	}
	rect=QRectF(QPointF(0,0),bound);
	rect.moveLeft(size.width());
	int limit=size.height()-(Utils::getConfig("/Danmaku/Protect",false)?80:0)-rect.height();
	for(int height=5;height<limit;height+=10){
		rect.moveTop(height);
		bool flag=true;
		for(Graphic *iter:current){
			if(intersects(iter)){
				flag=false;
				break;
			}
		}
		if(flag){
			mode=1;
			cache=getCache(comment,getFont(comment));
			current.append(this);
			break;
		}
	}
}

bool Mode1::move(qint64 time)
{
	rect.moveLeft(rect.left()-speed*time/1000.0);
	return rect.right()>=0;
}

void Mode1::draw(QPainter *painter)
{
	painter->drawPixmap(rect.topLeft(),cache);
}

bool Mode1::intersects(Graphic *other)
{
	if(other->getMode()!=1){
		return false;
	}
	const Mode1 &f=*dynamic_cast<Mode1 *>(other);
	const Mode1 &s=*this;
	if(f.rect.intersects(s.rect)){
		return true;
	}
	int ft=f.rect.top();
	int fb=f.rect.bottom();
	int st=s.rect.top();
	int sb=s.rect.bottom();
	bool sameHeight=(st>=ft&&st<=fb)||(sb<=fb&&sb>=ft)||(st<=ft&&sb>=fb);
	return sameHeight&&f.rect.right()/f.speed>=s.rect.left()/s.speed;
}

Mode4::Mode4(const Comment &comment,QList<Graphic *> &current,QSize size)
{
	if(comment.mode!=4){
		return;
	}
	QSize bound=getSize(comment,getFont(comment));
	QString exp=Utils::getConfig<QString>("/Danmaku/Life","5");
	exp.replace("%{width}",QString::number(bound.width()),Qt::CaseInsensitive);
	if((life=evaluate(exp))==0){
		return;
	}
	rect=QRectF(QPointF(0,0),bound);
	rect.moveCenter(QPoint(size.width()/2,0));
	int limit=rect.height();
	int height=size.height()-(Utils::getConfig("/Danmaku/Protect",false)?size.height()/10:5);
	for(;height>limit;height-=10){
		rect.moveBottom(height);
		bool flag=true;
		for(Graphic *iter:current){
			if(intersects(iter)){
				flag=false;
				break;
			}
		}
		if(flag){
			mode=4;
			cache=getCache(comment,getFont(comment));
			current.append(this);
			break;
		}
	}
}

bool Mode4::move(qint64 time)
{
	life-=time/1000.0;
	return life>0;
}

void Mode4::draw(QPainter *painter)
{
	painter->drawPixmap(rect.topLeft(),cache);
}

bool Mode4::intersects(Graphic *other)
{
	if(other->getMode()!=4){
		return false;
	}
	const Mode4 &f=*this;
	const Mode4 &s=*dynamic_cast<Mode4 *>(other);
	return f.rect.intersects(s.rect);
}

Mode5::Mode5(const Comment &comment,QList<Graphic *> &current,QSize size)
{
	if(comment.mode!=5){
		return;
	}
	QSize bound=getSize(comment,getFont(comment));
	QString exp=Utils::getConfig<QString>("/Danmaku/Life","5");
	exp.replace("%{width}",QString::number(bound.width()),Qt::CaseInsensitive);
	if((life=evaluate(exp))==0){
		return;
	}
	rect=QRectF(QPointF(0,0),bound);
	rect.moveCenter(QPoint(size.width()/2,0));
	int limit=size.height()-(Utils::getConfig("/Danmaku/Protect",false)?80:0)-rect.height();
	for(int height=5;height<limit;height+=10){
		rect.moveBottom(height);
		bool flag=true;
		for(Graphic *iter:current){
			if(intersects(iter)){
				flag=false;
				break;
			}
		}
		if(flag){
			mode=5;
			cache=getCache(comment,getFont(comment));
			current.append(this);
			break;
		}
	}
}

bool Mode5::move(qint64 time)
{
	life-=time/1000.0;
	return life>0;
}

void Mode5::draw(QPainter *painter)
{
	painter->drawPixmap(rect.topLeft(),cache);
}

bool Mode5::intersects(Graphic *other)
{
	if(other->getMode()!=5){
		return false;
	}
	const Mode5 &f=*this;
	const Mode5 &s=*dynamic_cast<Mode5 *>(other);
	return f.rect.intersects(s.rect);
}

Mode7::Mode7(const Comment &comment,QList<Graphic *> &current,QSize size)
{
	if(comment.mode!=7){
		return;
	}
	QJsonArray data=QJsonDocument::fromJson(comment.string.toUtf8()).array();
	if(data.size()!=14){
		Printer::instance()->append(QString("[Danmaku]unknown or broken mode7 code %1").arg(comment.string));
		return;
	}
	bPos=QPointF(data[0].toDouble(),data[1].toDouble());
	ePos=QPointF(data[7].toDouble(),data[8].toDouble());
	if(bPos.x()<1&&bPos.y()<1&&ePos.x()<1&&ePos.y()<1){
		bPos.rx()*=size.width();
		ePos.rx()*=size.width();
		bPos.ry()*=size.height();
		ePos.ry()*=size.height();
	}
	QStringList alpha=data[2].toString().split('-');
	bAlpha=alpha[0].toDouble();
	eAlpha=alpha[1].toDouble();
	life=data[3].toDouble();
	Comment c=comment;
	c.string=data[4].toString();
	QFont f=getFont(comment);
	f.setFamily(data[12].toString());
	cache=getCache(c,f);
	zRotate=data[5].toDouble();
	yRotate=data[6].toDouble();
	wait=data[10].toDouble()/1000;
	stay=life-wait-data[9].toDouble()/1000;
	mode=7;
	current.append(this);
}

bool Mode7::move(qint64 time)
{
	this->time+=time/1000.0;
	return this->time<=life;
}

void Mode7::draw(QPainter *painter)
{
	QTransform rotate;
	rotate.rotate(yRotate,Qt::YAxis);
	rotate.rotate(zRotate,Qt::ZAxis);
	QTransform trans=painter->transform();
	painter->setTransform(rotate);
	double aplha=painter->opacity();
	painter->setOpacity(bAlpha+(eAlpha-bAlpha)*time/life);
	painter->drawPixmap(bPos+(ePos-bPos)*qBound<double>(0,(time-wait)/(life-stay),1),cache);
	painter->setOpacity(aplha);
	painter->setTransform(trans);
}

bool Mode7::intersects(Graphic *)
{
	return false;
}
