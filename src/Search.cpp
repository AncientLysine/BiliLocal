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
	statusL=new QLabel(tr("Ready"));
	pageL=new QLabel;
	pageL->setText(tr("Page"));
	pageE=new QLineEdit;
	pageE->setFixedWidth(40);
	pageNumL=new QLabel;
	pageNumL->setFixedWidth(40);
	pagegotoB=new QPushButton;
	pagegotoB->setText(tr("Goto"));
	pageupB=new QPushButton;
	pageupB->setText(tr("Page up"));
	pagedownB=new QPushButton;
	pagedownB->setText(tr("Page down"));
	pageLayout->addWidget(statusL);
	pageLayout->addStretch();
	pageLayout->addWidget(pageL);
	pageLayout->addWidget(pageE);
	pageLayout->addWidget(pageNumL);
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
	responseLayout->addStretch();
	responseLayout->addWidget(cancelB);
	outerLayout->addLayout(responseLayout);

	this->setLayout(outerLayout);
	this->setWindowTitle(tr("Search"));
	this->resize(750,450);
	
	QStringList labels = {tr("AID"),tr("Title"),tr("Typename"),tr("Author")};
	resultW->setHeaderLabels(labels);
	resultW->setSelectionMode(QAbstractItemView::SingleSelection);
	resultW->setColumnWidth(0,100);
	resultW->setColumnWidth(1,400);
	resultW->setColumnWidth(2,100);

	connect(keywordE,&QLineEdit::textChanged,this,&Search::clearSearch);

	connect(searchB,&QPushButton::clicked,[this](){
			if(this->isRequesting) {
				QMessageBox::warning(this,tr("Warning"),tr("A request is pending."));
				return;
			}
			this->startSearch();
	});

	connect(pagegotoB,&QPushButton::clicked,[this](){
			if(this->pageCount==-1) {
				QMessageBox::warning(this,tr("Warning"),tr("No search in progress."));
				return;
			}
			if(this->isRequesting) {
				QMessageBox::warning(this,tr("Warning"),tr("A request is pending."));
				return;
			}
			int page=this->pageE->text().toInt();
			if(page < 1 || page > this->pageCount) {
				QMessageBox::warning(this,tr("Warning"),tr("Page num out of range."));
				return;
			}
			pageNum=page;
			this->getData(page);
	});

	connect(pageupB,&QPushButton::clicked,[this](){
			if(this->pageCount==-1) {
				QMessageBox::warning(this,tr("Warning"),tr("No search in progress."));
				return;
			}
			if(this->isRequesting) {
				QMessageBox::warning(this,tr("Warning"),tr("A request is pending."));
				return;
			}
			if(pageNum == 1) {
				QMessageBox::warning(this,tr("Warning"),tr("Page num out of range."));
				return;
			}
			this->pageE->setText(QString::number(--pageNum));
			this->getData(pageNum);
	});

	connect(pagedownB,&QPushButton::clicked,[this](){
			if(this->pageCount==-1) {
				QMessageBox::warning(this,tr("Warning"),tr("No search in progress."));
				return;
			}
			if(this->isRequesting) {
				QMessageBox::warning(this,tr("Warning"),tr("A request is pending."));
				return;
			}
			if(pageNum == pageCount) {
				QMessageBox::warning(this,tr("Warning"),tr("Page num out of range."));
				return;
			}
			this->pageE->setText(QString::number(++pageNum));
			this->getData(pageNum);
	});

	connect(okB,&QPushButton::clicked,[this](){
			if(this->pageCount==-1) {
				QMessageBox::warning(this,tr("Warning"),tr("No search in progress."));
				return;
			}
			if(this->isRequesting) {
				QMessageBox::warning(this,tr("Warning"),tr("A request is pending."));
				return;
			}
			this->id=this->resultW->currentItem()->text(0);
			this->accept();
	});
	connect(cancelB,&QPushButton::clicked,this,&QDialog::reject);

	connect(resultW,&QTreeWidget::itemActivated,okB,&QPushButton::clicked);
	
	manager=new QNetworkAccessManager(this);
	connect(manager,&QNetworkAccessManager::finished,this,&Search::dataProcessor);
	
}

QString Search::keyword()
{
	return key;
}

QString Search::selectedId()
{
	return id;
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
	resultW->clear();
	isRequesting = true;
	statusL->setText(tr("Requesting"));
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
	pageNumL->setText("");
	statusL->setText(tr("Ready"));
}

void Search::dataProcessor(QNetworkReply *reply)
{
	if (reply->error()==QNetworkReply::NoError) {
		QJsonObject json=QJsonDocument::fromJson(reply->readAll()).object();
		if (pageCount==-1) {
			pageCount=static_cast<int>(json["page"].toDouble());
			pageNumL->setText(QString("/%1").arg(pageCount));
		}
		
		QJsonObject results=json["result"].toObject();
		for(auto i : results) {
			QJsonObject item=i.toObject();
			if (item["type"].toString()==QString("video")) {
				QStringList content={
					item["aid"].toString(),
					item["title"].toString(),
					item["typename"].toString(),
					item["author"].toString()
				};
				new QTreeWidgetItem(resultW,content);
			}
		}
		statusL->setText(tr("Finished"));
	}
	else {
		QMessageBox::warning(this, tr("Network Error"),
							 tr("Network error occurred, error code: %1")
							 .arg(static_cast<int>(reply->error())));
		this->clearSearch();
	}
	isRequesting = false;
	reply->deleteLater();
}
