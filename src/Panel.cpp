/*=======================================================================
*
*   Copyright (C) 2013 Lysine.
*
*   Filename:    Panel.cpp
*   Time:        2013/05/23
*   Author:      zhengdanwei
*   Contributor: Lysine
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

#include "Panel.h"
#include "Utils.h"
#include "Cookie.h"
#include "Danmaku.h"
#include "VPlayer.h"

#define SPLIT 1500

static QHash<int,int> mode()
{
	static QHash<int,int> mode;
	if(mode.isEmpty()){
		mode[0]=5;
		mode[1]=1;
		mode[2]=4;
	}
	return mode;
}

namespace{
class Jumper:public QObject
{
public:
	Jumper(QAbstractSlider *parent,bool &flag):
		QObject(parent),f(flag)
	{
		s=parent;
		s->installEventFilter(this);
	}

	bool eventFilter(QObject *o, QEvent *e)
	{
		switch(e->type()){
		case QEvent::MouseButtonRelease:
			f=0;
			s->setValue(getPosintion(e));
			return true;
		case QEvent::MouseMove:
		case QEvent::MouseButtonPress:
			f=1;
			s->setSliderPosition(getPosintion(e));
			return true;
		case QEvent::Paint:
		{
			QPainter p(s);
			p.setRenderHint(QPainter::Antialiasing);
			QStyleOptionSlider o;
			o.initFrom(s);
			o.subControls = QStyle::SC_SliderGroove;
			o.activeSubControls = QStyle::SC_None;
			o.orientation = s->orientation();
			o.maximum = s->maximum();
			o.minimum = s->minimum();
			o.tickPosition = QSlider::NoTicks;
			o.tickInterval = 0;
			o.upsideDown = false;
			o.sliderPosition = s->sliderPosition();
			o.sliderValue = s->value();
			o.singleStep = s->singleStep();
			o.pageStep = s->pageStep();
			o.state = QStyle::State_Horizontal;
			QRect g = s->style()->subControlRect(QStyle::CC_Slider, &o, QStyle::SC_SliderGroove, s);
			p.translate(0.5, 0.5);
			QLinearGradient gradient;
			gradient.setStart(g.center().x(), g.top());
			gradient.setFinalStop(g.center().x(), g.bottom());
			QColor outline = s->palette().background().color().darker(140);
			p.setPen(QPen(outline));
			QColor grooveColor,buttonColor = button(s->palette());
			grooveColor.setHsv(buttonColor.hue(),
							   qMin(255, (int)(buttonColor.saturation())),
							   qMin(255, (int)(buttonColor.value()*0.9)));
			gradient.setColorAt(0, grooveColor.darker(110));
			gradient.setColorAt(1, grooveColor.lighter(110));
			p.setBrush(gradient);
			p.drawRoundedRect(g.adjusted(1, 1, -2, -2), 1, 1);
			if(s->maximum()){
				QColor highlight = s->palette().color(QPalette::Highlight);
				QColor highlightedoutline = highlight.darker(140);
				if (qGray(outline.rgb()) > qGray(highlightedoutline.rgb()))
					outline = highlightedoutline;
				p.setPen(QPen(outline));
				gradient.setColorAt(0, highlight);
				gradient.setColorAt(1, highlight.lighter(130));
				p.setBrush(gradient);
				g.setRight(g.right()*s->sliderPosition()/s->maximum());
				p.drawRoundedRect(g.adjusted(1, 1, -2, -2), 1, 1);
				p.setPen(QColor(255, 255, 255, 30));
				p.setBrush(Qt::NoBrush);
				p.drawRoundedRect(g.adjusted(2, 2, -3, -3), 1, 1);
			}
			p.end();
			return true;
		}
		default:
			return QObject::eventFilter(o,e);
		}
	}

private:
	bool &f;
	QAbstractSlider *s;

	int getPosintion(QEvent *e)
	{
		return dynamic_cast<QMouseEvent *>(e)->pos().x()*SPLIT/s->width();
	}

	QColor button(const QPalette &pal) const
	{
		QColor buttonColor = pal.button().color();
		int val = qGray(buttonColor.rgb());
		buttonColor = buttonColor.lighter(100 + qMax(1, (180 - val)/6));
		buttonColor.setHsv(buttonColor.hue(), buttonColor.saturation() * 0.75, buttonColor.value());
		return buttonColor;
	}
};
}

Panel::Panel(QWidget *parent) :
	QWidget(parent)
{
	ioo=0;
	sliding=updating=false;
	duration=-1;
	timer=new QTimer(this);
	connect(timer,&QTimer::timeout,[this](){
		if(ioo==0){
			timer->stop();
		}
		else if(ioo==1){
			if(effect->opacity()>=0.9){
				effect->setOpacity(1.0);
				timer->stop();
				ioo=2;
			}
			else{
				effect->setOpacity(effect->opacity()+0.1);
			}
		}
		else if(ioo==3){
			if(effect->opacity()<=0.1){
				effect->setOpacity(0.0);
				timer->stop();
				ioo=0;
				hide();
			}
			else{
				effect->setOpacity(effect->opacity()-0.1);
			}
		}
	});
	effect=new QGraphicsOpacityEffect(this);
	effect->setOpacity(0.0);
	setGraphicsEffect(effect);
	manager=new QNetworkAccessManager(this);
	manager->setCookieJar(Cookie::instance());
	Cookie::instance()->setParent(NULL);
	auto layout=new QGridLayout(this);
	layout->setMargin(0);
	layout->setSpacing(0);
	timeS=new QAbstractSlider(this);
	timeS->setOrientation(Qt::Horizontal);
	timeS->setRange(0,0);
	timeS->setValue(0);
	timeS->setTracking(false);
	connect(timeS,&QSlider::valueChanged,[this](int _time){
		if(duration!=-1&&!updating){
			emit time(duration*_time/SPLIT);
		}
	});
	new Jumper(timeS,sliding);
	layout->addWidget(timeS,0,0,1,4);
	commentM=new QComboBox(this);
	commentM->addItems(QStringList()<<tr("Top")<<tr("Slide")<<tr("Bottom"));
	commentM->setCurrentIndex(1);
	commentM->setFixedWidth(commentM->sizeHint().width());
	layout->addWidget(commentM,1,0);
	commentC=new QPushButton(this);
	commentC->setFixedWidth(25);
	setColor(Qt::white);
	connect(commentC,&QPushButton::clicked,[this](){
		QColor color=QColorDialog::getColor(getColor(),parentWidget());
		if(color.isValid()) setColor(color);
	});
	layout->addWidget(commentC,1,1);
	commentL=new QLineEdit(this);
	commentL->setEnabled(false);
	layout->addWidget(commentL,1,2);
	commentB=new QPushButton(tr("Post"),this);
	commentB->setEnabled(false);
	commentB->setFixedWidth(55);
	commentB->setToolTip(tr("DA☆ZE!"));
	layout->addWidget(commentB,1,3);
	commentA=new QAction(this);
	commentA->setShortcut(QKeySequence("Ctrl+Enter"));
	connect(commentB,&QPushButton::clicked,commentA,&QAction::trigger);
	connect(commentL,&QLineEdit::returnPressed,commentA,&QAction::trigger);
	connect(commentA,&QAction::triggered,[this](){
		if(!commentL->text().isEmpty()){
			postComment(commentL->text());
			commentL->setText("");
		}
	});
	connect(VPlayer::instance(),&VPlayer::begin,[this](){
		setDuration(VPlayer::instance()->getDuration());
	});
	connect(VPlayer::instance(),&VPlayer::reach,[this](){
		setDuration(-1);
	});
	connect(Danmaku::instance(),&Danmaku::layoutChanged,[this](){
		QString cid=getCid();
		commentL->setEnabled(!cid.isEmpty());
		commentB->setEnabled(!cid.isEmpty());
	});
	hide();
}

QColor Panel::getColor()
{
	QString sheet=commentC->styleSheet();
	return QColor(sheet.mid(sheet.indexOf('#')));
}

void Panel::fadeIn()
{
	if(ioo==0){
		ioo=1;
		timer->start(20);
		show();
	}
}

void Panel::fadeOut()
{
	if(ioo==2){
		ioo=3;
		timer->start(20);
	}
}

void Panel::setTime(qint64 _time)
{
	if(!timeS->isSliderDown()){
		int position=timeS->sliderPosition();
		updating=1;
		timeS->setValue(_time*SPLIT/duration);
		updating=0;
		if(sliding){
			timeS->setSliderPosition(position);
		}
	}
}

void Panel::setColor(QColor color)
{
	commentC->setStyleSheet(QString("background-color:%1").arg(color.name()));
}

void Panel::postComment(QString comment)
{
	QString cid=getCid();
	if(!cid.isEmpty()){
		Comment c;
		c.mode=mode()[commentM->currentIndex()];
		c.font=25;
		c.time=qMax<qint64>(0,VPlayer::instance()->getTime());
		c.color=getColor().rgb()&0xFFFFFF;
		c.string=comment;
		QNetworkRequest request(QUrl("http://interface.bilibili.tv/dmpost"));
		request.setHeader(QNetworkRequest::ContentTypeHeader,"application/x-www-form-urlencoded");
		QUrlQuery params;
		params.addQueryItem("cid",cid);
		params.addQueryItem("date",QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));
		params.addQueryItem("pool","0");
		params.addQueryItem("playTime",QString::number(c.time/1000.0,'f',4));
		params.addQueryItem("color",QString::number(c.color));
		params.addQueryItem("fontsize",QString::number(c.font));
		params.addQueryItem("message",c.string);
		params.addQueryItem("rnd",QString::number(qrand()));
		params.addQueryItem("mode",QString::number(c.mode));
		QNetworkReply *reply=manager->post(request,QUrl::toPercentEncoding(params.query(QUrl::FullyEncoded),"%=&","-.~_"));
		connect(reply,&QNetworkReply::finished,[=](){
			int error=reply->error();
			if(error==QNetworkReply::NoError){
				error=qMin<int>(QString(reply->readAll()).toInt(),QNetworkReply::NoError);
			}
			if(error!=QNetworkReply::NoError){
				QString info=tr("Network error occurred, error code: %1");
				QMessageBox::warning(parentWidget(),tr("Network Error"),info.arg(error));
			}
			else{
				Danmaku::instance()->appendToCurrent(&c,true);
			}
		});
	}
	else{
		QMessageBox::warning(this,tr("Warning"),tr("Empty cid."));
	}
}

void Panel::setDuration(qint64 _duration)
{
	if(_duration>0){
		duration=_duration;
		timeS->setRange(0,SPLIT);
	}
	else{
		duration=-1;
		timeS->setValue(0);
		timeS->setRange(0,0);
	}
}

QString Panel::getCid()
{
	for(const Record &r:Danmaku::instance()->getPool()){
		if(r.source.startsWith("http://comment.bilibili.tv/")){
			return QFileInfo(r.source).baseName();
		}
	}
	return QString();
}
