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
#include "APlayer.h"
#include "Config.h"
#include "Local.h"
#include "Utils.h"

namespace
{
QHash<int,QString> getChannel(QString name)
{
	static QHash<QString,QHash<int,QString>> m;
	if(!m.contains(name)){
		QFile file(":/Text/DATA");
		file.open(QIODevice::ReadOnly|QIODevice::Text);
		QJsonObject data=QJsonDocument::fromJson(file.readAll()).object()[name+"Channel"].toObject();
		for(auto iter=data.begin();iter!=data.end();++iter){
			m[name][iter.key().toInt()]=iter.value().toString();
		}
	}
	return m[name];
}
}

Search::Search(QWidget *parent):QDialog(parent)
{
	double x=logicalDpiX()/72.0,y=logicalDpiY()/72.0;
	pageNum=pageCur=-1;
	isWaiting=false;
	auto outerLayout=new QVBoxLayout;

	//Head
	auto keywdLayout=new QHBoxLayout;

	keywE=new QLineEdit(this);
	keywdLayout->addWidget(keywE);

	orderC=new QComboBox(this);
	orderC->setEditable(false);
	sitesC=new QComboBox(this);
	QStringList sites;
	sites<<"Bilibili"<<"AcFun"<<"AcPlay";
	sitesC->addItems(sites);
	sitesC->setEditable(false);
	orderC->setFixedWidth(60*x);
	sitesC->setFixedWidth(60*x);
	keywdLayout->addWidget(orderC);
	keywdLayout->addWidget(sitesC);

	searchB=new QPushButton(this);
	searchB->setText(tr("Search"));
	keywdLayout->addWidget(searchB);

	outerLayout->addLayout(keywdLayout);

	//Body
	resultW=new QTreeWidget(this);
	resultW->setIconSize(QSize(90*x,67.5*y));
	resultW->setIndentation(0);
	outerLayout->addWidget(resultW);
	setSite();

	//Tail
	auto pageLayout=new QHBoxLayout;
	statusL=new QLabel(tr("Ready"),this);
	pageLayout->addWidget(statusL);
	pageLayout->addStretch();

	pageT=new QLabel(tr("Page"),this);
	pageLayout->addWidget(pageT);

	pageE=new QLineEdit(this);
	pageE->setFixedWidth(30*x);
	pageLayout->addWidget(pageE);

	pageL=new QLabel(this);
	pageL->setFixedWidth(30*x);
	pageLayout->addWidget(pageL);

	pageGoB=new QPushButton(tr("Goto"),this);
	pageUpB=new QPushButton(tr("PgUp"),this);
	pageDnB=new QPushButton(tr("PgDn"),this);
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
	setMinimumSize(450*x,300*y);
	resize(675*x,390*y);
	Utils::setCenter(this);

	resultW->setSelectionMode(QAbstractItemView::SingleSelection);
	resultW->setColumnWidth(0,90*x+6);
	resultW->setColumnWidth(1,45*x);
	resultW->setColumnWidth(2,45*x);
	resultW->setColumnWidth(4,75*x);
	resultW->header()->setStretchLastSection(false);
	resultW->header()->setSectionResizeMode(3,QHeaderView::Stretch);

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
			return;
		}
		clearSearch();
		startSearch();
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
			getData(page);
		}
	};

	connect(pageGoB,&QPushButton::clicked,[jump,this](){jump(pageE->text().toInt());});
	connect(pageUpB,&QPushButton::clicked,[jump,this](){jump(pageCur-1);});
	connect(pageDnB,&QPushButton::clicked,[jump,this](){jump(pageCur+1);});

	connect(okB,&QPushButton::clicked,this,&Search::accept);
	connect(ccB,&QPushButton::clicked,this,&Search::reject);
	connect(resultW,&QTreeWidget::itemActivated,this,&Search::accept);

	manager=new QNetworkAccessManager(this);
	Config::setManager(manager);
	connect(manager,&QNetworkAccessManager::finished,[this](QNetworkReply *reply){
		reply->deleteLater();
		remain.remove(reply);
		QNetworkRequest redirect(reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl());
		if (redirect.url().isValid()){
			remain+=reply->manager()->get(redirect);
			return;
		}
		if (reply->error()==QNetworkReply::OperationCanceledError){
			return;
		}
		QVariant image=reply->request().attribute(QNetworkRequest::User);
		if (image.isValid()){
			QTreeWidgetItem *line=resultW->topLevelItem(image.toInt());
			if (line&&line->icon(0).isNull()){
				QPixmap pixmap;
				pixmap.loadFromData(reply->readAll());
				if(!pixmap.isNull()){
					pixmap=pixmap.scaled(resultW->iconSize(),Qt::IgnoreAspectRatio,Qt::SmoothTransformation);
				}
				line->setIcon(0,QIcon(pixmap));
			}
			return;
		}
		if (reply->error()!=QNetworkReply::NoError){
			QString info=tr("Network error occurred, error code: %1").arg(reply->error());
			QString sugg=Local::instance()->suggestion(reply->error());
			QMessageBox::warning(this,tr("Network Error"),sugg.isEmpty()?info:(info+'\n'+sugg));
			clearSearch();
			isWaiting=false;
		}
		switch(Utils::parseSite(reply->url().url())){
		case Utils::Bilibili:
		{
			QString data(reply->readAll());
			QRegularExpression r;
			QRegularExpressionMatch m;
			if (pageNum==-1){
				r.setPattern("(?<=page=)\\d+");
				m=r.match(data,data.indexOf("endPage"));
				pageNum=m.hasMatch()?m.captured().toInt():1;
				pageL->setText(QString("/%1").arg(pageNum));
			}
			QStringList ary=data.split("<li class=\"l ",QString::SkipEmptyParts);
			if(ary.size()>=2){
				QString &last=ary.last();
				last.truncate(last.lastIndexOf("</li>")+5);
				ary.removeFirst();
				for(const QString &item:ary){
					QTreeWidgetItem *row=new QTreeWidgetItem(resultW);
					r.setPattern("av\\d+");
					m=r.match(item);
					row->setData(0,Qt::UserRole,m.captured());
					row->setSizeHint(0,QSize(0,resultW->iconSize().height()+3));
					r.setPattern("(?<=src=\")[^\"']+");
					m=r.match(item,m.capturedEnd());
					QNetworkRequest request(QUrl(m.captured()));
					request.setAttribute(QNetworkRequest::User,resultW->invisibleRootItem()->childCount()-1);
					remain+=manager->get(request);
					r.setPattern("(?<=<span>)[^<]+");
					m=r.match(item,m.capturedEnd());
					row->setText(4,Utils::decodeXml(m.captured()));
					r.setPattern("(?<=\\s).+");
					m=r.match(item,m.capturedEnd());
					row->setText(3,Utils::decodeXml(m.captured()));
					r.setPattern("class=\"upper\"");
					m=r.match(item,m.capturedEnd());
					r.setPattern("(?<=>)[^<]+");
					m=r.match(item,m.capturedEnd());
					row->setText(5,Utils::decodeXml(m.captured()));
					r.setPattern("<i class");
					m=r.match(item,m.capturedEnd());
					r.setPattern("(?<=>)[\\s\\d\\-]+(?=</i>)");
					auto i=r.globalMatch(item,m.capturedEnd());
					row->setText(1,i.next().captured().simplified());
					i.next();
					row->setText(2,i.next().captured().simplified());
					r.setPattern("(?<=class=\"intro\">).*(?=</div>)");
					m=r.match(item,i.next().capturedEnd());
					row->setToolTip(3,Utils::decodeXml(m.captured()));
				}
			}
			statusL->setText(tr("Finished"));
			isWaiting=false;
			break;
		}
		case Utils::AcFun:
		{
			QJsonObject page=QJsonDocument::fromJson(reply->readAll()).object()["data"].toObject()["page"].toObject();
			if (pageNum==-1){
				pageNum=page["totalCount"].toDouble()/page["pageSize"].toDouble()+0.5;
				pageL->setText(QString("/%1").arg(pageNum));
			}
			QJsonArray list=page["list"].toArray();
			for(int i=0;i<list.count();++i){
				QJsonObject item=list[i].toObject();
				int channelId=item["channelId"].toDouble();
				if (channelId == 110 || channelId == 63 || (channelId > 72 && channelId < 77)) {
					continue;
				}
				QStringList content;
				content+="";
				content+=QString::number((int)item["views"].toDouble());
				content+=QString::number((int)item["comments"].toDouble());
				content+=Utils::decodeXml(item["title"].toString());
				content+=getChannel("AcFun")[channelId];
				content+=Utils::decodeXml(item["username"].toString());
				QTreeWidgetItem *row=new QTreeWidgetItem(resultW,content);
				row->setData(0,Qt::UserRole,item["contentId"].toString());
				row->setSizeHint(0,QSize(0,resultW->iconSize().height()+3));
				row->setToolTip(3,item["description"].toString());
				QNetworkRequest request(QUrl(item["titleImg"].toString()));
				request.setAttribute(QNetworkRequest::User,resultW->invisibleRootItem()->childCount()-1);
				remain+=manager->get(request);
			}
			statusL->setText(tr("Finished"));
			isWaiting=false;
			break;
		}
		case Utils::AcPlay:
		{
			QJsonObject json=QJsonDocument::fromJson(reply->readAll()).object();
			for(QJsonValue iter:json["Matches"].toArray()){
				QJsonObject item=iter.toObject();
				QStringList content;
				content+=item["AnimeTitle"].toString();
				content+="";
				content+="";
				content+=item["EpisodeTitle"].toString();
				content+=getChannel("AcPlay")[item["Type"].toInt()];
				content+="";
				QTreeWidgetItem *row=new QTreeWidgetItem(resultW,content);
				row->setData(0,Qt::UserRole,QString("dd%1").arg(item["EpisodeId"].toInt()));
				row->setSizeHint(0,QSize(0,resultW->iconSize().height()+3));
			}
			for(QJsonValue iter:json["Animes"].toArray()){
				QJsonObject item=iter.toObject();
				QString title=item["Title"].toString(),type=getChannel("AcPlay")[item["Type"].toInt()];
				for(QJsonValue epsd:item["Episodes"].toArray()){
					item=epsd.toObject();
					QStringList content;
					content+=title;
					content+="";
					content+="";
					content+=item["Title"].toString();
					content+=type;
					content+="";
					QTreeWidgetItem *row=new QTreeWidgetItem(resultW,content);
					row->setData(0,Qt::UserRole,QString("dd%1").arg(item["Id"].toInt()));
					row->setSizeHint(0,QSize(0,resultW->iconSize().height()+3));
				}
			}
			pageNum=1;
			pageL->setText("/1");
			statusL->setText(tr("Finished"));
			isWaiting=false;
			break;
		}
		default:
			break;
		}
	});
}

