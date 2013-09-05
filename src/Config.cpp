/*=======================================================================
*
*   Copyright (C) 2013 Lysine.
*
*   Filename:    Config.cpp
*   Time:        2013/06/17
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

#include "Config.h"

Config::Config(QWidget *parent,int index):
	QDialog(parent)
{
	setWindowTitle(tr("Config"));
	auto outer=new QGridLayout(this);
	tab=new QTabWidget(this);
	outer->addWidget(tab);
	//Playing
	{
		widget[0]=new QWidget(this);
		auto list=new QVBoxLayout(widget[0]);
		auto c=new QHBoxLayout;
		danm[0]=new QCheckBox(tr("clear when reloading"),widget[0]);
		danm[0]->setChecked(Utils::getConfig("/Playing/Clear",true));
		connect(danm[0],&QCheckBox::stateChanged,[this](int state){
			Utils::setConfig("/Playing/Clear",state==Qt::Checked);
		});
		c->addWidget(danm[0]);
		danm[1]=new QCheckBox(tr("auto delay after loaded"),widget[0]);
		danm[1]->setChecked(Utils::getConfig("/Playing//Delay",false));
		connect(danm[1],&QCheckBox::stateChanged,[this](int state){
			Utils::setConfig("/Playing/Delay",state==Qt::Checked);
		});
		c->addWidget(danm[1]);
		box[0]=new QGroupBox(tr("Loading"),widget[0]);
		box[0]->setLayout(c);
		list->addWidget(box[0]);

		auto s=new QHBoxLayout;
		play[0]=new QLineEdit(widget[0]);
		play[0]->setText(Utils::getConfig("/Danmaku/Speed",QString("125+%{width}/5")));
		connect(play[0],&QLineEdit::editingFinished,[this](){
			Utils::setConfig("/Danmaku/Speed",play[0]->text());
		});
		s->addWidget(play[0]);
		box[1]=new QGroupBox(tr("slide speed"),widget[0]);
		box[1]->setToolTip(tr("%{width} means the width of an danmaku"));
		box[1]->setLayout(s);
		list->addWidget(box[1]);

		auto l=new QHBoxLayout;
		play[1]=new QLineEdit(widget[0]);
		play[1]->setText(Utils::getConfig("/Danmaku/Life",QString("5")));
		connect(play[1],&QLineEdit::editingFinished,[this](){
			Utils::setConfig("/Danmaku/Life",play[1]->text());
		});
		l->addWidget(play[1]);
		box[2]=new QGroupBox(tr("life time"),widget[0]);
		box[2]->setToolTip(tr("%{width} means the width of an danmaku"));
		box[2]->setLayout(l);
		list->addWidget(box[2]);

		auto e=new QHBoxLayout;
		play[2]=new QLineEdit(widget[0]);
		play[2]->setText(QString::number(Utils::getConfig("/Danmaku/Scale",1.0),'f',2));
		connect(play[2],&QLineEdit::editingFinished,[this](){
			Utils::setConfig("/Danmaku/Scale",play[2]->text().toDouble());
		});
		e->addWidget(play[2]);
		box[3]=new QGroupBox(tr("force scale"),widget[0]);
		box[3]->setLayout(e);
		list->addWidget(box[3]);

		auto g=new QHBoxLayout;
		effect=new QComboBox(widget[0]);
		effect->addItem(tr("Stroke"));
		effect->addItem(tr("Stroke")+"&"+tr("Bold"));
		effect->addItem(tr("Projection"));
		effect->addItem(tr("Projection")+"&"+tr("Bold"));
		effect->setCurrentIndex(Utils::getConfig("/Danmaku/Effect",1));
		connect<void (QComboBox::*)(int)>(effect,&QComboBox::currentIndexChanged,[this](int i){
			Utils::setConfig("/Danmaku/Effect",i);
		});
		g->addWidget(effect);
		box[4]=new QGroupBox(tr("Style"),widget[0]);
		box[4]->setLayout(g);
		list->addWidget(box[4]);

		list->addStretch(10);
		tab->addTab(widget[0],tr("Playing"));
	}
	//Interface
	{
		widget[1]=new QWidget(this);
		auto lines=new QVBoxLayout(widget[1]);

		auto s=new QHBoxLayout;
		size=new QLineEdit(widget[1]);
		size->setText(Utils::getConfig("/Interface/Size",QString("960,540")));
		connect(size,&QLineEdit::editingFinished,[this](){
			Utils::setConfig("/Interface/Size",size->text());
		});
		s->addWidget(size);
		ui[0]=new QGroupBox(tr("initialize size"),widget[1]);
		ui[0]->setLayout(s);
		lines->addWidget(ui[0]);

		auto f=new QHBoxLayout;
		font=new QComboBox(widget[1]);
		font->addItems(QFontDatabase().families());
		font->setCurrentText(Utils::getConfig("/Interface/Font",QFont().family()));
		connect(font,&QComboBox::currentTextChanged,[this](QString _font){
			Utils::setConfig("/Interface/Font",_font);
		});
		f->addWidget(font);
		ui[1]=new QGroupBox(tr("interface font"),widget[1]);
		ui[1]->setLayout(f);
		lines->addWidget(ui[1]);

		auto t=new QHBoxLayout;
		stay=new QCheckBox(tr("stay on top"),widget[1]);
		stay->setChecked(Utils::getConfig("/Interface/Top",false));
		connect(stay,&QCheckBox::stateChanged,[this](int state){
			Utils::setConfig<bool>("/Interface/Top",state==Qt::Checked);
		});
		t->addWidget(stay);
		less=new QCheckBox(tr("frameless"),widget[1]);
		less->setChecked(Utils::getConfig("/Interface/Frameless",false));
		connect(less,&QCheckBox::stateChanged,[this](int state){
			Utils::setConfig<bool>("/Interface/Frameless",state==Qt::Checked);
		});
		t->addWidget(less);
		ui[2]=new QGroupBox(tr("window flag"),widget[1]);
		ui[2]->setLayout(t);
		lines->addWidget(ui[2]);

		auto j=new QHBoxLayout;
		jump=new QLineEdit(widget[1]);
		jump->setText(QString::number(Utils::getConfig("/Playing/Interval",10),'f',2));
		connect(jump,&QLineEdit::editingFinished,[this](){
			Utils::setConfig("/Playing/Interval",jump->text().toDouble());
		});
		j->addWidget(jump);
		ui[3]=new QGroupBox(tr("skip time"),widget[1]);
		ui[3]->setLayout(j);
		lines->addWidget(ui[3]);

		auto k=new QHBoxLayout;
		appk=new QLineEdit(widget[1]);
		appk->setText(Utils::getConfig("/Playing/Appkey",QString()));
		connect(appk,&QLineEdit::editingFinished,[this](){
			Utils::setConfig("/Playing/Appkey",appk->text());
		});
		k->addWidget(appk);
		ui[4]=new QGroupBox(tr("bilibili appkey"),widget[1]);
		ui[4]->setLayout(k);
		lines->addWidget(ui[4]);

		lines->addStretch(10);
		tab->addTab(widget[1],tr("Interface"));
	}
	//Shield
	{
		widget[2]=new QWidget(this);
		QStringList list={tr("Top"),tr("Bottom"),tr("Slide"),tr("Guest"),tr("Color"),tr("Whole")};
		auto lines=new QVBoxLayout(widget[2]);

		auto g=new QHBoxLayout;
		for(int i=0;i<6;++i){
			check[i]=new QCheckBox(list[i],widget[2]);
			check[i]->setFixedHeight(40);
			check[i]->setChecked(Shield::block[i]);
			connect(check[i],&QCheckBox::stateChanged,[=](int state){
				Shield::block[i]=state==Qt::Checked;
			});
			g->addWidget(check[i]);
		}
		lines->addLayout(g);

		edit=new QLineEdit(widget[2]);
		edit->setFixedHeight(25);
		regexp=new QListView(widget[2]);
		sender=new QListView(widget[2]);
		regexp->setModel(rm=new QStringListModel(regexp));
		sender->setModel(sm=new QStringListModel(sender));
		QStringList re;
		for(const auto &item:Shield::shieldR){
			re.append(item.pattern());
		}
		rm->setStringList(re);
		sm->setStringList(Shield::shieldU);
		action[0]=new QAction(tr("Add"),widget[2]);
		action[1]=new QAction(tr("Del"),widget[2]);
		action[2]=new QAction(tr("Import"),widget[2]);
		action[1]->setShortcut(QKeySequence("Del"));
		action[2]->setShortcut(QKeySequence("Ctrl+I"));
		connect(action[0],&QAction::triggered,[this](){
			if(!edit->text().isEmpty()){
				rm->insertRow(rm->rowCount());
				rm->setData(rm->index(rm->rowCount()-1),edit->text());
				edit->clear();
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
			QString path=QFileDialog::getOpenFileName(parentWidget(),tr("Import File"),QDir::homePath());
			if(!path.isEmpty()){
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
		});
		widget[2]->addAction(action[1]);
		widget[2]->addAction(action[2]);
		button[0]=new QPushButton(tr("Add"),widget[2]);
		button[1]=new QPushButton(tr("Del"),widget[2]);
		button[0]->setFixedHeight(25);
		button[1]->setFixedHeight(25);
		button[0]->setFocusPolicy(Qt::NoFocus);
		button[1]->setFocusPolicy(Qt::NoFocus);
		connect(button[0],&QPushButton::clicked,action[0],&QAction::trigger);
		connect(button[1],&QPushButton::clicked,action[1],&QAction::trigger);
		widget[2]->setContextMenuPolicy(Qt::ActionsContextMenu);
		auto l=new QVBoxLayout;
		l->addWidget(edit);
		l->addWidget(regexp);
		auto r=new QVBoxLayout;
		auto b=new QHBoxLayout;
		b->addWidget(button[0]);
		b->addWidget(button[1]);
		r->addLayout(b);
		r->addWidget(sender);
		auto s=new QHBoxLayout;
		s->addLayout(l,8);
		s->addLayout(r,1);
		lines->addLayout(s);

		limit[0]=new QLineEdit(widget[2]);
		limit[0]->setText(QString::number(Utils::getConfig("/Shield/Limit",0.005)));
		connect(limit[0],&QLineEdit::editingFinished,[this](){
			Utils::setConfig("/Shield/Limit",limit[0]->text().toDouble());
		});
		auto a=new QHBoxLayout;
		a->addWidget(limit[0]);
		label[0]=new QGroupBox(tr("limit of the same"),widget[2]);
		label[0]->setToolTip(tr("0 means disabled"));
		label[0]->setLayout(a);
		lines->addWidget(label[0]);

		limit[1]=new QLineEdit(widget[2]);
		limit[1]->setText(QString::number(Utils::getConfig("/Shield/Density",80)));
		connect(limit[1],&QLineEdit::editingFinished,[this](){
			Utils::setConfig("/Shield/Density",limit[1]->text().toInt());
		});
		auto d=new QHBoxLayout;
		d->addWidget(limit[1]);
		label[1]=new QGroupBox(tr("limit of density"),widget[2]);
		label[1]->setToolTip(tr("0 means disabled"));
		label[1]->setLayout(d);
		lines->addWidget(label[1]);

		tab->addTab(widget[2],tr("Shield"));
	}
	//Thanks
	{
		widget[3]=new QWidget(this);
		auto w=new QGridLayout(widget[3]);
		QFile t(":/Text/THANKS");
		t.open(QIODevice::ReadOnly|QIODevice::Text);
		thanks=new QTextEdit(widget[3]);
		thanks->setReadOnly(true);
		thanks->setText(QString(t.readAll()).replace("\n","<br>"));
		w->addWidget(thanks);
		tab->addTab(widget[3],tr("Thanks"));
	}
	//License
	{
		widget[4]=new QWidget(this);
		auto w=new QGridLayout(widget[4]);
		QFile l(":/Text/COPYING");
		l.open(QIODevice::ReadOnly|QIODevice::Text);
		license=new QTextEdit(widget[4]);
		license->setReadOnly(true);
		license->setText(QString("<center><font size=\"2\">%1</font></center>").arg(QString(l.readAll()).replace("\n","<br>")));
		w->addWidget(license);
		tab->addTab(widget[4],tr("License"));
	}
	tab->setCurrentIndex(index);
	connect(this,&QDialog::finished,[this](){
		Shield::shieldR.clear();
		Shield::shieldU.clear();
		for(QString item:rm->stringList()){
			Shield::shieldR.append(QRegExp(item));
		}
		for(QString item:sm->stringList()){
			Shield::shieldU.append(item);
		}
		Shield::cacheS.clear();
	});
	resize(540,outer->minimumSize().height());
	Utils::setCenter(this);
}
