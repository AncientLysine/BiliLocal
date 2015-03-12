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

QModelIndex getGroupHead(QModelIndex i)
{
	QModelIndex f;
	for(int o=0;;++o){
		f=i.sibling(i.row()-o,0);
		if (f.data(List::CodeRole).toInt()==List::Records||f.row()==0){
			return f;
		}
	}
}

QModelIndex getGroupTail(QModelIndex i)
{
	for(;;){
		QModelIndex j=i.sibling(i.row()+1,0);
		if (j.isValid()&&j.data(List::CodeRole).toInt()!=List::Records){
			i=j;
		}
		else{
			return i;
		}
	}
}

void clearItemGroup(int row)
{
	QStandardItem *i=List::instance()->item(row);
	i->setIcon(QIcon());
	i->setData(List::Records,List::CodeRole);
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
	QStandardItem *lastE=nullptr;
	QStandardItem *lastD=nullptr;
	int lastC=0;
	auto conbine=[&](){
		if (lastE&&lastD){
			QModelIndexList indexes;
			for(int i=lastE->row();i<=lastD->row();++i){
				indexes.append(index(i,0));
			}
			setRelated(indexes,lastC);
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
			lastC=data["Danm"].toString()=="Inherit"?Inherit:Surmise;
			lastD=item;
		}
		appendRow(item);
	}
	conbine();

	connect(APlayer::instance(),&APlayer::timeChanged, [this](qint64 _time){
		time=_time;
	});
	connect(APlayer::instance(),&APlayer::mediaChanged,[this](QString file){
		updateCurrent();
		QStandardItem *old=cur;
		cur=itemFromFile(file,true);
		if (old){
			old->setData(QColor(Qt::black),Qt::ForegroundRole);
		}
		cur->setData(QColor(90,115,210),Qt::ForegroundRole);
		cur->setData(QDateTime::currentDateTime(),DateRole);
		bool success=false;
		switch(cur->data(CodeRole).toInt()){
		case Inherit:
			Danmaku::instance()->delayAll(-time);
			success=true;
			break;;
		case Surmise:
			for(int i=1;;i++){
				QStandardItem *head=item(cur->row()-i);
				if (head->data(CodeRole).toInt()!=Records){
					continue;
				}
				for(int j=0;j<head->rowCount();++j){
					QString code=head->child(j)->data(CodeRole).toString();
					int sharp=code.indexOf(QRegularExpression("[#_]"));
					if (sharp!=-1&&!QFile::exists(code)){
						QString id=code.mid(0,sharp);
						QString pt=code.mid(sharp+1);
						Load::instance()->loadDanmaku((id+"#%1").arg(pt.toInt()+i));
						success=true;
					}
				}
				break;
			}
			Danmaku::instance()->clearPool();
			break;
		case Records:
		{
			QFileInfo info(cur->data(FileRole).toString());
			for(int i=0;i<cur->rowCount();++i){
				Load *load=Load::instance();
				QStandardItem *d=cur->child(i);
				QString danmaku=d->data(CodeRole).toString();
				danmaku.replace("%{File}",info.completeBaseName()).replace("%{Path}",info.absolutePath());
				Load::Task task=load->codeToTask(danmaku);
				task.delay=d->data(List::TimeRole).value<qint64>();
				load->enqueue(task);
				success=true;
			}
			if (old||success){
				Danmaku::instance()->clearPool();
			}
			break;
		}
		}
		if(!success&&Danmaku::instance()->getPool().isEmpty()){
			QFileInfo info(cur->data(FileRole).toString());
			QStringList accept=Utils::getSuffix(Utils::Danmaku);
			for(const QFileInfo &iter:info.dir().entryInfoList(QDir::Files,QDir::Name)){
				QString file=iter.absoluteFilePath();
				if(!accept.contains(iter.suffix().toLower())||
					info.completeBaseName()!=iter.completeBaseName()){
					continue;
				}
				Load::instance()->loadDanmaku(file);
				break;
			}
		}
	});
	connect(APlayer::instance(),&APlayer::reach,this,[this](bool m){
		if(!m&&!finished()){
			QModelIndex i=indexFromItem(cur);
			if (jumpToIndex(index(i.row()+1,0,i.parent()),false)&&
				APlayer::instance()->getState()!=APlayer::Play){
				APlayer::instance()->play();
				return;
			}
		}
		updateCurrent();
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

QStringList List::mimeTypes() const
{
	return {"application/x-bililocallistdata"};
}

namespace
{
QList<QStandardItem *> getItems(const QModelIndexList &indexes)
{
	QList<QStandardItem *> items;
	for(const QModelIndex&i:indexes){
		items.append(List::instance()->itemFromIndex(i));
	}
	std::sort(items.begin(),items.end(),[](QStandardItem *f,QStandardItem *s){return f->row()<s->row();});
	return items;
}
}

QMimeData *List::mimeData(const QModelIndexList &indexes) const
{
	QByteArray byte;
	QDataStream s(&byte,QIODevice::WriteOnly);
	for(QStandardItem *item:getItems(indexes)){
		s<<item->row();
	}
    QMimeData *data=new QMimeData;
	data->setData("application/x-bililocallistdata",byte);
	return data;
}

bool List::dropMimeData(const QMimeData *data,Qt::DropAction action,int row,int column,const QModelIndex &parent)
{
	if (action!=Qt::MoveAction||column!=0||parent.isValid()){
		return false;
	}
	if (item(row)&&item(row)->data(CodeRole).toInt()!=Records){
		return false;
	}
	QDataStream s(data->data("application/x-bililocallistdata"));
	QList<QStandardItem *> items;
	while(!s.atEnd()){
		int source;
		s>> source;
		items.append(item(source));
	}
	if (items.isEmpty()){
		return false;
	}
	std::sort(items.begin(),items.end(),[](QStandardItem *f,QStandardItem *s){return f->row()<s->row();});
	for(QStandardItem *item:items){
		split(item->index());
		if (item->row()<row){
			--row;
		}
		insertRow(row,takeRow(item->row()));
		++row;
	}
	return true;
}

void List::setRelated(const QModelIndexList &indexes,int reason)
{
	if (indexes.size()<2){
		return;
	}
	int i=-1;
	auto items=getItems(indexes);
	split(indexes);
	for(QStandardItem *item:items){
		if (i<0){
			i=item->row();
			insertRow(i,takeRow(item->row()));
			item->setIcon(icons[0]);
		}
		else{
			++i;
			insertRow(i,takeRow(item->row()));
			item->setIcon(icons[1]);
			item->setData(reason,CodeRole);
			item->setRowCount(0);
		}
	}
	item(i)->setIcon(icons[2]);
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

namespace
{
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
		QModelIndexList indexes;
		indexes.append(item->index());
		for(const QFileInfo &iter:info.dir().entryInfoList(QDir::Files,QDir::Name)){
			QString p=iter.absoluteFilePath();
			QString c=info.completeBaseName();
			QString o=iter.completeBaseName();
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
	QStandardItem *n=item(cur?cur->row()+1:0);
	return !n||(n->data(CodeRole).toInt()==Records&&!Config::getValue("/Playing/Continue",true));
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
	QFileInfo info(cur->data(FileRole).toString());
	for(const Record &r:Danmaku::instance()->getPool()){
		QStandardItem *d=new QStandardItem;
		QString danmaku=r.access;
		danmaku.replace(info.completeBaseName(),"%{File}").replace(info.absolutePath(),"%{Path}");
		d->setData(danmaku,CodeRole);
		d->setData(r.delay,TimeRole);
		cur->appendRow(d);
	}
}

void List::waste(const QModelIndex &index)
{
	if (itemFromIndex(index)==cur){
		return;
	}
	int self=index.row();
	int head=getGroupHead(index).row();
	int tail=getGroupTail(index).row();
	removeRow(self);
	switch(tail-head){
	case 0:
		return;
	case 1:
		clearItemGroup(head);
		return;
	default:
		--tail;
		item(tail)->setIcon(icons[2]);
		item(head)->setIcon(icons[0]);
		item(head)->setData(Records,CodeRole);
		return;
	}
}

void List::waste(const QModelIndexList &indexes)
{
	for(QStandardItem *item:getItems(indexes)){
		waste(item->index());
	}
}

void List::split(const QModelIndex &index)
{
	int self=index.row();
	int head=getGroupHead(index).row();
	int tail=getGroupTail(index).row();
	switch(tail-head){
	case 0:
		return;
	case 1:
		clearItemGroup(head);
		clearItemGroup(tail);
		return;
	default:
		insertRow(tail,takeRow(self));
		clearItemGroup(tail);
		--tail;
		item(tail)->setIcon(icons[2]);
		item(head)->setIcon(icons[0]);
		item(head)->setData(Records,CodeRole);
		return;
	}
}

void List::split(const QModelIndexList &indexes)
{
	QList<QStandardItem *> items=getItems(indexes);
	while(!items.isEmpty()){
		split(items.takeLast()->index());
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

void List::jumpToLast()
{
	int rc=rowCount();
	int i=cur?cur->row():rc;
	jumpToIndex(index((i+rc-1)%rc,0));
}

void List::jumpToNext()
{
	int rc=rowCount();
	int i=cur?cur->row():-1;
	jumpToIndex(index((i+rc+1)%rc,0));
}

bool List::jumpToIndex(const QModelIndex &index,bool manually)
{
	QStandardItem *head=itemFromIndex(index);
	if(!head||(manually&&head->data(CodeRole).toInt()==Inherit)){
		return false;
	}
	APlayer::instance()->setMedia(head->data(FileRole).toString(),manually);
	return true;
}
