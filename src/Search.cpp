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
#include "Utils.h"
#include "Config.h"
#include "VPlayer.h"

static QHash<int,QString> getChannel(QString name)
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

#define tr
QList<const char *> Search::AcOrder()
{
	static QList<const char *> l;
	if(l.isEmpty()){
		l<<tr("ranklevel")
		<<tr("click")
		<<tr("pubdate")
		<<tr("scores")
		<<tr("stow");
	}
	return l;
}

QList<const char *> Search::BiOrder()
{
	static QList<const char *> l;
	if(l.isEmpty()){
		l<<tr("default")
		<<tr("pubdate")
		<<tr("senddate")
		<<tr("ranklevel")
		<<tr("click")
		<<tr("scores")
		<<tr("dm")
		<<tr("stow");
	}
	return l;
}
#undef tr

Search::Search(QWidget *parent):QDialog(parent)
{
	pageNum=pageCur=-1;
	isWaiting=false;
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
	QStringList sites;
	sites<<"Bilibili"<<"AcFun"<<"AcPlay";
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
		else if(!keywE->text().isEmpty()||(sitesC->currentIndex()==2&&orderC->currentIndex()==2)){
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

	manager=new QNetworkAccessManager(this);
	Config::setManager(manager);
	connect(manager,&QNetworkAccessManager::finished,[this](QNetworkReply *reply){
		QUrl redirect=reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
		if(redirect.isValid()){
			reply->manager()->get(QNetworkRequest(redirect));
			reply->deleteLater();
			return;
		}
		auto error=[this](int code){
			QString info=tr("Network error occurred, error code: %1");
			QMessageBox::warning(this,tr("Network Error"),info.arg(code));
			clearSearch();
			isWaiting=false;
		};
		QString url=reply->url().url();
		if(reply->error()==QNetworkReply::NoError){
			switch(Utils::getSite(url)){
			case Utils::Bilibili:
			{
				QString data(reply->readAll());
				int sta,end;
				if(pageNum==-1){
					sta=data.indexOf("<div class=\"pagelistbox\"><span>");
					if(sta!=-1){
						end=data.indexOf("</div>",sta);
						QString page=data.mid(sta,end-sta+6);
						sta=page.indexOf("<a class=\"endPage\"");
						sta=page.indexOf("page=",sta)+5;
						end=page.indexOf("\">",sta);
						pageNum=page.mid(sta,end-sta).toInt();
					}
					else{
						pageNum=1;
					}
					pageNuL->setText(QString("/%1").arg(pageNum));
				}
				QStringList ary=data.split("<li class=\"l\">",QString::SkipEmptyParts);
				if(ary.size()>=2){
					QString &item=ary.last();
					item.truncate(item.lastIndexOf("</li>")+5);
					ary.removeFirst();
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
							sta=item.indexOf("<span>",end)+6;
							end=item.indexOf("</span>",sta);
							row->setText(4,Utils::decodeXml(item.mid(sta,end-sta)));
							sta=end+7;
							end=item.indexOf("</div>",sta);
							row->setText(3,Utils::decodeXml(item.mid(sta,end-sta)));
							sta=item.indexOf("class=\"upper\"",end);
							sta=item.indexOf("\">",sta)+2;
							end=item.indexOf("</a>",sta);
							row->setText(5,Utils::decodeXml(item.mid(sta,end-sta)));
							sta=item.indexOf("class=\"gk\"",end);
							auto iter=QRegularExpression("[\\d-]+").globalMatch(item.mid(sta));
							row->setText(1,iter.next().captured());
							iter.next();
							row->setText(2,iter.next().captured());
							sta=item.indexOf("class=\"intro\">",sta)+14;
							end=item.indexOf("</div>",sta);
							row->setToolTip(3,item.mid(sta,end-sta));
						}
					}
				}
				statusL->setText(tr("Finished"));
				isWaiting=false;
				break;
			}
			case Utils::AcFun:
			{
				QJsonObject json=QJsonDocument::fromJson(reply->readAll()).object();
				QJsonObject page=json["page"].toObject();
				if(pageNum==-1){
					pageNum=page["totalPage"].toDouble();
					pageNuL->setText(QString("/%1").arg(pageNum));
				}
				QJsonArray list=json["contents"].toArray();
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
					row->setData(0,Qt::UserRole,QString("ac%1").arg(item["aid"].toInt()));
					row->setSizeHint(0,QSize(120,92));
					row->setToolTip(3,item["description"].toString());
					QNetworkRequest request(QUrl(item["titleImg"].toString()));
					request.setAttribute(QNetworkRequest::User,resultW->invisibleRootItem()->childCount()-1);
					reply->manager()->get(request);
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
					row->setSizeHint(0,QSize(120,92));
				}
				pageNum=1;
				pageNuL->setText("/1");
				statusL->setText(tr("Finished"));
				isWaiting=false;
				break;
			}
			default:
			{
				QTreeWidgetItem *line=resultW->topLevelItem(reply->request().attribute(QNetworkRequest::User).toInt());
				if(line!=NULL&&line->icon(0).isNull()){
					QPixmap pixmap;
					pixmap.loadFromData(reply->readAll());
					if(!pixmap.isNull()){
						pixmap=pixmap.scaled(120,90,Qt::IgnoreAspectRatio,Qt::SmoothTransformation);
					}
					line->setIcon(0,QIcon(pixmap));
				}
				break;
			}
			}
		}
		else if(Utils::getSite(url)!=Utils::Unknown){
			error(reply->error());
		}
		reply->deleteLater();
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
	QStringList header,options;
	switch(sitesC->currentIndex()){
	case 0:
		header<<tr("Cover")<<tr("Play")<<tr("Danmaku")<<tr("Title")<<tr("Typename")<<tr("Author");
		for(const char *i:BiOrder()){
			options.append(tr(i));
		}
		break;
	case 1:
		header<<tr("Cover")<<tr("Play")<<tr("Comment")<<tr("Title")<<tr("Typename")<<tr("Author");
		for(const char *i:AcOrder()){
			options.append(tr(i));
		}
		break;
	case 2:
		header<<tr("Title")<<""<<""<<tr("Episode")<<tr("Typename")<<"";
		options<<tr("TVAnime")<<tr("Other")<<tr("FileMatch");
		break;
	}
	isWaiting=true;
	orderC->clear();
	orderC->addItems(options);
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
	QUrl url;
	switch(sitesC->currentIndex()){
	case 0:
	{
		url=QUrl("http://www.bilibili.tv/search");
		QUrlQuery query;
		query.addQueryItem("keyword",key);
		query.addQueryItem("page",QString::number(pageNum));
		query.addQueryItem("orderby",BiOrder()[orderC->currentIndex()]);
		query.addQueryItem("pagesize","20");
		url.setQuery(query);
		break;
	}
	case 1:
	{
		url=QUrl("http://api.acfun.tv/search");
		QUrlQuery query;
		query.addQueryItem("query",key);
		query.addQueryItem("exact","1");
		query.addQueryItem("orderId",QString::number(orderC->currentIndex()));
		query.addQueryItem("orderBy","1");
		query.addQueryItem("pageNo",QString::number(pageNum));
		query.addQueryItem("pageSize","20");
		url.setQuery(query);
		break;
	}
	case 2:
	{
		if(orderC->currentIndex()==2){
			QFile file(key);
			if(!file.exists()){
				file.setFileName(VPlayer::instance()->getMedia());
			}
			if(file.exists()){
				file.open(QIODevice::ReadOnly);
				url=QUrl("http://api.acplay.net/api/v1/match");
				QUrlQuery query;
				query.addQueryItem("fileName",QFileInfo(file).baseName());
				query.addQueryItem("hash",QCryptographicHash::hash(file.read(0x1000000),QCryptographicHash::Md5).toHex());
				query.addQueryItem("length",QString::number(file.size()));
				url.setQuery(query);
			}
			else{
				QMessageBox::warning(this,tr("Match Error"),tr("Please open a video or type in the file path."));
				return;
			}
		}
		else{
			QUrlQuery query;
			QStringList args=key.split("#");
			switch(orderC->currentIndex()){
			case 0:
				url=QUrl("http://api.acplay.net/api/v1/search/TVAnime");
				if(args.size()!=2){
					QMessageBox::warning(this,tr("Match Error"),tr("Format {anime}#{episode} needed."));
					return;
				}
				query.addQueryItem("anime",args[0]);
				query.addQueryItem("episode",args[1]);
				break;
			case 1:
				url=QUrl("http://api.acplay.net/api/v1/search/Other");
				if(args.size()==0||args.size()>2){
					QMessageBox::warning(this,tr("Match Error"),tr("Format {anime}#{episode} needed."));
					return;
				}
				query.addQueryItem("anime",args[0]);
				query.addQueryItem("episode",args.size()==2?args[1]:QString(""));
				break;
			}
			url.setQuery(query);
		}
	}
	}
	resultW->clear();
	isWaiting=true;
	statusL->setText(tr("Requesting"));
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