void Search::getData(int pageNum)
{
	for(QNetworkReply *r:QSet<QNetworkReply *>(remain)){
		r->abort();
	}
	key=keywE->text();
	QUrl url;
	switch(sitesC->currentIndex()){
	case 0:
	{
		if (key.isEmpty()){
			return;
		}
		url=QUrl("http://www."+
				 Utils::customUrl(Utils::Bilibili)+
				 "/search");
		auto order=getOrder(Utils::Bilibili);
		QUrlQuery query;
		query.addQueryItem("keyword",key);
		query.addQueryItem("page",QString::number(pageNum));
		query.addQueryItem("orderby",order[orderC->currentIndex()]);
		query.addQueryItem("pagesize","20");
		url.setQuery(query);
		break;
	}
	case 1:
	{
		if (key.isEmpty()){
			return;
		}
		url=QUrl("http://search."+
				 Utils::customUrl(Utils::AcFun)+
				 "/search");
		auto order=getOrder(Utils::AcFun);
		QUrlQuery query;
		query.addQueryItem("q",key);
		query.addQueryItem("sortType","-1");
		query.addQueryItem("field","title");
		query.addQueryItem("sortField",order[orderC->currentIndex()]);
		query.addQueryItem("pageNo",QString::number(pageNum));
		query.addQueryItem("pageSize","20");
		url.setQuery(query);
		break;
	}
	case 2:
	{
		QUrlQuery query;
		switch(orderC->currentIndex()){
		case 0:
		{
			if (key.isEmpty()){
				return;
			}
			QStringList args=key.split("#");
			url=QUrl("http://api."+
					 Utils::customUrl(Utils::AcPlay)+
					 "/api/v1/searchall/"+
					 (args.size()==2?args.join("/"):key));
			break;
		}
		case 1:
		{
			if (key.isEmpty()){
				return;
			}
			if (QRegularExpression("^[^#]*#\\d+$").match(key).hasMatch()){
				QMessageBox::warning(this,tr("Match Error"),tr("Format {anime}#{episode} needed."));
				return;
			}
			QStringList args=key.split("#");
			url=QUrl("http://api."+
					 Utils::customUrl(Utils::AcPlay)+
					 "/api/v1/search/TVAnime");
			query.addQueryItem("anime"  ,args[0]);
			query.addQueryItem("episode",args[1]);
			break;
		}
		case 2:
		{
			if (key.isEmpty()){
				return;
			}
			QStringList args=key.split("#");
			url=QUrl("http://api."+
						Utils::customUrl(Utils::AcPlay)+
						"/api/v1/search/Other");
			query.addQueryItem("anime"  ,args.size()==2?args[0]:key);
			query.addQueryItem("episode",args.size()==2?args[1]:QString(""));
			break;
		}
		case 3:
		{
			QFile file(key);
			if(!file.exists()){
				file.setFileName(APlayer::instance()->getMedia());
			}
			if (file.exists()){
				file.open(QIODevice::ReadOnly);
				url=QUrl("http://api."+
						 Utils::customUrl(Utils::AcPlay)+
						 "/api/v1/match");
				query.addQueryItem("fileName",QFileInfo(file).baseName());
				query.addQueryItem("hash",QCryptographicHash::hash(file.read(0x1000000),QCryptographicHash::Md5).toHex());
				query.addQueryItem("length",QString::number(file.size()));
			}
			else{
				QMessageBox::warning(this,tr("Match Error"),tr("Please open a video or type in the file path."));
				return;
			}
		}
		}
		url.setQuery(query);
	}
	}
	resultW->clear();
	isWaiting=true;
	pageCur=pageNum;
	pageE->setText(QString::number(pageCur));
	statusL->setText(tr("Requesting"));
	remain+=manager->get(QNetworkRequest(url));
}

