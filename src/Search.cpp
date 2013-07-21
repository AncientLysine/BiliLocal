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

Search::Search(QWidget *parent):QDialog(parent)
{
	data=QSqlDatabase::database();
	data.open();
	QSqlQuery query;
	query.exec("PRAGMA synchronous = OFF;");
	auto outerLayout=new QVBoxLayout;
	auto keywdLayout=new QHBoxLayout;
	statusL=new QLabel(tr("Ready"),this);
	if(Utils::getConfig("/Playing/Appkey",QString("0"))=="0"){
		statusL->setText(tr("<font color=red>Empty Appkey<font>"));
	}
	pageTxL=new QLabel(tr("Page"),this);
	pageNuL=new QLabel(this);
	pageNuL->setFixedWidth(40);
	keywE=new QLineEdit(this);
	pageE=new QLineEdit(this);
	pageE->setFixedWidth(40);
	orderC=new QComboBox(this);
	QStringList orders={tr("default"),tr("pubdate"),tr("senddate"),tr("ranklevel"),tr("click"),tr("scores"),tr("danmaku"),tr("stow")};
	orderC->addItems(orders);
	orderC->setEditable(false);
	searchB=new QPushButton(this);
	searchB->setText(tr("Search"));
	keywdLayout->addWidget(keywE);
	keywdLayout->addWidget(orderC);
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
	Utils::setCenter(this);
	
	QStringList labels={tr("Cover"),tr("Play"),tr("Danmaku"),tr("Title"),tr("Typename"),tr("Author")};
	resultW->setHeaderLabels(labels);
	resultW->setSelectionMode(QAbstractItemView::SingleSelection);
	resultW->setColumnWidth(0,165);
	resultW->setColumnWidth(1,60);
	resultW->setColumnWidth(2,60);
	resultW->setColumnWidth(3,370);
	resultW->setColumnWidth(4,100);

	connect(orderC,&QComboBox::currentTextChanged,[this](QString){
		if(!isWaiting&&resultW->topLevelItemCount()>0){
			searchB->click();
		}
	});

	connect(searchB,&QPushButton::clicked,[this](){
		if(isWaiting){
			QMessageBox::warning(this,tr("Warning"),tr("A request is pending."));
		}
		else if(!keywE->text().isEmpty()){
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
		auto error=[this](int code){
			QString info=tr("Network error occurred, error code: %1");
			QMessageBox::warning(this,tr("Network Error"),info.arg(code));
			clearSearch();
			isWaiting=false;
		};
		if (reply->error()==QNetworkReply::NoError) {
			QJsonObject json=QJsonDocument::fromJson(reply->readAll()).object();
			int code=json["code"].toDouble();
			if(code==0){
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
						row->setSizeHint(0,QSize(120,92));
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
							query.addBindValue(QDateTime::currentDateTime().toTime_t());
							query.addBindValue(item["pic"].toString());
							query.exec();
						}
						else{
							int index=resultW->invisibleRootItem()->childCount()-1;
							auto getPic=new QNetworkAccessManager(this);
							connect(getPic,&QNetworkAccessManager::finished,[=](QNetworkReply *reply){
								if (reply->error()==QNetworkReply::NoError){
									QSqlQuery query;
									QByteArray byte=reply->readAll();
									query.prepare("INSERT INTO Cache VALUES(?,?,?);");
									query.addBindValue(reply->url().toString());
									query.addBindValue(QDateTime::currentDateTime().toTime_t());
									query.addBindValue(byte);
									query.exec();
									query.prepare("SELECT COUNT(*) FROM Cache;");
									query.exec();
									if(query.first()&&query.value(0).toInt()>200){
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
								reply->manager()->deleteLater();
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
				if(json["error"].toString()=="overspeed"){
					Utils::delayExec(2000,[this](){getData(pageCur);});
					isWaiting=true;
				}
				else{
					error(-code);
				}
			}
		}
		else {
			error(reply->error());
		}
	});

	auto quitSC=new QShortcut(this);
	quitSC->setKey(QString("Ctrl+Q"));
	connect(quitSC,&QShortcut::activated,this,&QWidget::close);
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
	QString order[]={"default","pubdate","senddate","ranklevel","click","scores","damku","stow"};
	QString apiUrl("http://api.bilibili.tv/search?type=json&appkey=%1&keyword=%2&page=%3&order=%4");
	request.setUrl(QUrl(apiUrl.arg(Utils::getConfig("/Playing/Appkey",QString("0"))).arg(key).arg(pageNum).arg(order[orderC->currentIndex()])));
	manager->get(request);
}

void Search::initDataBase()
{
	QSqlDatabase data=QSqlDatabase::addDatabase("QSQLITE");
	bool exists=QFile::exists("Cache.db");
	data.setDatabaseName("Cache.db");
	data.open();
	QSqlQuery query;
	if(!exists){
		query.exec("PRAGMA auto_vacuum = 1;");
		query.exec("CREATE TABLE Cache ( Url VARCHAR(128), Time INTEGER, Pixmap BLOB, PRIMARY KEY(Url) );");
	}
	data.close();
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
