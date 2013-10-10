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

QHash<int,QString> Search::AcFunChannel={
	{1,"动画"},
	{106,"动画短片"},
	{107,"MAD·AMV"},
	{108,"MMD·3D"},
	{67,"新番连载"},
	{109,"动画合集"},
	{58,"音乐"},
	{101,"演唱·乐器"},
	{102,"宅舞"},
	{103,"Vocaloid"},
	{105,"流行音乐"},
	{104,"ACG音乐"},
	{59,"游戏"},
	{83,"游戏集锦"},
	{84,"实况解说"},
	{71,"Flash游戏"},
	{72,"Mugen"},
	{85,"英雄联盟"},
	{60,"娱乐"},
	{86,"生活娱乐"},
	{87,"鬼畜调教"},
	{88,"萌宠"},
	{89,"美食"},
	{70,"科技"},
	{90,"科普"},
	{91,"数码"},
	{92,"军事"},
	{69,"体育"},
	{93,"惊奇体育"},
	{94,"足球"},
	{95,"篮球"},
	{68,"影视"},
	{96,"电影"},
	{97,"剧集"},
	{98,"综艺"},
	{99,"特摄·霹雳"},
	{100,"纪录片"},
	{63,"文章"},
	{110,"文章综合"},
	{73,"工作·情感"},
	{74,"动漫文化"},
	{75,"漫画·小说"},
	{76,"页游资料"},
	{77,"1区"},
	{78,"21区"},
	{79,"31区"},
	{80,"41区"},
	{81,"文章里区(不审)"},
	{82,"视频里区(不审)"},
	{42,"图库"}
};
#define tr
QVector<const char *> Search::AcOrder={
	tr("ranklevel"),
	tr("click"),
	tr("pubdate"),
	tr("scores"),
	tr("stow")
};
QVector<const char *> Search::BiOrder={
	tr("default"),
	tr("pubdate"),
	tr("senddate"),
	tr("ranklevel"),
	tr("click"),
	tr("scores"),
	tr("dm"),
	tr("stow")
};
#undef tr

