/*=======================================================================
*
*   Copyright (C) 2013 Lysine.
*
*   Filename:    Shield.cpp
*   Time:        2013/05/20
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

#include "Shield.h"

Shield *Shield::instance=NULL;

Shield::Shield()
{
	QJsonObject shield=Utils::getConfig("Shield");
	for(const auto &item:shield["User"].toArray()){
		shieldU.append(item.toString());
	}
	for(const auto &item:shield["Regex"].toArray()){
		shieldR.append(QRegExp(item.toString()));
	}
	int group=shield["Group"].toDouble();
	for(int i=5;i>=0;--i){
		block[i]=group&1;
		group=group>>1;
	}
	instance=this;
}

Shield::~Shield()
{
	QJsonObject shield;
	QJsonArray u,r;
	for(auto &item:shieldU){
		u.append(item);
	}
	for(auto &item:shieldR){
		r.append(item.pattern());
	}
	shield.insert("User",u);
	shield.insert("Regex",r);
	int g=0;
	for(int i=0;i<6;++i){
		g=(g<<1)+block[i];
	}
	shield.insert("Group",g);
	Utils::setConfig(shield,"Shield");
	instance=NULL;
}

void Shield::configure(QWidget *parent)
{
	if(instance){
		Editor edit(instance,parent);
		edit.exec();
		instance->shieldR.clear();
		instance->shieldU.clear();
		for(QString item:edit.getRegexp()){
			instance->shieldR.append(QRegExp(item));
		}
		for(QString item:edit.getSender()){
			instance->shieldU.append(item);
		}
	}
}

bool Shield::isBlocked(const Comment &comment)
{
	if(instance){
		if(instance->block[Whole]||comment.mode>5
				||(comment.mode==1&&instance->block[Slide])
				||(comment.mode==4&&instance->block[Bottom])
				||(comment.mode==5&&instance->block[Top])
				||(comment.sender.startsWith('D',Qt::CaseInsensitive)&&instance->block[Guest])
				||(comment.color!=Qt::white&&instance->block[Color])){
			return true;
		}
		else{
			bool flag=false;
			for(const QRegExp &reg:instance->shieldR){
				if(reg.indexIn(comment.content)!=-1){
					flag=true;
					break;
				}
			}
			for(const QString &name:instance->shieldU){
				if(name==comment.sender){
					flag=true;
					break;
				}
			}
			return flag;
		}
	}
	else{
		return false;
	}
}

Editor::Editor(Shield *shield,QWidget *parent):
	QDialog(parent)
{
	setAcceptDrops(true);
	setFixedSize(400,300);
	QStringList list;
	list.append(tr("Top"));
	list.append(tr("Bottom"));
	list.append(tr("Slide"));
	list.append(tr("Guest"));
	list.append(tr("Color"));
	list.append(tr("Whole"));
	auto block=shield->block;
	for(int i=0;i<6;++i){
		check[i]=new QCheckBox(list[i],this);
		check[i]->setGeometry(10+i*65,15,55,20);
		check[i]->setChecked(block[i]);
		connect(check[i],&QCheckBox::stateChanged,[=](int state){
			block[i]=state==Qt::Checked;
		});
	}
	edit=new QLineEdit(this);
	edit->setGeometry(10,50,290,25);
	regexp=new QListView(this);
	sender=new QListView(this);
	regexp->setModel(rm=new QStringListModel(regexp));
	sender->setModel(sm=new QStringListModel(sender));
	regexp->setGeometry(10,80,290,210);
	sender->setGeometry(310,80,80,210);
	QStringList r;
	for(const auto &item:shield->shieldR){
		r.append(item.pattern());
	}
	rm->setStringList(r);
	sm->setStringList(shield->shieldU);

	action[0]=new QAction(tr("Add"),this);
	action[1]=new QAction(tr("Del"),this);
	action[2]=new QAction(tr("Import"),this);
	action[1]->setShortcut(QKeySequence("Del"));
	action[2]->setShortcut(QKeySequence("Ctrl+I"));
	connect(action[0],&QAction::triggered,[this](){
		if(!edit->text().isEmpty()){
			rm->insertRow(rm->rowCount());
			rm->setData(rm->index(rm->rowCount()-1),edit->text());
		}
	});
	connect(action[1],&QAction::triggered,[this](){
		if(regexp->hasFocus()){
			rm->removeRow(regexp->currentIndex().row());
		}
		if(sender->hasFocus()){
			sm->removeRow(sender->currentIndex().row());
		}
	});
	connect(action[2],&QAction::triggered,[this](){
		QString file=QFileDialog::getOpenFileName(parentWidget(),tr("Import File"),QDir::homePath());
		if(!file.isEmpty()){
			import(file);
		}
	});
	addAction(action[1]);
	addAction(action[2]);
	button[0]=new QPushButton(tr("Add"),this);
	button[1]=new QPushButton(tr("Del"),this);
	button[0]->setGeometry(310,50,38,25);
	button[1]->setGeometry(352,50,38,25);
	button[0]->setFocusPolicy(Qt::NoFocus);
	button[1]->setFocusPolicy(Qt::NoFocus);
	connect(button[0],&QPushButton::clicked,action[0],&QAction::trigger);
	connect(button[1],&QPushButton::clicked,action[1],&QAction::trigger);
	setContextMenuPolicy(Qt::ActionsContextMenu);
}

void Editor::import(QString path)
{
	QFile file(path);
	if(file.exists()){
		file.open(QIODevice::ReadOnly|QIODevice::Text);
		QTextStream stream(&file);
		stream.setCodec("UTF-8");
		QString all=stream.readAll();
		file.close();
		QRegExp del(QString("[")+QChar(0)+"-"+QChar(31)+"]");
		all.replace(del," ");
		all.replace("</item>"," ");
		all=all.simplified();
		QRegExp fix("[tu]\\=\\S*");
		int cur=0;
		while((cur=fix.indexIn(all,cur))!=-1){
			int len=fix.matchedLength();
			QString item=all.mid(cur,len);
			QString text=item.mid(2);
			cur+=len;
			if(item.startsWith("u=")&&!sm->stringList().contains(text)){
				sm->insertRow(sm->rowCount());
				sm->setData(sm->index(sm->rowCount()-1),text);
			}
			if(item.startsWith("t=")&&!rm->stringList().contains(text)){
				rm->insertRow(rm->rowCount());
				rm->setData(rm->index(rm->rowCount()-1),text);
			}
		}
	}
}

void Editor::dropEvent(QDropEvent *e)
{
	if(e->mimeData()->hasFormat("text/uri-list")){
		QString drop(e->mimeData()->data("text/uri-list"));
		QStringList list=drop.split('\n');
		for(QString &item:list){
			QString file=QUrl(item).toLocalFile().trimmed();
			if(file.endsWith(".sol")){
				import(file);
			}
		}
	}
}

void Editor::dragEnterEvent(QDragEnterEvent *e)
{
	if(e->mimeData()->hasFormat("text/uri-list")){
		e->acceptProposedAction();
	}
}
