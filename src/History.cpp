/*=======================================================================
*
*   Copyright (C) 2013 Lysine.
*
*   Filename:    History.cpp
*   Time:        2014/10/05
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
#include "APlayer.h"
#include "Danmaku.h"
#include "Config.h"
#include "Load.h"

History *History::ins=nullptr;

History *History::instance()
{
	return ins?ins:new History(qApp);
}

History::History(QObject *parent) :
    QObject(parent)
{
	ins=this;
	setObjectName("History");
	last=nullptr;
	time=0;
	model=new QStandardItemModel(this);
	for(const QJsonValue &value:Config::getValue<QJsonArray>("/Playing/History/List")){
		QStandardItem *head=new QStandardItem;
		QJsonObject record=value.toObject();
		QStringList suffix=Utils::getSuffix(Utils::Video|Utils::Audio);
		for(auto iter=record.begin();iter!=record.end();++iter){
			QString file=QUrl::fromPercentEncoding(iter.key().toUtf8());qint64 time=(*iter).toDouble();
			QFileInfo info(file);
			if(suffix.contains(info.suffix())){
				head->setText(info.completeBaseName());
				head->setData(file,FileRole);
				head->setData(time,TimeRole);
			}
			else{
				QStandardItem *item=new QStandardItem;
				item->setData(file,FileRole);
				item->setData(time,TimeRole);
				head->appendRow(item);
			}
		}
		model->appendRow(head);
	}
	connect(APlayer::instance(),&APlayer::timeChanged, [this](qint64 _time){
		time=_time;
	});
	connect(APlayer::instance(),&APlayer::mediaChanged,[this](QString file){
		QFileInfo info(file);
		updateRecord();
		QString name=info.completeBaseName();
		QString path=info.absoluteFilePath();
		int c=model->rowCount(),i;
		for(i=0;i<c;++i){
			if (model->item(i)->data(FileRole).toString()==path){
				model->insertRow(0,model->takeRow(i));
				last=model->item(0);
				break;
			}
		}
		if(i>=c){
			last=new QStandardItem;
			last->setText(name);
			last->setData(path,FileRole);
			model->insertRow(0,last);
			if (c>=Config::getValue("/Playing/History/Max",10)){
				model->removeRow(c);
			}
		}
	});
}

History::~History()
{
	QJsonArray history;
	updateRecord();
	for(int i=0;i<model->rowCount();++i){
		QJsonObject record;
		QList<QStandardItem *> items;
		QStandardItem *head=model->item(i);
		items<<head;
		for(int i=0;i<head->rowCount();++i){
			items.append(head->child(i));
		}
		for(QStandardItem *iter:items){
			QString key=QUrl::toPercentEncoding(iter->data(FileRole).toString()," !@#$%^&*()+={}[]|\\\"\':;/?<,>.");
			record[key]=iter->data(TimeRole).toDouble();
		}
		history.append(record);
	}
	Config::setValue("/Playing/History/List",history);
}

void History::updateRecord()
{
	int c=model->rowCount();
	if (c==0||last==nullptr){
		return;
	}
	QStandardItem *head=model->item(0);
	head->setRowCount(0);
	head->setData(time,TimeRole);
	for(const Record &r:Danmaku::instance()->getPool()){
		QUrl u(r.source);
		QStandardItem *item=new QStandardItem;
		item->setData(u.isLocalFile()?u.toLocalFile():r.string,FileRole);
		item->setData(r.delay,TimeRole);
		head->appendRow(item);
	}
}

QString History::lastPath()
{
	for(int i=0;i<model->rowCount();++i){
		return model->item(i)->data(FileRole).toString();
	}
	return QString();
}

void History::rollback(const QModelIndex &index)
{
	Load *load=Load::instance();
	QStandardItem *head=model->item(index.row());
	for(int i=0;i<head->rowCount();++i){
		QStandardItem *item=head->child(i);
		Load::Task task=Load::instance()->codeToTask(item->data(FileRole).toString());
		task.delay=item->data(TimeRole).value<qint64>();
		load->enqueue(task);
	}
	bool state=load->autoLoad();
	load->setAutoLoad(false);
	APlayer::instance()->setMedia(index.data(FileRole).toString());
	load->setAutoLoad(state);
	Danmaku::instance()->clearPool();
}
