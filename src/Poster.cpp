#include "Poster.h"

Poster::Poster(QWidget *parent) :
    QWidget(parent)
{
    commentL = new QLineEdit(this);
    commentB = new QPushButton(this);
    commentB->setText(tr("post"));
    commentA = new QAction(this);
    manager=new QNetworkAccessManager(this);

    rnd = qrand()%100000000;

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
void Poster::setInfo(Info *value)
{
    info = value;
}


void Poster::resizeEvent(QResizeEvent *)
{
    commentL->setGeometry(QRect(5,3,this->width()-70,25));
    commentB->setGeometry(QRect(this->width()-60, 3, 55, 25));
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
        params.addQueryItem("playTime",QString::number(info->getTime()*1.0/1000,'f',4));
        params.addQueryItem("color","16777215");//white color
        params.addQueryItem("fontsize","25");
        params.addQueryItem("message",commentL->text());
        params.addQueryItem("rnd",QString::number(rnd));
        params.addQueryItem("mode","1");

        data = QUrl::toPercentEncoding(params.query(QUrl::FullyEncoded),"%=&","-.~_");
        qDebug()<<"POST:"<<data;
        manager->post(request, data);
    }
    else{
        QMessageBox::warning(this,tr("Warning"),tr("Danmaku post failed."));
        qDebug()<<"Attending to post but danmaku not loaded";
    }
}
