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

QHash<int,int> Poster::mode={
	{0,5},
	{1,1},
	{2,4}
};

Poster::Poster(QWidget *parent) :
	QWidget(parent)
{
	ioo=0;
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
	auto layout=new QHBoxLayout(this);
	layout->setMargin(0);
	layout->setSpacing(0);
	commentM=new QComboBox(this);
	commentM->addItems(QStringList({tr("Top"),tr("Slide"),tr("Bottom")}));
	commentM->setCurrentIndex(1);
	commentM->setFixedWidth(commentM->sizeHint().width());
	layout->addWidget(commentM);
	commentC=new QPushButton(this);
	commentC->setFixedWidth(25);
	setColor(Qt::white);
	connect(commentC,&QPushButton::clicked,[this](){
		QColor color=QColorDialog::getColor(getColor(),parentWidget());
		if(color.isValid()) setColor(color);
	});
	layout->addWidget(commentC);
	commentL=new QLineEdit(this);
	layout->addWidget(commentL);
	commentB=new QPushButton(tr("Post"),this);
	commentB->setFixedWidth(55);
	commentB->setToolTip("毁灭地喷射白光!da!");
	layout->addWidget(commentB);
	commentA=new QAction(this);
	connect(commentB,&QPushButton::clicked,commentA,&QAction::trigger);
	connect(commentL,&QLineEdit::returnPressed,commentA,&QAction::trigger);
	connect(commentA,&QAction::triggered,[this](){
		if(!commentL->text().isEmpty()){
			postComment(commentL->text());
			commentL->setText("");
		}
	});
	hide();
}

bool Poster::isValid()
{
	return !getCid().isEmpty();
}

void Poster::postComment(QString comment)
{
	QString cid=getCid();
	if(!cid.isEmpty()){
		Comment c;
		c.mode=mode[commentM->currentIndex()];
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
				Danmaku::instance()->appendToCurrent(c);
			}
		});
	}
	else{
		QMessageBox::warning(this,tr("Warning"),tr("Empty cid."));
	}
}

void Poster::fadeIn()
{
	if(ioo==0){
		ioo=1;
		timer->start(20);
		show();
	}
}

void Poster::fadeOut()
{
	if(ioo==2){
		ioo=3;
		timer->start(20);
	}
}

QString Poster::getCid()
{
	for(const Record &r:Danmaku::instance()->getPool()){
		if(r.source.startsWith("http://comment.bilibili.tv/")){
			return QFileInfo(r.source).baseName();
		}
	}
	return QString();
}

QColor Poster::getColor()
{
	QString sheet=commentC->styleSheet();
	return QColor(sheet.mid(sheet.indexOf('#')));
}

void Poster::setColor(QColor color)
{
	commentC->setStyleSheet(QString("background-color:%1").arg(color.name()));
}
