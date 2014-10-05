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
#include "Config.h"
#include "APlayer.h"
#include "Danmaku.h"

History *History::ins=NULL;

History *History::instance()
{
	return ins?ins:new History(qApp);
}

History::History(QObject *parent) :
    QObject(parent)
{
	ins=this;
	setObjectName("History");
	model=new QStandardItemModel(this);
	for(const QJsonValue &record:Config::getValue<QJsonArray>("/Playing/History/List")){
		QString file=record.toString();
		QStandardItem *item=new QStandardItem;
		item->setText(QFileInfo(file).baseName());
		item->setData(file,Qt::UserRole);
		model->appendRow(item);
	}
	connect(APlayer::instance(),&APlayer::mediaChanged,[this](QString file){
		QFileInfo info(file);
		QString name=info.baseName(),path=info.absoluteFilePath();
		QStandardItem *item=new QStandardItem;
		item->setText(name);
		item->setData(path,Qt::UserRole);
		model->insertRow(0,item);
		int c=model->rowCount(),i;
		for(i=1;i<c;++i){
			if(model->item(i)->data(Qt::UserRole).toString()==path){
				model->removeRow(i);
				break;
			}
		}
		if(c>Config::getValue("/Playing/History/Max",10)&&i>=c){
			model->removeRow(c-1);
		}
	});
}

History::~History()
{
	QJsonArray history;
	for(int i=0;i<model->rowCount();++i){
		QStandardItem *item=model->item(i);
		history.append(item->data(Qt::UserRole).toString());
	}
	Config::setValue("/Playing/History/List",history);
}

QString History::lastPath()
{
	for(int i=0;i<model->rowCount();++i){
		return model->item(i)->data(Qt::UserRole).toString();
	}
	return QString();
}
