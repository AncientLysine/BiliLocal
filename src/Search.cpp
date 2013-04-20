/*=======================================================================
*
*   Copyright (C) 2013 Lysine.
*
*   Filename:    Search.cpp
*   Time:        2013/04/18
*   Author:      Chaserhkj
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

namespace {
	const QString apiUrl("http://api.bilibili.tv/search?type=json&keyword=%1&page=%2&order=default");
}

Search::Search(QWidget *parent) : QDialog(parent)
{
	auto outerLayout=new QVBoxLayout;
	auto keywordLayout=new QHBoxLayout;
	keywordE=new QLineEdit;
	searchB=new QPushButton;
	searchB->setText(tr("Search"));
	keywordLayout->addWidget(keywordE);
	keywordLayout->addWidget(searchB);
	outerLayout->addLayout(keywordLayout);
	resultW=new QTreeWidget;
	outerLayout->addWidget(resultW);
	auto pageLayout=new QHBoxLayout;
	pageL=new QLabel;
	pageL->setText(tr("Page"));
	pageE=new QLineEdit;
	pagegotoB=new QPushButton;
	pagegotoB->setText(tr("Goto"));
	pageupB=new QPushButton;
	pageupB->setText(tr("Page up"));
	pagedownB=new QPushButton;
	pagedownB->setText(tr("Page down"));
	pageLayout->addWidget(pageL);
	pageLayout->addWidget(pageE);
	pageLayout->addWidget(pagegotoB);
	pageLayout->addWidget(pageupB);
	pageLayout->addWidget(pagedownB);
	outerLayout->addLayout(pageLayout);
	auto responseLayout=new QHBoxLayout;
	okB=new QPushButton;
	okB->setText(tr("Confirm and load"));
	cancelB=new QPushButton;
	cancelB->setText(tr("Cancel"));
	responseLayout->addWidget(okB);
	responseLayout->addWidget(cancelB);
	outerLayout->addLayout(responseLayout);

	this->setLayout(outerLayout);
	this->setWindowTitle(tr("Search"));
	
	QStringList labels = {tr("AID"),tr("Title"),tr("Typename"),tr("Author")};
	resultW->setHeaderLabels(labels);
	
	connect(keywordE,&QLineEdit::textChanged,this,&Search::clearSearch);
	
	connect(searchB,&QPushButton::clicked,this,&Search::startSearch);

	connect(pagegotoB,&QPushButton::clicked,this,[this](){
			if(this->pageCount==-1) {
				QMessageBox::warning(this,tr("Warning"),tr("No search in progress."));
			}
	});

	connect(pageupB,&QPushButton::clicked,this,[this](){
			if(this->pageCount==-1) {
				QMessageBox::warning(this,tr("Warning"),tr("No search in progress."));
			}
	});

	connect(pagedownB,&QPushButton::clicked,this,[this](){
			if(this->pageCount==-1) {
				QMessageBox::warning(this,tr("Warning"),tr("No search in progress."));
			}
	});

	connect(okB,&QPushButton::clicked,this,&QDialog::accept);
	connect(cancelB,&QPushButton::clicked,this,&QDialog::reject);

	manager=new QNetworkAccessManager(this);
	connect(manager,&QNetWorkAccessManager::finished,this,&Search::dataProcessor);
}

Search::~Search()
{
	
}

QString Search::keyword()
{
	return key;
}


void Search::setKeyword(const QString & key)
{
	this->key=key;
	keywordE->setText(key);
}

void Search::startSearch()
{
	key=keywordE->text();
	pageNum=1;
	pageE->setText("1");
	this->getData(1);
}

void Search::getData(int pagenum)
{
	QNetworkRequest request;
	request.setUrl(QUrl(apiUrl
						.arg(key)
						.arg(QString::number(pagenum))));
	manager->get(request);
}

void Search::clearSearch()
{
	resultW->clear();
	pageCount=-1;
	pageNum=-1;
	pageE->setText("");
}

void Search::dataProcessor(QNetworkReply *reply)
{
	QJsonObject json=QJsonDocument::fromJson(reply->readAll()).object();
	if (pageCount==-1)
		pageCount=json["page"].toInt();

	QJsonObject results=json["result"].toObject();
	for(auto &i : results) {
		QJsonObject item=i.toObject();
		if (item["type"].toString()==QString("video")) {
			QStringList content={
				item["aid"].toString(),
				item["title"].toString(),
				item["typename"].toString(),
				item["author"].toString()
			};
			auto widgetItem=new QTreeWidgetItem(resultW,content);
		}
	}
}
