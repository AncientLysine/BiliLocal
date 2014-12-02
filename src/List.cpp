/*=======================================================================
*
*   Copyright (C) 2013 Lysine.
*
*   Filename:    List.cpp
*   Time:        2014/11/19
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

#include "List.h"
#include "APlayer.h"
#include "Danmaku.h"
#include "Config.h"
#include "Load.h"
#include "Local.h"
#include <functional>

List *List::ins=nullptr;

List *List::instance()
{
	return ins?ins:new List(qApp);
}

namespace
{
bool diffAtNum(QString f,QString s)
{
	for(int i=0;i<f.size()&&i<s.size();++i){
		if(f[i]!=s[i]){
			return f[i].isNumber()&&s[i].isNumber();
		}
	}
	return false;
}
}

List::List(QObject *parent):
	QObject(parent)
{
	ins=this;
	setObjectName("List");
	cur=nullptr;
	stop=0;
	time=0;
	model=new QStandardItemModel(this);
	for(const QJsonValue &i:Config::getValue<QJsonArray>("/Playing/List")){
		QStandardItem *item=new QStandardItem;
		QJsonObject data=i.toObject();
		QFileInfo info(data["File"].toString());
		item->setData(QColor(info.isFile()?Qt::black:Qt::gray),Qt::ForegroundRole);
		item->setText(info.completeBaseName());
		item->setData(info.absoluteFilePath(),FileRole);
		item->setData(data["Time"].toDouble(),TimeRole);
		item->setData(data["Date"].toDouble(),DateRole);
		for(const QJsonValue &j:data["Danm"].toArray()){
			QStandardItem *c=new QStandardItem;
			QJsonObject d=j.toObject();
			c->setData(d["Code"].toString(),CodeRole);
			c->setData(d["Time"].toDouble(),TimeRole);
			item->appendRow(c);
		}
		model->appendRow(item);
	}
	connect(APlayer::instance(),&APlayer::timeChanged, [this](qint64 _time){
		time=_time;
	});
	connect(APlayer::instance(),&APlayer::mediaChanged,[this](QString file){
		QFileInfo info(file);
		updateCurrent();
		QString name=info.completeBaseName();
		QString path=info.absoluteFilePath();
		int c=model->rowCount(),i;
		for(i=0;i<c;++i){
			if (model->item(i)->data(FileRole).toString()==path){
				Danmaku::instance()->clearPool();
				cur=model->item(i);
				Load *load=Load::instance();
				for(int i=0;i<cur->rowCount();++i){
					QStandardItem *d=cur->child(i);
					Load::Task task=load->codeToTask(d->data(List::CodeRole).toString());
					task.delay=d->data(List::TimeRole).value<qint64>();
					load->enqueue(task);
				}
				break;
			}
		}
		if(i>=c){
			cur=new QStandardItem;
			cur->setText(name);
			cur->setData(path,FileRole);
			model->appendRow(cur);
		}
		cur->setData(QDateTime::currentMSecsSinceEpoch(),DateRole);
	});
	connect(APlayer::instance(),&APlayer::begin,[this](){
		stop=false;
	});
	connect(APlayer::instance(),&APlayer::reach,[this](bool m){
		if(m){
			stop=true;
		}
		else{
			QModelIndex i=model->indexFromItem(cur);
			if(jumpToIndex(model->index(i.row()+1,0,i.parent()),false)){
				APlayer::instance()->play();
			}
		}
	});
}

List::~List()
{
	QJsonArray list;
	updateCurrent();
	for(int i=0;i<model->rowCount();++i){
		QStandardItem *item=model->item(i);
		QJsonObject data;
		data["File"]=item->data(FileRole).toString();
		data["Time"]=item->data(TimeRole).toDouble();
		data["Date"]=item->data(DateRole).toDouble();
		QJsonArray danm;
		for(int i=0;i<item->rowCount();++i){
			QStandardItem *c=item->child(i);
			QJsonObject d;
			d["Code"]=c->data(CodeRole).toString();
			d["Time"]=c->data(TimeRole).toDouble();
			danm.append(d);
		}
		data["Danm"]=danm;
		list.append(data);
	}
	Config::setValue("/Playing/List",list);
}

QString List::defaultPath(int type)
{
	QStandardItem *item=cur;
	if (model->hasChildren()&&!item){
		item=model->item(0);
		for(int i=1;i<model->rowCount();++i){
			QStandardItem *iter=model->item(i);
			if (iter->data(DateRole).toDouble()>item->data(DateRole).toDouble()){
				item=iter;
			}
		}
	}
	if (item){
		if (type!=Utils::Danmaku){
			return item->data(FileRole).toString();
		}
		for(int i=0;i<item->rowCount();++i){
			QString code=item->child(i)->data(CodeRole).toString();
			if (QFile::exists(code)){
				return code;
			}
		}
	}
	QStringList paths=QStandardPaths::standardLocations(QStandardPaths::MoviesLocation);
	paths.append(QDir::homePath());
	return paths.front();
}

bool List::finished()
{
	return stop||!model->hasChildren()||cur==model->item(model->rowCount()-1);
}

void List::updateCurrent()
{
	if(!cur){
		return;
	}
	cur->setRowCount(0);
	cur->setData(time,TimeRole);
	for(const Record &r:Danmaku::instance()->getPool()){
		QUrl u(r.source);
		QStandardItem *d=new QStandardItem;
		d->setData(u.isLocalFile()?u.toLocalFile():r.string,CodeRole);
		d->setData(r.delay,TimeRole);
		cur->appendRow(d);
	}
}

bool List::jumpToIndex(const QModelIndex &index,bool manually)
{
	QStandardItem *head=model->itemFromIndex(index);
	if(!head){
		return false;
	}
	Load *load=Load::instance();
	bool state=load->autoLoad();
	load->setAutoLoad(false);
	APlayer::instance()->setMedia(head->data(FileRole).toString(),manually);
	load->setAutoLoad(state);
	return true;
}
