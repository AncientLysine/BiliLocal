/*=======================================================================
*
*   Copyright (C) 2013 Lysine.
*
*   Filename:    History.cpp
*   Time:        2014/02/03
*   Author:      Lysine
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

#include "History.h"
#include "Utils.h"
#include "Cookie.h"
#include "Danmaku.h"

History::History(Record &record,QWidget *parent):
	QDialog(parent,Qt::Popup)
{
	auto layout=new QGridLayout(this);
	date=new QLabel(this);
	date->setAlignment(Qt::AlignCenter);
	layout->addWidget(date,0,1);
	prev=new QToolButton(this);
	next=new QToolButton(this);
	prev->setIcon(QIcon(":/Picture/previous.png"));
	next->setIcon(QIcon(":/Picture/next.png"));
	connect(prev,&QToolButton::clicked,[this](){
		setCurrentPage(page.addMonths(-1));
	});
	connect(next,&QToolButton::clicked,[this](){
		setCurrentPage(page.addMonths(+1));
	});
	layout->addWidget(prev,0,0);
	layout->addWidget(next,0,2);
	table=new QTableWidget(7,7,this);
	table->setShowGrid(false);
	table->setSelectionMode(QAbstractItemView::SingleSelection);
	table->verticalHeader()->hide();
	table->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);
	table->horizontalHeader()->hide();
	table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
	table->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	table->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	curr=record.limit==0?QDate::currentDate().addDays(1):QDateTime::fromTime_t(record.limit).date();
	if(record.source.indexOf("http://comment.bilibili.tv/")!=-1){
		manager=new QNetworkAccessManager(this);
		manager->setCookieJar(Cookie::instance());
		Cookie::instance()->setParent(NULL);
		QString cid=QFileInfo(record.source).baseName();
		QString api("http://comment.bilibili.tv/rolldate,%1");
		Utils::getReply(manager,QNetworkRequest(api.arg(cid)),[this](QNetworkReply *reply){
			for(QJsonValue iter:QJsonDocument::fromJson(reply->readAll()).array()){
				QJsonObject obj=iter.toObject();
				QJsonValue time=obj["timestamp"],size=obj["new"];
				count[QDateTime::fromTime_t(time.toVariant().toInt()).date()]+=size.toVariant().toInt();
			}
			count[QDate::currentDate().addDays(1)]=0;
			setCurrentPage(curr);
		});
		connect(table,&QTableWidget::itemDoubleClicked,[=,&record](){
			QString url;
			QDate selected=selectedDate();
			if(selected.isValid()){
				url=QString("http://comment.bilibili.tv/dmroll,%1,%2").arg(QDateTime(selected).toTime_t()).arg(cid);
			}
			else{
				url=QString("http://comment.bilibili.tv/%1.xml").arg(cid);
			}
			table->setEnabled(false);
			Utils::getReply(manager,QNetworkRequest(url),[this,&record](QNetworkReply *reply){
				record.danmaku=Utils::parseComment(reply->readAll(),Utils::Bilibili);
				accept();
			});
		});
	}
	else{
		for(const Comment &c:record.danmaku){
			++count[QDateTime::fromTime_t(c.date).date()];
		}
		count[QDate::currentDate().addDays(1)]=0;
		count.remove(count.firstKey());
		setCurrentPage(curr);
		connect(table,&QTableWidget::itemDoubleClicked,this,&QDialog::accept);
	}
	layout->addWidget(table,1,0,1,3);
}

QDate History::selectedDate()
{
	QTableWidgetItem *item=table->currentItem();
	if(item!=NULL){
		QDate selected=item->data(Qt::UserRole).toDate();
		if(selected<=QDate::currentDate()){
			return selected;
		}
	}
	return QDate();
}

void History::setCurrentPage(QDate _d)
{
	page.setDate(_d.year(),_d.month(),1);
	table->clear();
	for(int day=1;day<=7;++day){
		QTableWidgetItem *item=new QTableWidgetItem(QDate::shortDayName(day));
		item->setFlags(0);
		QFont f=item->font();
		f.setBold(true);
		item->setFont(f);
		item->setData(Qt::TextColorRole,QColor(Qt::black));
		table->setItem(0,day-1,item);
	}
	int row=1;
	for(QDate iter=page;iter.month()==page.month();iter=iter.addDays(1)){
		QTableWidgetItem *item=new QTableWidgetItem(QString::number(iter.day()));
		item->setData(Qt::UserRole,iter);
		item->setData(Qt::TextAlignmentRole,Qt::AlignCenter);
		if(!count.contains(iter)){
			item->setFlags(0);
		}
		else{
			item->setFlags(Qt::ItemIsEnabled|Qt::ItemIsSelectable);
		}
		if(iter==curr){
			item->setData(Qt::BackgroundRole,QColor(Qt::gray));
		}
		table->setItem(row,iter.dayOfWeek()-1,item);
		if(item->column()==6){
			++row;
		}
	}
	for(int r=0;r<7;++r){
		for(int c=0;c<7;++c){
			if(table->item(r,c)==0){
				QTableWidgetItem *item=new QTableWidgetItem;
				item->setFlags(0);
				table->setItem(r,c,item);
			}
		}
	}
	QDate last(page.year(),page.month(),page.daysInMonth());
	prev->setEnabled(page>count.firstKey());
	next->setEnabled(last<count.lastKey());
	date->setText(page.toString("MMM yyyy"));
}