Search::Search(QWidget *parent):QDialog(parent)
{
	auto outerLayout=new QVBoxLayout;
	auto keywdLayout=new QHBoxLayout;
	statusL=new QLabel(tr("Ready"),this);
	pageTxL=new QLabel(tr("Page"),this);
	pageNuL=new QLabel(this);
	pageNuL->setFixedWidth(40);
	keywE=new QLineEdit(this);
	pageE=new QLineEdit(this);
	pageE->setFixedWidth(40);
	keywdLayout->addWidget(keywE);

	orderC=new QComboBox(this);
	orderC->setEditable(false);
	sitesC=new QComboBox(this);
	QStringList sites={"Bilibili","AcFun"};
	sitesC->addItems(sites);
	sitesC->setEditable(false);
	keywdLayout->addWidget(orderC);
	keywdLayout->addWidget(sitesC);

	searchB=new QPushButton(this);
	searchB->setText(tr("Search"));
	keywdLayout->addWidget(searchB);

	outerLayout->addLayout(keywdLayout);

	pageGoB=new QPushButton(tr("Goto"),this);
	pageUpB=new QPushButton(tr("PgUp"),this);
	pageDnB=new QPushButton(tr("PgDn"),this);

	resultW=new QTreeWidget(this);
	resultW->setIconSize(QSize(120,90));
	outerLayout->addWidget(resultW);
	setSite();
	int widthHint=qMax(orderC->sizeHint().width(),sitesC->sizeHint().width());
	orderC->setFixedWidth(widthHint);
	sitesC->setFixedWidth(widthHint);

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

	connect(sitesC,&QComboBox::currentTextChanged,[this](QString){
		setSite();
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
		QString url=reply->url().url();
		if(reply->error()==QNetworkReply::NoError){
			if(url.startsWith("http://www.bilibili.tv")){
				QString data(reply->readAll());
				int sta,end;
				if(pageNum==-1){
					sta=data.indexOf("<div class=\"pagelistbox\"><span>");
					end=data.indexOf("</div>",sta);
					QString page=data.mid(sta,end-sta+6);
					sta=page.indexOf("<a class=\"endPage\"");
					sta=page.indexOf("page=",sta)+5;
					end=page.indexOf("\">",sta);
					pageNum=page.mid(sta,end-sta).toInt();
					pageNuL->setText(QString("/%1").arg(pageNum));
				}
				QStringList ary=data.split("<li class=\"l\">",QString::SkipEmptyParts);
				if(ary.size()>=2){
					QString &item=ary.last();
					item.truncate(item.indexOf("</li>")+5);
					ary.removeFirst();
				}
				for(QString item:ary){
					item=item.simplified();
					sta=item.indexOf("http://www.bilibili.tv/video/");
					if(sta!=-1){
						sta+=29;
						end=item.indexOf("/\"",sta);
						QTreeWidgetItem *row=new QTreeWidgetItem(resultW);
						row->setData(0,Qt::UserRole,item.mid(sta,end-sta));
						row->setSizeHint(0,QSize(120,92));
						sta=item.indexOf("<img src=",end)+10;
						end=item.indexOf("\" ",sta);
						QNetworkRequest request(QUrl(item.mid(sta,end-sta)));
						request.setAttribute(QNetworkRequest::User,resultW->invisibleRootItem()->childCount()-1);
						reply->manager()->get(request);
						sta=item.indexOf("<span>[",end)+7;
						end=item.indexOf("]</span>",sta);
						row->setText(4,item.mid(sta,end-sta));
						sta=end+8;
						end=item.indexOf("</div>",sta);
						row->setText(3,item.mid(sta,end-sta).remove(QRegularExpression("<.*>")));
						sta=item.indexOf("class=\"upper\"",end);
						sta=item.indexOf("\">",sta)+2;
						end=item.indexOf("</a>",sta);
						row->setText(5,item.mid(sta,end-sta));
						auto iter=QRegularExpression("\\d+").globalMatch(item.mid(end));
						row->setText(1,iter.next().captured());
						iter.next();
						row->setText(2,iter.next().captured());
					}
				}
				statusL->setText(tr("Finished"));
				isWaiting=false;
			}
			else if(url.startsWith("http://www.acfun.tv")){
				QJsonObject json=QJsonDocument::fromJson(reply->readAll()).object();
				QJsonObject page=json["page"].toObject();
				if(pageNum==-1){
					pageNum=page["totalPage"].toDouble();
					pageNuL->setText(QString("/%1").arg(pageNum));
				}
				QJsonArray ary=json["contents"].toArray();
				for(int i=0;i<ary.count();++i){
					QJsonObject item=ary[i].toObject();
					if(item["url"].toString().startsWith("/v/")){
						QStringList content={
							"",
							QString::number(item["views"].toDouble()),
							QString::number(item["comments"].toDouble()),
							item["title"].toString(),
							AcFunChannel[item["channelId"].toDouble()],
							item["username"].toString()
						};
						QTreeWidgetItem *row=new QTreeWidgetItem(resultW,content);
						row->setData(0,Qt::UserRole,item["url"].toString().mid(3));
						row->setSizeHint(0,QSize(120,92));
						QNetworkRequest request(QUrl(item["titleImg"].toString()));
						request.setAttribute(QNetworkRequest::User,resultW->invisibleRootItem()->childCount()-1);
						reply->manager()->get(request);
					}
				}
				statusL->setText(tr("Finished"));
				isWaiting=false;
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
		else if(url.startsWith("http://api.bilibili.tv")||url.startsWith("http://www.acfun.tv")){
			error(reply->error());
		}
	});
}

void Search::setKey(QString _key)
{
	key=_key;
	keywE->setText(key);
	searchB->click();
}

void Search::setSite()
{
	QStringList orders;
	int s=sitesC->currentIndex();
	for(const char *iter:(s==0?BiOrder:AcOrder)){
		orders.append(tr(iter));
	}
	QStringList header={tr("Cover"),tr("Play"),s==0?tr("Danmaku"):tr("Comment"),tr("Title"),tr("Typename"),tr("Author")};
	isWaiting=true;
	orderC->clear();
	orderC->addItems(orders);
	orderC->setCurrentIndex(0);
	resultW->setHeaderLabels(header);
	isWaiting=false;
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
	QUrl url;
	if(sitesC->currentIndex()==0){
		url=QUrl("http://www.bilibili.tv/search");
		QUrlQuery query;
		query.addQueryItem("keyword",key);
		query.addQueryItem("page",QString::number(pageNum));
		query.addQueryItem("orderby",BiOrder[orderC->currentIndex()]);
		query.addQueryItem("pagesize","20");
		url.setQuery(query);
	}
	else{
		url=QUrl("http://www.acfun.tv/api/search.aspx");
		QUrlQuery query;
		query.addQueryItem("query",key);
		query.addQueryItem("exact","1");
		query.addQueryItem("orderId",QString::number(orderC->currentIndex()));
		query.addQueryItem("orderBy","1");
		query.addQueryItem("pageNo",QString::number(pageNum));
		query.addQueryItem("pageSize","20");
		url.setQuery(query);
	}
	manager->get(QNetworkRequest(url));
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
