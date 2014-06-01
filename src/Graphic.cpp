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
#include "Config.h"

class Plain:public Graphic
{
public:
	virtual void draw(QPainter *painter);

protected:
	Plain(const Comment &comment,const QSize &size);
	QImage cache;
};

class Mode1:public Plain
{
public:
	Mode1(const Comment &comment,const QSize &size);
	bool move(qint64 time);
	uint intersects(Graphic *other);

private:
	double speed;
};

class Mode4:public Plain
{
public:
	Mode4(const Comment &comment,const QSize &size);
	bool move(qint64 time);
	uint intersects(Graphic *other);

private:
	double life;
};

class Mode5:public Plain
{
public:
	Mode5(const Comment &comment,const QSize &size);
	bool move(qint64 time);
	uint intersects(Graphic *other);

private:
	double life;
};

class Mode6:public Plain
{
public:
	Mode6(const Comment &comment,const QSize &size);
	bool move(qint64 time);
	uint intersects(Graphic *other);

private:
	double speed;
	const QSize &size;
};

class Mode7:public Graphic
{
public:
	Mode7(const Comment &comment,const QSize &size);
	bool move(qint64 time);
	void draw(QPainter *painter);
	uint intersects(Graphic *){return 0;}

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

static QFont getFont(int pixelSize,QString family=Config::getValue("/Danmaku/Font",QFont().family()))
{
	QFont font;
	font.setBold(Config::getValue("/Danmaku/Effect",5)%2);
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
	int m=Config::getValue("/Danmaku/Scale/Fitted",0x1);
	if(mode==7&&(m&0x1)==0){
		return 0;
	}
	if(mode<=6&&(m&0x2)==0){
		return Config::getValue("/Danmaku/Scale/Factor",1.0);
	}
	QSizeF player=getPlayer(date);
	return qMin(size.width()/player.width(),size.height()/player.height());
}

void qt_blurImage(QPainter *p,QImage &blurImage,qreal radius,
				  bool quality,bool alphaOnly,int transposed);

static QImage getCache(QString string,
					   int color,
					   QFont font,
					   QSize size,
					   bool frame,
					   int effect=Config::getValue("/Danmaku/Effect",5)/2,
					   int opacity=Config::getValue("/Danmaku/Alpha",100))
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
	if(frame){
		painter.begin(&fst);
		painter.setPen(QColor(100,255,255));
		painter.setBrush(Qt::NoBrush);
		painter.drawRect(fst.rect().adjusted(0,0,-1,-1));
		painter.end();
	}
	if(opacity!=100){
		QImage sec(size,QImage::Format_ARGB32_Premultiplied);
		sec.fill(Qt::transparent);
		painter.begin(&sec);
		painter.setOpacity(opacity/100.0);
		painter.drawImage(QPoint(0,0),fst);
		painter.end();
		fst=sec;
	}
	return fst;
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
						 const QSize &size)
{
	Graphic *graphic=NULL;
	switch(comment.mode){
	case 1:
		graphic=new Mode1(comment,size);
		break;
	case 4:
		graphic=new Mode4(comment,size);
		break;
	case 5:
		graphic=new Mode5(comment,size);
		break;
	case 6:
		graphic=new Mode6(comment,size);
		break;
	case 7:
		graphic=new Mode7(comment,size);
		break;
	}
	if(graphic!=NULL&&!graphic->isEnabled()){
		delete graphic;
		return NULL;
	}
	return graphic;
}


void Graphic::setIndex()
{
	static quint64 globalIndex;
	index=globalIndex++;
}

Plain::Plain(const Comment &comment,const QSize &size)
{
	source=&comment;
	QFont font=getFont(comment.font*getScale(comment.mode,comment.date,size));
	QSize need=getSize(comment.string,font);
	rect.setSize(need);
	cache=getCache(comment.string,comment.color,font,need,comment.isLocal());
}

void Plain::draw(QPainter *painter)
{
	if(enabled){
		painter->drawImage(rect.topLeft(),cache);
	}
}