#define tr
QList<const char *> Search::getOrder(int site)
{
	QList<const char *> od;
	switch(site){
	case Utils::AcFun:
		od<<tr("rankLevel")<<
			tr("releaseDate")<<
			tr("views")<<
			tr("comments")<<
			tr("stows");
		break;
	case Utils::Bilibili:
		od<<tr("default")<<
		    tr("pubdate")<<
			tr("senddate")<<
			tr("ranklevel")<<
			tr("click")<<
			tr("scores")<<
			tr("dm")<<
			tr("stow");
		break;
	case Utils::AcPlay:
		od<<tr("default")<<
			tr("TVAnime")<<
			tr("Other")<<
			tr("FileMatch");
		break;
	}
	return od;
}
#undef tr

void Search::setText(QString text)
{
	key=text;
	keywE->setText(key);
	searchB->click();
}

void Search::setSite()
{
	QStringList header,options;
	switch(sitesC->currentIndex()){
	case 0:
		header<<tr("Cover")<<tr("Play")<<tr("Danmaku")<<tr("Title")<<tr("Typename")<<tr("Author");
		for(const char *i:getOrder(Utils::Bilibili)){
			options.append(tr(i));
		}
		break;
	case 1:
		header<<tr("Cover")<<tr("Play")<<tr("Comment")<<tr("Title")<<tr("Typename")<<tr("Author");
		for(const char *i:getOrder(Utils::AcFun)){
			options.append(tr(i));
		}
		break;
	case 2:
		header<<tr("Title")<<""<<""<<tr("Episode")<<tr("Typename")<<"";
		for(const char *i:getOrder(Utils::AcPlay)){
			options.append(tr(i));
		}
		break;
	}
	isWaiting=true;
	orderC->clear();
	orderC->addItems(options);
	orderC->setCurrentIndex(0);
	resultW->setHeaderLabels(header);
	isWaiting=false;
}

void Search::accept()
{
	QTreeWidgetItem *item=resultW->currentItem();
	if (item){
		aid=item->data(0,Qt::UserRole).toString();
		QDialog::accept();
	}
	else{
		QMessageBox::warning(this,tr("Warning"),tr("No video has been chosen."));
	}
}

void Search::startSearch()
{
	getData(1);
}

void Search::clearSearch()
{
	resultW->clear();
	pageNum=-1;
	pageCur=-1;
	pageE->clear();
	pageL->clear();
	statusL->setText(tr("Ready"));
}
