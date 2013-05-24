/*=======================================================================
*
*   Copyright (C) 2013 Lysine.
*
*   Filename:    Poster.cpp
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

#include "Poster.h"

Poster::Poster(QWidget *parent) :
	QWidget(parent)
{
	ioo=0;
	rnd=qrand()%100000000;
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
	commentL=new QLineEdit(this);
	commentB=new QPushButton(tr("Post"),this);
	commentA=new QAction(this);
	connect(commentB,&QPushButton::clicked,commentA,&QAction::trigger);
	connect(commentL,&QLineEdit::returnPressed,commentA,&QAction::trigger);
	connect(commentA,&QAction::triggered,[this](){
		if(!commentL->text().isEmpty()){
			postComment(commentL->text());
			commentL->setText("");
		}
	});
}

void Poster::setDanmaku(Danmaku *value)
{
	danmaku=value;
}

void Poster::setVplayer(VPlayer *value)
{
	vplayer=value;
}

void Poster::resizeEvent(QResizeEvent *e)
{
	int w=e->size().width(),h=e->size().height();
	commentL->setGeometry(0, 0,w-70,h);
	commentB->setGeometry(w-60,0,60,h);
}

void Poster::postComment(QString comment)
{
	QString cid=danmaku->getCid();
	if (!cid.isEmpty()){
		QNetworkRequest request(QUrl("http://interface.bilibili.tv/dmpost"));
		request.setHeader(QNetworkRequest::ContentTypeHeader,"application/x-www-form-urlencoded");

		QUrlQuery params;
		params.addQueryItem("cid",cid);
		params.addQueryItem("date",QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));
		params.addQueryItem("pool","0");
		params.addQueryItem("playTime",QString::number(qMax(qint64(0),vplayer->getTime())/1000.0,'f',4));
		params.addQueryItem("color","16777215");
		params.addQueryItem("fontsize","25");
		params.addQueryItem("message",comment);
		params.addQueryItem("rnd",QString::number(rnd));
		params.addQueryItem("mode","1");

		manager->post(request,QUrl::toPercentEncoding(params.query(QUrl::FullyEncoded),"%=&","-.~_"));
	}
	else{
		QMessageBox::warning(this,tr("Warning"),tr("Empty cid."));
	}
}

void Poster::fadeIn()
{
	if(ioo==0){
		ioo=1;
		timer->start(50);
		show();
	}
}

void Poster::fadeOut()
{
	if(ioo==2){
		ioo=3;
		timer->start(50);
	}
}

