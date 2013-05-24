#include "Poster.h"

Poster::Poster(QWidget *parent) :
	QWidget(parent)
{
	rnd = qrand()%100000000;
	manager = new QNetworkAccessManager(this);
	commentL = new QLineEdit(this);
	commentB = new QPushButton(tr("Post"),this);
	commentA = new QAction(this);
	connect(commentB,&QPushButton::clicked, commentA,&QAction::trigger);
	connect(commentL,&QLineEdit::returnPressed, commentA,&QAction::trigger);
	connect(commentA,&QAction::triggered,[this](){
		postComment();
		commentL->setText("");
	});
}

void Poster::setDanmaku(Danmaku *value)
{
	danmaku = value;
}

void Poster::setVplayer(VPlayer *value)
{
	vplayer = value;
}

void Poster::resizeEvent(QResizeEvent *e)
{
	int w=e->size().width(),h=e->size().height();
	commentL->setGeometry(0, 0,w-70,h);
	commentB->setGeometry(w-60,0,60,h);
}

void Poster::postComment(){
	QUrl url("http://interface.bilibili.tv/dmpost");
	QNetworkRequest request(url);
	request.setHeader(QNetworkRequest::ContentTypeHeader,"application/x-www-form-urlencoded");

	QByteArray data;
	QUrlQuery params;

	QString cid = danmaku->getCid();
	if (!cid.isEmpty()){
		params.addQueryItem("cid",cid);

		QDateTime dt;
		QTime time;
		QDate date;
		dt.setTime(time.currentTime());
		dt.setDate(date.currentDate());
		params.addQueryItem("date",dt.toString("yyyy-MM-dd hh:mm:ss"));
		params.addQueryItem("pool","0");
		params.addQueryItem("playTime",QString::number(vplayer->getTime()/1000.0,'f',4));
		params.addQueryItem("color","16777215");//white color
		params.addQueryItem("fontsize","25");
		params.addQueryItem("message",commentL->text());
		params.addQueryItem("rnd",QString::number(rnd));
		params.addQueryItem("mode","1");

		data = QUrl::toPercentEncoding(params.query(QUrl::FullyEncoded),"%=&","-.~_");
		manager->post(request, data);
	}
	else{
		QMessageBox::warning(this,tr("Warning"),tr("Empty cid."));
	}
}
