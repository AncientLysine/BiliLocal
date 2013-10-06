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
	auto outerLayout=new QVBoxLayout;
	auto keywdLayout=new QHBoxLayout;
	statusL=new QLabel(tr("Ready"),this);
	if(Utils::getConfig("/Playing/Appkey",QString()).isEmpty()){
		QString warning=tr("<font color=red>Empty Appkey<font>");
		statusL->setText(warning);
		QMessageBox::warning(parentWidget(),tr("Warning"),warning);
		Config config(parentWidget(),1);
		config.exec();
		Danmaku::instance()->parse(0x2|0x4);
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
		QTreeWidgetItem *item=resultW->currentItem();
		if(item!=NULL){
			aid=item->data(0,Qt::UserRole).toString();
			accept();
		}
		else{
			QMessageBox::warning(this,tr("Warning"),tr("No video has been chosen."));
		}
	});

	connect(ccB,&QPushButton::clicked,this,&QDialog::reject);

	connect(resultW,&QTreeWidget::itemActivated,okB,&QPushButton::clicked);

	cache=new QNetworkDiskCache(this);
	cache->setCacheDirectory("./cache");

	manager=new QNetworkAccessManager(this);
	manager->setCache(cache);
	connect(manager,&QNetworkAccessManager::finished,[this](QNetworkReply *reply){
		auto error=[this](int code){
			QString info=tr("Network error occurred, error code: %1");
			QMessageBox::warning(this,tr("Network Error"),info.arg(code));
			clearSearch();
			isWaiting=false;
		};
		if(reply->error()==QNetworkReply::NoError){
			if(reply->url().url().startsWith("http://api.bilibili.tv")){
				QJsonObject json=QJsonDocument::fromJson(reply->readAll()).object();
				int code=json["code"].toDouble();
				if(code==0){
					if(pageNum==-1){
						pageNum=json["page"].toDouble();
						pageNuL->setText(QString("/%1").arg(pageNum));
					}
					QJsonObject map=json["result"].toObject();
					for(int i=0;i<map.count();++i){
						QJsonObject item=map[QString::number(i)].toObject();
						if(item["type"].toString()==QString("video")){
							QStringList content={
								"",
								item["play"].toString(),
								item["video_review"].toString(),
								item["title"].toString(),
								item["typename"].toString(),
								item["author"].toString()
							};
							QTreeWidgetItem *row=new QTreeWidgetItem(resultW,content);
							row->setData(0,Qt::UserRole,item["aid"].toString());
							row->setSizeHint(0,QSize(120,92));
							QNetworkRequest request(QUrl(item["pic"].toString()));
							request.setAttribute(QNetworkRequest::User,resultW->invisibleRootItem()->childCount()-1);
							reply->manager()->get(request);
						}
					}
					statusL->setText(tr("Finished"));
					isWaiting=false;
				}
				else{
					if(json["error"].toString()=="overspeed"){
						QPointer<Search> ptr(this);
						Utils::delayExec(2000,[ptr](){
							if(!ptr.isNull()){
								ptr->getData(ptr->pageCur);
							}
						});
						isWaiting=true;
					}
					else{
						error(-code);
					}
				}
			}
			else{
				QTreeWidgetItem *line=resultW->topLevelItem(reply->request().attribute(QNetworkRequest::User).toInt());
				if(line!=NULL&&line->icon(0).isNull()){
					QPixmap pixmap;
					pixmap.loadFromData(reply->readAll());
					if(!pixmap.isNull()){
						pixmap=pixmap.scaled(120,90,Qt::IgnoreAspectRatio,Qt::SmoothTransformation);
					}
					line->setIcon(0,QIcon(pixmap));
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
	resultW->clear();
	isWaiting=true;
	statusL->setText(tr("Requesting"));
	QNetworkRequest request;
	QString order[]={"default","pubdate","senddate","ranklevel","click","scores","dm","stow"};
	QString apiUrl("http://api.bilibili.tv/search?type=json&appkey=%1&keyword=%2&page=%3&order=%4");
	request.setUrl(QUrl(apiUrl.arg(Utils::getConfig("/Playing/Appkey",QString())).arg(key).arg(pageNum).arg(order[orderC->currentIndex()])));
	manager->get(request);
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
