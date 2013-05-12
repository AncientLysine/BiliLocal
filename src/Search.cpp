/*=======================================================================
*
*   Copyright (C) 2013 Lysine.
*
*   Filename:    Search.cpp
*   Time:        2013/04/18
*   Author:      Chaserhkj
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

#include "Search.h"

QSqlDatabase Search::data;

Search::Search(QWidget *parent):QDialog(parent)
{
	bool exists=QFile::exists("Cache.db");
	data.setDatabaseName("Cache.db");
	data.open();
	QSqlQuery query;
	if(!exists){
		query.exec("PRAGMA auto_vacuum = 1;");
		query.exec("CREATE TABLE Cache ( Url VARCHAR(128), Time INTEGER, Pixmap BLOB, PRIMARY KEY(Url) );");
	}
	query.exec("PRAGMA synchronous = OFF;");
	auto outerLayout=new QVBoxLayout;
	auto keywdLayout=new QHBoxLayout;
	statusL=new QLabel(tr("Ready"),this);
	pageTxL=new QLabel(tr("Page"),this);
	pageNuL=new QLabel(this);
	pageNuL->setFixedWidth(40);
	keywE=new QLineEdit(this);
	pageE=new QLineEdit(this);
	pageE->setFixedWidth(40);
	searchB=new QPushButton(this);
	searchB->setText(tr("Search"));
	keywdLayout->addWidget(keywE);
	keywdLayout->addWidget(searchB);
	outerLayout->addLayout(keywdLayout);

	pageGoB=new QPushButton(tr("Goto"),this);
	pageUpB=new QPushButton(tr("PgUp"),this);
	pageDnB=new QPushButton(tr("PgDn"),this);

	resultW=new QTreeWidget(this);
	resultW->setIconSize(QSize(120,90));
	outerLayout->addWidget(resultW);

	auto pageLayout=new QHBoxLayout;
	pageLayout->addWidget(statusL);
	pageLayout->addStretch();
	pageLayout->addWidget(pageTxL);
	pageLayout->addWidget(pageE);
	pageLayout->addWidget(pageNuL);
	pageLayout->addWidget(pageGoB);
	pageLayout->addWidget(pageUpB);
	pageLayout->addWidget(pageDnB);
	outerLayout->addLayout(pageLayout);

	auto responseLayout=new QHBoxLayout;
	okB=new QPushButton(tr("Confirm"),this);
	ccB=new QPushButton(tr("Cancel"), this);
	responseLayout->addWidget(okB);
	responseLayout->addStretch();
	responseLayout->addWidget(ccB);
	outerLayout->addLayout(responseLayout);

	setLayout(outerLayout);
	setWindowTitle(tr("Search"));
	resize(900,520);
	
	QStringList labels={tr("Cover"),tr("Play"),tr("Danmaku"),tr("Title"),tr("Typename"),tr("Author")};
	resultW->setHeaderLabels(labels);
	resultW->setSelectionMode(QAbstractItemView::SingleSelection);
	resultW->setColumnWidth(0,165);
	resultW->setColumnWidth(1,60);
	resultW->setColumnWidth(2,60);
	resultW->setColumnWidth(3,370);
	resultW->setColumnWidth(4,100);

	connect(searchB,&QPushButton::clicked,[this](){
		if(isWaiting){
			QMessageBox::warning(this,tr("Warning"),tr("A request is pending."));
		}
		else if(!key.isEmpty()){
			clearSearch();
			startSearch();
		}
	});

	auto jump=[this](int page){
		if(pageNum==-1) {
			QMessageBox::warning(this,tr("Warning"),tr("No search in progress."));
		}
		else if(isWaiting) {
			QMessageBox::warning(this,tr("Warning"),tr("A request is pending."));
		}
		else if(page<1||page>pageNum) {
			QMessageBox::warning(this,tr("Warning"),tr("Page num out of range."));
		}
		else{
			pageCur=page;
			getData(page);
			pageE->setText(QString::number(pageCur));
		}
	};

	connect(pageGoB,&QPushButton::clicked,[jump,this](){jump(pageE->text().toInt());});
	connect(pageUpB,&QPushButton::clicked,[jump,this](){jump(pageCur-1);});
	connect(pageDnB,&QPushButton::clicked,[jump,this](){jump(pageCur+1);});

	connect(okB,&QPushButton::clicked,[this](){
		if(resultW->currentIndex().isValid()){
			aid=temp[resultW->currentIndex().row()];
			accept();
		}
		else{
			QMessageBox::warning(this,tr("Warning"),tr("No video has been chosen."));
		}
	});

	connect(ccB,&QPushButton::clicked,this,&QDialog::reject);

	connect(resultW,&QTreeWidget::itemActivated,okB,&QPushButton::clicked);

	manager=new QNetworkAccessManager(this);
	connect(manager,&QNetworkAccessManager::finished,[this](QNetworkReply *reply){
		if (reply->error()==QNetworkReply::NoError) {
			QJsonObject json=QJsonDocument::fromJson(reply->readAll()).object();
			if(static_cast<int>(json["code"].toDouble())==0){
				if (pageNum==-1) {
					pageNum=json["page"].toDouble();
					pageNuL->setText(QString("/%1").arg(pageNum));
				}
				QJsonObject results=json["result"].toObject();
				for(auto record:results) {
					QJsonObject item=record.toObject();
					if (item["type"].toString()==QString("video")) {
						QStringList content={
							"",
							item["play"].toString(),
							item["video_review"].toString(),
							item["title"].toString(),
							item["typename"].toString(),
							item["author"].toString()
						};
						temp.append(item["aid"].toString());
						QSqlQuery query;
						query.prepare("SELECT Pixmap FROM Cache Where Url=?;");
						query.addBindValue(item["pic"].toString());
						query.exec();
						QTreeWidgetItem *row=new QTreeWidgetItem(resultW,content);
						auto time=[](){
							quint64 _time=0;
							_time+=QDate(2000,1,1).daysTo(QDate::currentDate());
							_time*=24*60*60;
							_time+=QTime(0,0,1).secsTo(QTime::currentTime())+1;
							return _time;
						};
						auto loadPixmap=[](QByteArray data){
							QPixmap pixmap;
							pixmap.loadFromData(data);
							if(!pixmap.isNull()){
								pixmap=pixmap.scaled(120,90,Qt::IgnoreAspectRatio,Qt::SmoothTransformation);
							}
							return pixmap;
						};
						if(query.first()){
							row->setIcon(0,QIcon(loadPixmap(query.value("Pixmap").toByteArray())));
							query.prepare("UPDATE Cache SET Time=? WHERE Url=?");
							query.addBindValue(time());
							query.addBindValue(item["pic"].toString());
						}
						else{
							int index=resultW->invisibleRootItem()->childCount()-1;
							auto getPic=new QNetworkAccessManager(this);
							connect(getPic,&QNetworkAccessManager::finished,[=](QNetworkReply *reply){
								if (reply->error()==QNetworkReply::NoError){
									QByteArray byte=reply->readAll();
									QSqlQuery query;
									query.prepare("INSERT INTO Cache VALUES(?,?,?);");
									query.addBindValue(reply->url().toString());
									query.addBindValue(time());
									query.addBindValue(byte);
									query.exec();
									auto size=[](){
										QFile file("Cache.db");
										file.open(QIODevice::ReadOnly);
										qint64 _size=file.size();
										file.close();
										return _size;
									};
									while(size()>2*1024*1024){
										query.prepare("DELETE FROM Cache "
													  "WHERE Cache.Time=("
													  "SELECT MIN(Time) FROM Cache);");
										query.exec();
									}
									auto line=resultW->topLevelItem(index);
									if(line!=NULL&&line->icon(0).isNull()){
										line->setIcon(0,QIcon(loadPixmap(byte)));
									}
								}
							});
							QNetworkRequest request;
							request.setUrl(QUrl(item["pic"].toString()));
							getPic->get(request);
						}
					}
				}
				statusL->setText(tr("Finished"));
				isWaiting=false;
			}
			else{
				auto timer=new QTimer(this);
				timer->setSingleShot(true);
				connect(timer,&QTimer::timeout,[this](){getData(pageCur);});
				timer->start(2000);
				isWaiting=true;
			}
		}
		else {
			QString info=tr("Network error occurred, error code: %1");
			QMessageBox::warning(this,tr("Network Error"),info.arg(reply->error()));
			clearSearch();
			isWaiting=false;
		}
	});

	auto quitSC=new QShortcut(this);
	quitSC->setKey(QString("Ctrl+Q"));
	connect(quitSC,&QShortcut::activated,this,&QWidget::close);
}

Search::~Search()
{
	data.close();
}

void Search::setKey(QString _key)
{
	key=_key;
	keywE->setText(key);
	searchB->click();
}

void Search::startSearch()
{
	key=keywE->text();
	pageCur=1;
	pageE->setText("1");
	getData(1);
}

void Search::getData(int pageNum)
{
	temp.clear();
	resultW->clear();
	isWaiting=true;
	statusL->setText(tr("Requesting"));
	QNetworkRequest request;
	QString apiUrl("http://api.bilibili.tv/search?type=json&keyword=%1&page=%2&order=default");
	request.setUrl(QUrl(apiUrl.arg(key).arg(pageNum)));
	manager->get(request);
}

void Search::initDataBase()
{
	data=QSqlDatabase::addDatabase("QSQLITE");
}

void Search::clearSearch()
{
	resultW->clear();
	pageNum=-1;
	pageCur=-1;
	pageE->setText("");
	pageNuL->setText("");
	statusL->setText(tr("Ready"));
}