Mode1::Mode1(const Comment &comment,const QSize &size):
	Plain(comment,size)
{
	if(comment.mode!=1){
		return;
	}
	QString expression=Config::getValue<QString>("/Danmaku/Speed","125+%{width}/5");
	expression.replace("%{width}",QString::number(rect.width()),Qt::CaseInsensitive);
	if((speed=Utils::evaluate(expression))==0){
		return;
	}
	rect.moveTopLeft(QPointF(size.width(),0));
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
		return 0;
	}
	const Mode1 &f=*dynamic_cast<Mode1 *>(other);
	const Mode1 &s=*this;
	int h;
	if((h=getOverlap(f.rect.top(),f.rect.bottom(),s.rect.top(),s.rect.bottom()))==0){
		return 0;
	}
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
		double o=f.rect.right()-f.speed*s.rect.left()/s.speed;
		w=o>0?qMin(qMin(f.rect.width(),s.rect.width()),o):0;
	}
	return h*w;
}

Mode4::Mode4(const Comment &comment,const QSize &size):
	Plain(comment,size)
{
	if(comment.mode!=4){
		return;
	}
	QString expression=Config::getValue<QString>("/Danmaku/Life","5");
	expression.replace("%{width}",QString::number(rect.width()),Qt::CaseInsensitive);
	if((life=Utils::evaluate(expression))==0){
		return;
	}
	rect.moveCenter(QPointF(size.width()/2.0,0));
	rect.moveBottom(size.height()*(Config::getValue("/Danmaku/Protect",false)?0.85:1));
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
		return 0;
	}
	const Mode4 &f=*this;
	const Mode4 &s=*dynamic_cast<Mode4 *>(other);
	return getOverlap(f.rect.top(),f.rect.bottom(),s.rect.top(),s.rect.bottom())*qMin(f.rect.width(),s.rect.width());
}

Mode5::Mode5(const Comment &comment,const QSize &size):
	Plain(comment,size)
{
	if(comment.mode!=5){
		return;
	}
	QSizeF bound=rect.size();
	QString expression=Config::getValue<QString>("/Danmaku/Life","5");
	expression.replace("%{width}",QString::number(bound.width()),Qt::CaseInsensitive);
	if((life=Utils::evaluate(expression))==0){
		return;
	}
	rect.moveCenter(QPointF(size.width()/2.0,0));
	rect.moveTop(0);
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
		return 0;
	}
	const Mode5 &f=*this;
	const Mode5 &s=*dynamic_cast<Mode5 *>(other);
	return getOverlap(f.rect.top(),f.rect.bottom(),s.rect.top(),s.rect.bottom())*qMin(f.rect.width(),s.rect.width());
}

Mode6::Mode6(const Comment &comment,const QSize &size):
	Plain(comment,size),size(size)
{
	if(comment.mode!=6){
		return;
	}
	QString expression=Config::getValue<QString>("/Danmaku/Speed","125+%{width}/5");
	expression.replace("%{width}",QString::number(rect.width()),Qt::CaseInsensitive);
	if((speed=Utils::evaluate(expression))==0){
		return;
	}
	rect.moveTopLeft(QPointF(-rect.width(),0));
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
	int h;
	if((h=getOverlap(f.rect.top(),f.rect.bottom(),s.rect.top(),s.rect.bottom()))==0){
		return 0;
	}
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
		double o=f.rect.left()-f.speed*s.rect.right()/s.speed;
		w=o>0?qMin(qMin(f.rect.width(),s.rect.width()),o):0;
	}
	return h*w;
}

Mode7::Mode7(const Comment &comment,const QSize &size)
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
	int effect=(v.isString()?v.toString()=="true":v.toVariant().toBool())?Config::getValue("/Danmaku/Effect",5)/2:-1;
	QFont font=getFont(scale?comment.font*scale:comment.font,l<13?Utils::defaultFont(true):data[12].toString());
	QString string=data[4].toString();
	cache=getCache(string,comment.color,font,getSize(string,font),comment.isLocal(),effect,100);
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
