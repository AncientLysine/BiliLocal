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
#include <algorithm>

List *List::ins=nullptr;

List *List::instance()
{
	return ins?ins:new List(qApp);
}

namespace
{
class IconEngine:public QIconEngine
{
public:
	explicit IconEngine(int state):
		state(state)
	{
	}

	void paint(QPainter *p,const QRect &r,QIcon::Mode,QIcon::State)
	{
		switch(state){
		case 0:
			p->drawPolyline(QPolygon({(r.topRight()+r.bottomRight())/2,r.center(),(r.bottomLeft()+r.bottomRight())/2}));
			break;
		case 1:
			p->drawLine((r.topRight()+r.topLeft())/2,(r.bottomLeft()+r.bottomRight())/2);
			break;
		case 2:
			p->drawPolyline(QPolygon({(r.topRight()+r.bottomRight())/2,r.center(),(r.topLeft()   +r.topRight())   /2}));
			break;
		default:
			break;
		}
	}

	QIconEngine *clone() const
	{
		return new IconEngine(state);
	}

private:
	int state;
};

int diffAtNum(QString f,QString s)
{
	for(int i=0;i<f.size()&&i<s.size();++i){
		int d=s[i].unicode()-f[i].unicode();
		if (d!=0){
			return f[i].isNumber()&&s[i].isNumber()?d:0;
		}
	}
	return 0;
}
}

List::List(QObject *parent):
	QStandardItemModel(parent)
{
	ins=this;
	setObjectName("List");
	cur=nullptr;
	time=0;
	for(int i=0;i<3;++i){
		icons.append(QIcon(new IconEngine(i)));
	}
	QStandardItem *lastE=nullptr,*lastD=nullptr;
	auto conbine=[&](){
		if (lastE&&lastD){
			QModelIndexList indexes;
			for(int i=lastE->row();i<=lastD->row();++i){
				indexes.append(index(i,0));
			}
			setRelated(indexes,lastD->data(CodeRole).toInt());
		}
	};
	for(const QJsonValue &i:Config::getValue<QJsonArray>("/Playing/List")){
		QStandardItem *item=new QStandardItem;
		QJsonObject data=i.toObject();
		QFileInfo info(data["File"].toString());
		item->setData(QColor(info.isFile()?Qt::black:Qt::gray),Qt::ForegroundRole);
		item->setText(info.completeBaseName());
		item->setData(info.absoluteFilePath(),FileRole);
		item->setData(data["Time"].toDouble(),TimeRole);
		item->setData(data["Date"].toString(),DateRole);
		item->setEditable(false);item->setDropEnabled(false);
		QJsonValue danm=data["Danm"];
		if (danm.isArray()){
			for(const QJsonValue &j:danm.toArray()){
				QStandardItem *c=new QStandardItem;
				QJsonObject d=j.toObject();
				c->setData(d["Code"].toString(),CodeRole);
				c->setData(d["Time"].toDouble(),TimeRole);
				item->appendRow(c);
			}
			conbine();
			lastE=item;
		}
		else{
			item->setData(data["Danm"].toString()=="Inherit"?Inherit:Surmise,CodeRole);
			lastD=item;
		}
		appendRow(item);
	}
	conbine();

	connect(APlayer::instance(),&APlayer::timeChanged, [this](qint64 _time){
		time=_time;
	});
	connect(APlayer::instance(),&APlayer::mediaChanged,[this](QString file){
		QStandardItem *old=cur;
		cur=itemFromFile(file,true);
		if (old){
			old->setData(QColor(Qt::black),Qt::ForegroundRole);
		}
		switch(cur->data(CodeRole).toInt()){
		case Inherit:
			Danmaku::instance()->delayAll(-time);
			break;
		case Surmise:
			for(int i=1;;i++){
				QStandardItem *head=item(cur->row()-i);
				if (head->data(CodeRole).toInt()==Records){
					for(int j=0;j<head->rowCount();++j){
						QString code=head->child(j)->data(CodeRole).toString();
						int sharp=code.indexOf("#");
						if (sharp!=-1&&!QFile::exists(code)){
							QString id=code.mid(0,sharp);
							QString pt=code.mid(sharp+1);
							Load::instance()->loadDanmaku((id+"#%1").arg(pt.toInt()+i));
						}
					}
					break;
				}
			}
			Danmaku::instance()->clearPool();
			break;
		case Records:
			for(int i=0;i<cur->rowCount();++i){
				Load *load=Load::instance();
				QStandardItem *d=cur->child(i);
				Load::Task task=load->codeToTask(d->data(CodeRole).toString());
				task.delay=d->data(List::TimeRole).value<qint64>();
				load->enqueue(task);
			}
			if (old){
				Danmaku::instance()->clearPool();
			}
			break;
		}
		cur->setData(QColor(90,115,210),Qt::ForegroundRole);
		cur->setData(QDateTime::currentDateTime(),DateRole);
	});
	connect(APlayer::instance(),&APlayer::reach,this,[this](bool m){
		updateCurrent();
		if(!m){
			QTimer::singleShot(0,[this]{
				QModelIndex i=indexFromItem(cur);
				if (jumpToIndex(index(i.row()+1,0,i.parent()),false)&&
					APlayer::instance()->getState()!=APlayer::Play){
					APlayer::instance()->play();
				}
			});
		}
	});
}

