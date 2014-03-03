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

class Plain:public Graphic
{
public:
	virtual void draw(QPainter *painter);
	virtual QRectF currentRect(){return rect;}

protected:
	Plain(const Comment &comment,const QSize &size);
	QRectF rect;
	QImage cache;
};

class Mode1:public Plain
{
public:
	Mode1(const Comment &comment,const QSize &size,const QList<Graphic *> &current);
	bool move(qint64 time);
	uint intersects(Graphic *other);

private:
	double speed;
};

class Mode4:public Plain
{
public:
	Mode4(const Comment &comment,const QSize &size,const QList<Graphic *> &current);
	bool move(qint64 time);
	uint intersects(Graphic *other);

private:
	double life;
};

class Mode5:public Plain
{
public:
	Mode5(const Comment &comment,const QSize &size,const QList<Graphic *> &current);
	bool move(qint64 time);
	uint intersects(Graphic *other);

private:
	double life;
};

class Mode6:public Plain
{
public:
	Mode6(const Comment &comment,const QSize &size,const QList<Graphic *> &current);
	bool move(qint64 time);
	uint intersects(Graphic *other);

private:
	double speed;
	const QSize &size;
};

class Mode7:public Graphic
{
public:
	Mode7(const Comment &comment,const QSize &size,const QList<Graphic *> &current);
	bool move(qint64 time);
	void draw(QPainter *painter);
	uint intersects(Graphic *){return 0;}
	QRectF currentRect(){return QRectF();}

private:
	QPointF bPos;
	QPointF ePos;
	double bAlpha;
	double eAlpha;
	double zRotate;
	double yRotate;
	QImage cache;
	double wait;
	double stay;
	double life;
	double time;
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
	return date<=1384099200?QSizeF(545,388):QSizeF(862,568);
}

static double getScale(int mode,qint64 date,QSize size)
{
	int scale=Utils::getConfig("/Danmaku/Scale",0x1);
	if(mode<=6&&(scale&0x2)==0) return 1;
	if(mode==7&&(scale&0x1)==0) return 0;
	QSizeF player=getPlayer(date);
	return qMin(size.width()/player.width(),size.height()/player.height());
}

void qt_blurImage(QPainter *p,QImage &blurImage,qreal radius,
				  bool quality,bool alphaOnly,int transposed);

