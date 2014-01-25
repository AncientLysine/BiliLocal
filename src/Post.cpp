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
	QDialog(parent)
{
	setMinimumSize(400,300);
	setWindowTitle(tr("Post"));
	manager=new QNetworkAccessManager(this);
	manager->setCookieJar(Cookie::instance());
	Cookie::instance()->setParent(NULL);
	auto layout=new QGridLayout(this);
	layout->setMargin(0);
	layout->setSpacing(0);
	layout->setRowStretch(0,5);
	commentP=new QLabel(this);
	layout->addWidget(commentP,0,0,1,4);
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
	layout->addWidget(commentL,1,2);
	commentB=new QPushButton(tr("Post"),this);
	commentB->setDefault(true);
	commentB->setFixedWidth(55);
	commentB->setToolTip(tr("DA☆ZE!"));
	layout->addWidget(commentB,1,3);
	commentA=new QAction(this);
	commentA->setShortcut(QKeySequence("Ctrl+Enter"));
	connect(commentB,&QPushButton::clicked,commentA,&QAction::trigger);
	connect(commentL,&QLineEdit::returnPressed,commentA,&QAction::trigger);
	connect(commentA,&QAction::triggered,[this](){
		if(!commentL->text().isEmpty()){
			postComment();
			accept();
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

const Record *Post::getBilibili()
{
	for(const Record &r:Danmaku::instance()->getPool()){
		if(r.source.startsWith("http://comment.bilibili.tv/")){
			return &r;
		}
	}
	return NULL;
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
	const Record *r=getBilibili();
	if(r!=NULL){
		const Comment &c=getComment();
		QNetworkRequest request(QUrl("http://interface.bilibili.tv/dmpost"));
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
			reply->deleteLater();
		});
	}
	else{
		QMessageBox::warning(this,tr("Warning"),tr("Empty cid."));
	}
}