List::~List()
{
	QJsonArray list;
	updateCurrent();
	for(int i=0;i<rowCount();++i){
		QStandardItem *item=this->item(i);
		QJsonObject data;
		data["File"]=item->data(FileRole).toString();
		data["Time"]=item->data(TimeRole).toDouble();
		data["Date"]=item->data(DateRole).toString();
		switch(item->data(CodeRole).toInt()){
		case Records:
		{
			QJsonArray danm;
			for(int i=0;i<item->rowCount();++i){
				QStandardItem *c=item->child(i);
				QJsonObject d;
				d["Code"]=c->data(CodeRole).toString();
				d["Time"]=c->data(TimeRole).toDouble();
				danm.append(d);
			}
			data["Danm"]=danm;
			break;
		}
		case Inherit:
			data["Danm"]="Inherit";
			break;
		case Surmise:
			data["Danm"]="Surmise";
			break;
		}
		list.append(data);
	}
	Config::setValue("/Playing/List",list);
}

bool List::dropMimeData(const QMimeData *data,Qt::DropAction action,int row,int column,const QModelIndex &parent)
{
	if ((!item(row)||item(row)->data(CodeRole).toInt()==Records)&&
		QStandardItemModel::dropMimeData(data,action,row,column,parent)){
		QModelIndexList indexes;
		indexes.append(index(row,0));
		for(int o=row+1;;++o){
			QModelIndex i=index(o,0);
			if (i.data(CodeRole).toInt()==Records){
				setRelated(indexes,indexes.last().data(CodeRole).toInt());
				break;
			}
			else{
				indexes.append(i);
			}
		}
		return 1;
	}
	else{
		return 0;
	}
}

void List::setRelated(const QModelIndexList &indexes,int reason)
{
	QList<int> rows;
	for(const QModelIndex&i:indexes){
		rows.append(i.row());
	}
	int s=rows.size()-1;
	if (s<=0){
		return;
	}
	std::sort(rows.begin(),rows.end());
	int b=rows[0];
	for(int r:rows){
		insertRow(b++,takeRow(r));
	}
	int f=rows.first(),t=b-1;
	item(f)->setIcon(icons[0]);
	for(int i=f+1;i<=t;++i){
		QStandardItem *item=this->item(i);
		item->setIcon(icons[i==t?2:1]);
		item->setData(reason,CodeRole);
		item->setRowCount(0);
	}
}

QString List::defaultPath(int type)
{
	QStandardItem *item=cur;
	if (hasChildren()&&!item){
		item=this->item(0);
		for(int i=1;i<rowCount();++i){
			QStandardItem *iter=this->item(i);
			if (iter->data(DateRole).toDateTime()>item->data(DateRole).toDateTime()){
				item=iter;
			}
		}
	}
	if (item){
		if (type==Utils::Danmaku){
			for(int i=0;i<item->rowCount();++i){
				QString code=item->child(i)->data(CodeRole).toString();
				if (QFile::exists(code)){
					return code;
				}
			}
		}
		return item->data(FileRole).toString();
	}
	QStringList paths=QStandardPaths::standardLocations(QStandardPaths::MoviesLocation);
	paths.append(QDir::homePath());
	return paths.front();
}

QStandardItem *List::itemFromFile(QString file,bool create)
{
	int c=rowCount(),i;
	for(i=0;i<c;++i){
		if (item(i)->data(FileRole).toString()==file){
			return item(i);
		}
	}
	if(!create){
		return nullptr;
	}
	else{
		QFileInfo info(file);
		QString name=info.completeBaseName();
		QString path=info.absoluteFilePath();
		QStandardItem *item=new QStandardItem;
		item->setText(name);
		item->setData(path,FileRole);
		item->setEditable(false);item->setDropEnabled(false);
		appendRow(item);
		QStringList accept=Utils::getSuffix(Utils::Danmaku);
		QModelIndexList indexes;
		indexes.append(item->index());
			for(const QFileInfo &iter:info.dir().entryInfoList(QDir::Files,QDir::Name)){
			QString p=iter.absoluteFilePath();
			if(!item->hasChildren()&&
				accept.contains(iter.suffix().toLower())&&
				info.baseName()==iter.baseName()){
				QStandardItem *d=new QStandardItem;
				d->setData(p,CodeRole);
				item->appendRow(d);
			}
			QString c=info.completeBaseName(),o=iter.completeBaseName();
			if(!itemFromFile(p)&&info.suffix()==iter.suffix()&&diffAtNum(c,o)>0){
				QStandardItem *i=new QStandardItem;
				i->setText(o);
				i->setData(p,FileRole);
				i->setEditable(false);i->setDropEnabled(false);
				appendRow(i);
				indexes.append(i->index());
			}
		}
		group(indexes);
		return item;
	}
}

bool List::finished()
{
	return !hasChildren()||cur==item(rowCount()-1);
}

void List::appendMedia(QString file)
{
	itemFromFile(file,true);
}

void List::updateCurrent()
{
	if(!cur){
		return;
	}
	cur->setRowCount(0);
	cur->setData(time,TimeRole);
	if (cur->data(CodeRole).toInt()!=Records){
		return;
	}
	for(const Record &r:Danmaku::instance()->getPool()){
		QUrl u(r.source);
		QStandardItem *d=new QStandardItem;
		d->setData(u.isLocalFile()?u.toLocalFile():r.string,CodeRole);
		d->setData(r.delay,TimeRole);
		cur->appendRow(d);
	}
}

void List::merge(const QModelIndexList &indexes)
{
	setRelated(indexes,Inherit);
}

void List::group(const QModelIndexList &indexes)
{
	setRelated(indexes,Surmise);
}

void List::split(const QModelIndexList &indexes)
{
	for(const QModelIndex&i:indexes){
		QStandardItem *item=itemFromIndex(i);
		item->setIcon(QIcon());item->setData(0,CodeRole);
	}
}

bool List::jumpToIndex(const QModelIndex &index,bool manually)
{
	QStandardItem *head=itemFromIndex(index);
	if(!head){
		return false;
	}
	APlayer::instance()->setMedia(head->data(FileRole).toString(),manually);
	return true;
}