static QImage getCache(QString string,
					   int color,
					   QFont font,
					   QSize size,
					   int effect=Utils::getConfig("/Danmaku/Effect",5)/2,
					   int opacity=Utils::getConfig("/Danmaku/Alpha",100))
{
	QPainter painter;
	QColor base(color),edge=qGray(color)<30?Qt::white:Qt::black;
	QImage src(size,QImage::Format_ARGB32_Premultiplied);
	src.fill(Qt::transparent);
	painter.begin(&src);
	painter.setPen(base);
	painter.setFont(font);
	painter.drawText(src.rect().adjusted(2,2,-2,-2),string);
	painter.end();
	QImage fst(size,QImage::Format_ARGB32_Premultiplied);
	fst.fill(Qt::transparent);
	if(effect==2){
		QImage blr=src;
		painter.begin(&fst);
		painter.save();
		qt_blurImage(&painter,blr,4,false,true,0);
		painter.restore();
		painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
		painter.fillRect(src.rect(),edge);
		painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
		painter.drawImage(0,0,src);
		painter.end();
		QImage sec(size,QImage::Format_ARGB32_Premultiplied);
		sec.fill(Qt::transparent);
		painter.begin(&sec);
		blr=fst;
		painter.save();
		qt_blurImage(&painter,blr,4,false,true,0);
		painter.restore();
		painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
		painter.fillRect(sec.rect(),edge);
		painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
		painter.drawImage(0,0,fst);
		painter.end();
		fst=sec;
	}
	else{
		QImage edg=src;
		painter.begin(&edg);
		painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
		painter.fillRect(edg.rect(),edge);
		painter.end();
		painter.begin(&fst);
		switch(effect){
		case 0:
			painter.drawImage(+1,0,edg);
			painter.drawImage(-1,0,edg);
			painter.drawImage(0,+1,edg);
			painter.drawImage(0,-1,edg);
			break;
		case 1:
			painter.drawImage(1,1,edg);
			break;
		}
		painter.drawImage(0,0,src);
		painter.end();
	}
	if(opacity==100){
		return fst;
	}
	else{
		QImage sec(size,QImage::Format_ARGB32_Premultiplied);
		sec.fill(Qt::transparent);
		painter.begin(&sec);
		painter.setOpacity(opacity/100.0);
		painter.drawImage(QPoint(0,0),fst);
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

static void setRectangle(QRectF &rect,
						 Graphic *self,
						 const QSize &size,
						 const QList<Graphic *> &current)
{
	const Comment &comment=*self->getSource();
	if(comment.font*(comment.string.count("\n")+1)>=360){
		return;
	}
	double m=-1;
	QRectF r=rect;
	switch(self->getMode())
	{
	case 1:
	case 5:
	case 6:
	{
		int limit=size.height()-(Utils::getConfig("/Danmaku/Protect",false)?80:0)-rect.height();
		for(int height=rect.top();height<limit;height+=10){
			rect.moveTop(height);
			double c=0;
			for(Graphic *iter:current){
				c+=self->intersects(iter);
			}
			if(m==-1||c<m){
				m=c;
				r=rect;
				if(c==0){
					break;
				}
			}
		}
		rect=r;
		break;
	}
	case 4:
	{
		int limit=rect.height();
		for(int height=rect.bottom();height>limit;height-=10){
			rect.moveBottom(height);
			double c=0;
			for(Graphic *iter:current){
				c+=self->intersects(iter);
			}
			if(m==-1||c<m){
				m=c;
				r=rect;
				if(c==0){
					break;
				}
			}
		}
		rect=r;
		break;
	}
	}
}

Graphic *Graphic::create(const Comment &comment,
						 const QSize &size,
						 const QList<Graphic *> &current)
{
	Graphic *graphic=NULL;
	switch(comment.mode){
	case 1:
		graphic=new Mode1(comment,size,current);
		break;
	case 4:
		graphic=new Mode4(comment,size,current);
		break;
	case 5:
		graphic=new Mode5(comment,size,current);
		break;
	case 6:
		graphic=new Mode6(comment,size,current);
		break;
	case 7:
		graphic=new Mode7(comment,size,current);
		break;
	}
	if(graphic!=NULL&&!graphic->isEnabled()){
		delete graphic;
		return NULL;
	}
	return graphic;
}

Plain::Plain(const Comment &comment,const QSize &size)
{
	source=&comment;
	QFont font=getFont(comment.font*getScale(comment.mode,comment.date,size));
	QSize need=getSize(comment.string,font);
	rect.setSize(need);
	cache=getCache(comment.string,comment.color,font,need);
}

void Plain::draw(QPainter *painter)
{
	if(enabled){
		painter->drawImage(rect.topLeft(),cache);
	}
}

Mode1::Mode1(const Comment &comment,const QSize &size,const QList<Graphic *> &current):
	Plain(comment,size)
{
	if(comment.mode!=1){
		return;
	}
	QString expression=Utils::getConfig<QString>("/Danmaku/Speed","125+%{width}/5");
	expression.replace("%{width}",QString::number(rect.width()),Qt::CaseInsensitive);
	if((speed=Utils::evaluate(expression))==0){
		return;
	}
	rect.moveTopLeft(QPointF(size.width(),0));
	setRectangle(rect,this,size,current);
	enabled=true;
}

bool Mode1::move(qint64 time)
{
	if(enabled){
		rect.moveLeft(rect.left()-speed*time/1000.0);
	}
	return rect.right()>=0;
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

Mode4::Mode4(const Comment &comment,const QSize &size,const QList<Graphic *> &current):
	Plain(comment,size)
{
	if(comment.mode!=4){
		return;
	}
	QString expression=Utils::getConfig<QString>("/Danmaku/Life","5");
	expression.replace("%{width}",QString::number(rect.width()),Qt::CaseInsensitive);
	if((life=Utils::evaluate(expression))==0){
		return;
	}
	rect.moveCenter(QPointF(size.width()/2.0,0));
	rect.moveBottom(size.height()-(Utils::getConfig("/Danmaku/Protect",false)?size.height()/10:0));
	setRectangle(rect,this,size,current);
	enabled=true;
}

bool Mode4::move(qint64 time)
{
	if(enabled){
		life-=time/1000.0;
	}
	return life>0;
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

Mode5::Mode5(const Comment &comment,const QSize &size,const QList<Graphic *> &current):
	Plain(comment,size)
{
	if(comment.mode!=5){
		return;
	}
	QSizeF bound=rect.size();
	QString expression=Utils::getConfig<QString>("/Danmaku/Life","5");
	expression.replace("%{width}",QString::number(bound.width()),Qt::CaseInsensitive);
	if((life=Utils::evaluate(expression))==0){
		return;
	}
	rect.moveCenter(QPointF(size.width()/2.0,0));
	rect.moveTop(0);
	setRectangle(rect,this,size,current);
	enabled=true;
}

bool Mode5::move(qint64 time)
{
	if(enabled){
		life-=time/1000.0;
	}
	return life>0;
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

Mode6::Mode6(const Comment &comment,const QSize &size,const QList<Graphic *> &current):
	Plain(comment,size),size(size)
{
	if(comment.mode!=6){
		return;
	}
	QString expression=Utils::getConfig<QString>("/Danmaku/Speed","125+%{width}/5");
	expression.replace("%{width}",QString::number(rect.width()),Qt::CaseInsensitive);
	if((speed=Utils::evaluate(expression))==0){
		return;
	}
	rect.moveTopLeft(QPointF(-rect.width(),0));
	setRectangle(rect,this,size,current);
	enabled=true;
}

bool Mode6::move(qint64 time)
{
	if(enabled){
		rect.moveLeft(rect.left()+speed*time/1000.0);
	}
	return rect.left()<=size.width();
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

Mode7::Mode7(const Comment &comment,const QSize &size,const QList<Graphic *> &)
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
	if(enabled){
		QPointF cPos=bPos+(ePos-bPos)*qBound<double>(0,(time-wait)/(life-stay),1);
		QTransform rotate;
		rotate.translate(+cPos.x(),+cPos.y());
		rotate.rotate(yRotate,Qt::YAxis);
		rotate.rotate(zRotate,Qt::ZAxis);
		rotate.translate(-cPos.x(),-cPos.y());
		painter->save();
		painter->setTransform(rotate);
		painter->setOpacity(bAlpha+(eAlpha-bAlpha)*time/life);
		painter->drawImage(cPos,cache);
		painter->restore();
	}
}
