/*=======================================================================
*
*   Copyright (C) 2013 Lysine.
*
*   Filename:    Post.cpp
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

#include "Post.h"
#include "Cookie.h"
#include "Danmaku.h"
#include "VPlayer.h"
#include "Graphic.h"

Post::Post(QWidget *parent):
	QDialog(parent,Qt::FramelessWindowHint)
{
	setMinimumSize(480,300);
	setWindowTitle(tr("Post"));
	setAttribute(Qt::WA_TranslucentBackground);
	close=QIcon::fromTheme("go-bottom.png",QIcon(":/Picture/bottom.png"));
	manager=new QNetworkAccessManager(this);
	manager->setCookieJar(Cookie::instance());
	Cookie::instance()->setParent(NULL);
	auto layout=new QGridLayout(this);
	layout->setMargin(0);
	layout->setSpacing(0);
	layout->setRowStretch(0,5);
	commentP=new QLabel(this);
	layout->addWidget(commentP,0,0,1,5);
	commentS=new QComboBox(this);
	for(const Record *r:getRecords()){
		commentS->addItem(QFileInfo(r->source).baseName(),(quintptr)r);
	}
	layout->addWidget(commentS,1,3);
	if(commentS->count()==1) commentS->hide();
	commentM=new QComboBox(this);
	commentM->addItems(QStringList()<<tr("Top")<<tr("Slide")<<tr("Bottom"));
	commentM->setCurrentIndex(1);
	commentM->setFixedWidth(commentM->sizeHint().width());
	connect(commentM,SIGNAL(currentIndexChanged(int)),this,SLOT(drawComment()));
	layout->addWidget(commentM,1,0);
	commentC=new QPushButton(this);
	commentC->setFixedWidth(25);
	setColor(Qt::white);
	connect(commentC,&QPushButton::clicked,[this](){
		QColor color=QColorDialog::getColor(getColor(),parentWidget());
		if(color.isValid()){
			setColor(color);
			drawComment();
		}
	});
	layout->addWidget(commentC,1,1);
	commentL=new QLineEdit(this);
	connect(commentL,&QLineEdit::textChanged,this,&Post::drawComment);
	commentL->setFocus();
	layout->addWidget(commentL,1,2);
	commentB=new QPushButton(tr("Post"),this);
	commentB->setDefault(true);
	commentB->setFixedWidth(55);
	commentB->setToolTip(tr("DA☆ZE!"));
	layout->addWidget(commentB,1,4);
	commentA=new QAction(this);
	commentA->setShortcut(QKeySequence("Ctrl+Enter"));
	connect(commentB,&QPushButton::clicked,commentA,&QAction::trigger);
	connect(commentL,&QLineEdit::returnPressed,commentA,&QAction::trigger);
	connect(commentA,&QAction::triggered,[this](){
		if(!commentL->text().isEmpty()){
			postComment();
			commentL->clear();
		}
	});
}

QColor Post::getColor()
{
	QString sheet=commentC->styleSheet();
	return QColor(sheet.mid(sheet.indexOf('#')));
}

static int mode(int i)
{
	switch(i){
	case 0:
		return 5;
	case 1:
		return 1;
	case 2:
		return 4;
	default:
		return 0;
	}
}

Comment Post::getComment()
{
	Comment c;
	c.mode=mode(commentM->currentIndex());
	c.font=25;
	c.time=qMax<qint64>(0,VPlayer::instance()->getTime());
	c.color=getColor().rgb()&0xFFFFFF;
	c.string=commentL->text();
	return c;
}

void Post::paintEvent(QPaintEvent *e)
{
	QPainter painter(this);
	painter.setPen(QPen(Qt::black,1));
	painter.setBrush(Qt::NoBrush);
	painter.setOpacity(0.2);
	QRect r=rect().adjusted(0,0,0,-commentM->height());
	QPoint p[4]={r.bottomLeft(),r.topLeft(),r.topRight(),r.bottomRight()};
	painter.drawPolyline(p,4);
	painter.setOpacity(0.8);
	close.paint(&painter,width()-20,4,16,16);
	QDialog::paintEvent(e);
}

void Post::mouseReleaseEvent(QMouseEvent *e)
{
	if(e->x()>=width()-20&&e->y()<=20){
		accept();
	}
}

QList<const Record *> Post::getRecords()
{
	QList<const Record *> list;
	for(const Record &r:Danmaku::instance()->getPool()){
		if(r.source.startsWith("http://comment.bilibili.tv/")||r.source.startsWith("http://api.acplay.net/")){
			list.append(&r);
		}
	}
	return list;
}

void Post::setColor(QColor color)
{
	commentC->setStyleSheet(QString("background:%1").arg(color.name()));
}

void Post::drawComment()
{
	if(!commentL->text().isEmpty()){
		Comment comment=getComment();
		comment.string.replace("/n","\n");
		Graphic *g=Graphic::create(comment,commentP->size());
		if(g){
			QRect r=g->currentRect().toRect();
			QPixmap c(r.size());
			c.fill(Qt::transparent);
			QPainter p(&c);
			p.translate(-r.topLeft());
			g->draw(&p);
			switch(g->getMode()){
			case 1:
				commentP->setAlignment(Qt::AlignCenter);
				break;
			case 4:
				commentP->setAlignment(Qt::AlignHCenter|Qt::AlignBottom);
				break;
			case 5:
				commentP->setAlignment(Qt::AlignHCenter|Qt::AlignTop);
				break;
			}
			delete g;
			commentP->setPixmap(c);
			return;
		}
		else{
			commentP->setText(tr("Error while rendering."));
		}
	}
	else{
		commentP->clear();
	}
}

void Post::postComment()
{
	const Record *r=(const Record *)commentS->currentData().value<quintptr>();
	QNetworkRequest request;
	QByteArray data;
	const Comment &c=getComment();
	if(r->source.startsWith("http://comment.bilibili.tv/")){
		request.setUrl(QUrl("http://interface.bilibili.tv/dmpost"));
		request.setHeader(QNetworkRequest::ContentTypeHeader,"application/x-www-form-urlencoded");
		QUrlQuery params;
		params.addQueryItem("cid",QFileInfo(r->source).baseName());
		params.addQueryItem("date",QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));
		params.addQueryItem("pool","0");
		params.addQueryItem("playTime",QString::number((c.time-r->delay)/1000.0,'f',4));
		params.addQueryItem("color",QString::number(c.color));
		params.addQueryItem("fontsize",QString::number(c.font));
		params.addQueryItem("message",c.string);
		params.addQueryItem("rnd",QString::number(qrand()));
		params.addQueryItem("mode",QString::number(c.mode));
		data=QUrl::toPercentEncoding(params.query(QUrl::FullyEncoded),"%=&","-.~_");
	}
	if(r->source.startsWith("http://api.acplay.net/")){
		request.setUrl(QString("http://api.acplay.net/api/v1/comment/%1").arg(QFileInfo(r->source).baseName()));
		request.setHeader(QNetworkRequest::ContentTypeHeader,"application/json");
		QJsonObject params;
		params["Token"]=0;
		params["Time"]=(c.time-r->delay)/1000.0;
		params["Mode"]=c.mode;
		params["Color"]=c.color;
		params["TimeStamp"]=QDateTime::currentMSecsSinceEpoch();
		params["Pool"]=0;
		params["UId"]=0;
		params["CId"]=0;
		params["Message"]=c.string;
		data=QJsonDocument(params).toJson();
	}
	QNetworkReply *reply=manager->post(request,data);
	connect(reply,&QNetworkReply::finished,[=](){
		int error=reply->error();
		QString url=reply->url().toString();
		if(error==QNetworkReply::NoError){
			if(url.startsWith("http://interface.bilibili.tv/")){
				error=qMin<int>(QString(reply->readAll()).toInt(),QNetworkReply::NoError);
			}
			if(url.startsWith("http://acplay.net/")){
				QJsonObject o=QJsonDocument::fromJson(reply->readAll()).object();
				error=o["Success"].toBool()?QNetworkReply::NoError:QNetworkReply::UnknownNetworkError;
			}
		}
		if(error!=QNetworkReply::NoError){
			QString info=tr("Network error occurred, error code: %1");
			QMessageBox::warning(parentWidget(),tr("Network Error"),info.arg(error));
		}
		else{
			Danmaku::instance()->appendToCurrent(&c,true);
		}
		reply->deleteLater();
	});
}

int Post::exec()
{
	QRect p=parentWidget()->geometry(),c=geometry();
	move(p.center().x()-c.center().x(),p.bottom()-c.height());
	return QDialog::exec();
}
