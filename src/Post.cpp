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
	setFixedSize(480,25);
	setAttribute(Qt::WA_TranslucentBackground);
	moveWithParent();
	parent->installEventFilter(this);
	manager=new QNetworkAccessManager(this);
	manager->setCookieJar(Cookie::instance());
	Cookie::instance()->setParent(NULL);
	auto layout=new QHBoxLayout(this);
	layout->setMargin(0);
	layout->setSpacing(0);
	commentM=new QComboBox(this);
	commentM->addItems(QStringList()<<tr("Top")<<tr("Slide")<<tr("Bottom"));
	commentM->setCurrentIndex(1);
	commentM->setFixedWidth(commentM->sizeHint().width());
	layout->addWidget(commentM);
	commentC=new QPushButton(this);
	commentC->setFixedWidth(25);
	setColor(Qt::white);
	connect(commentC,&QPushButton::clicked,[this](){
		QColor color=QColorDialog::getColor(getColor(),parentWidget());
		if(color.isValid()){
			setColor(color);
		}
	});
	layout->addWidget(commentC);
	commentL=new QLineEdit(this);
	commentL->setFocus();
	layout->addWidget(commentL);
	commentS=new QComboBox(this);
	layout->addWidget(commentS);
	commentB=new QPushButton(tr("Post"),this);
	commentB->setDefault(true);
	commentB->setFixedWidth(55);
	commentB->setToolTip(tr("DA☆ZE!"));
	layout->addWidget(commentB);
	commentA=new QAction(this);
	commentA->setShortcut(QKeySequence("Ctrl+Enter"));
	connect(commentB,&QPushButton::clicked,commentA,&QAction::trigger);
	connect(commentL,&QLineEdit::returnPressed,commentA,&QAction::trigger);
	connect(commentA,&QAction::triggered,[this](){
		if(!commentL->text().isEmpty()){
			postComment();
			commentL->clear();
			hide();
		}
	});
	connect(Danmaku::instance(),&Danmaku::modelReset,[this](){
		commentS->clear();
		for(const Record *r:getRecords()){
			commentS->addItem(QFileInfo(r->source).baseName(),(quintptr)r);
		}
		if(commentS->count()==0){
			hide();
		}
		commentS->setVisible(commentS->count()>=2);
	});
}

bool Post::eventFilter(QObject *,QEvent *e)
{
	switch(e->type()){
	case QEvent::Move:
	case QEvent::Resize:
		moveWithParent();
		return false;
	default:
		return false;
	}
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
	commentC->setStyleSheet(QString("background-color:%1").arg(color.name()));
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
	if(Danmaku::instance()->appendToPool(r->source,c)){
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
			reply->deleteLater();
		});
	}
}

void Post::moveWithParent()
{
	QRect p=parentWidget()->geometry(),c=geometry();
	move(p.center().x()-c.width()/2,p.bottom()-c.height()-2);
}
