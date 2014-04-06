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
#include "Utils.h"
#include "Shield.h"
#include "Cookie.h"
#include "Danmaku.h"
#include "VPlayer.h"

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
		auto c=new QGridLayout;
		load[0]=new QCheckBox(tr("clear when reloading"),widget[0]);
		load[0]->setChecked(Utils::getConfig("/Playing/Clear",true));
		connect(load[0],&QCheckBox::stateChanged,[this](int state){
			Utils::setConfig("/Playing/Clear",state==Qt::Checked);
		});
		c->addWidget(load[0],0,0);
		load[1]=new QCheckBox(tr("auto delay after loaded"),widget[0]);
		load[1]->setChecked(Utils::getConfig("/Playing/Delay",false));
		connect(load[1],&QCheckBox::stateChanged,[this](int state){
			Utils::setConfig("/Playing/Delay",state==Qt::Checked);
		});
		c->addWidget(load[1],0,1);
		load[2]=new QCheckBox(tr("load local subtitles"),widget[0]);
		load[2]->setChecked(Utils::getConfig("/Playing/Subtitle",true));
		connect(load[2],&QCheckBox::stateChanged,[this](int state){
			Utils::setConfig("/Playing/Subtitle",state==Qt::Checked);
		});
		c->addWidget(load[2],1,0);
		load[3]=new QCheckBox(tr("auto play after loaded"),widget[0]);
		load[3]->setChecked(Utils::getConfig("/Playing/Immediate",false));
		connect(load[3],&QCheckBox::stateChanged,[this](int state){
			Utils::setConfig("/Playing/Immediate",state==Qt::Checked);
		});
		c->addWidget(load[3],1,1);
		box[0]=new QGroupBox(tr("loading"),widget[0]);
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
		int state=Utils::getConfig("/Danmaku/Scale/Fitted",0x1);
		fitted[0]=new QCheckBox(tr("ordinary"),widget[0]);
		fitted[0]->setChecked((state&0x2)>0);
		fitted[1]=new QCheckBox(tr("advanced"),widget[0]);
		fitted[1]->setChecked((state&0x1)>0);
		auto slot=[this](){
			int n=fitted[0]->checkState()==Qt::Checked;
			int a=fitted[1]->checkState()==Qt::Checked;
			Utils::setConfig("/Danmaku/Scale/Fitted",(n<<1)+a);
		};
		connect(fitted[0],&QCheckBox::stateChanged,slot);
		connect(fitted[1],&QCheckBox::stateChanged,slot);
		e->addWidget(fitted[0],1);
		e->addWidget(fitted[1],1);
		box[3]=new QGroupBox(tr("scale to fitted"),widget[0]);
		box[3]->setLayout(e);

		auto a=new QHBoxLayout;
		factor=new QLineEdit(widget[0]);
		factor->setText(QString::number(Utils::getConfig("/Danmaku/Scale/Factor",1.0),'f',2));
		connect(factor,&QLineEdit::editingFinished,[this](){
			Utils::setConfig("/Danmaku/Scale/Factor",factor->text().toDouble());
		});
		a->addWidget(factor);
		box[4]=new QGroupBox(tr("scale by factor"),widget[0]);
		box[4]->setLayout(a);

		auto o=new QHBoxLayout;
		o->addWidget(box[3],1);
		o->addWidget(box[4],1);
		list->addLayout(o);

		auto g=new QHBoxLayout;
		int ef=Utils::getConfig("/Danmaku/Effect",5);
		bold=new QCheckBox(tr("Bold"),widget[0]);
		bold->setChecked(ef&1);
		connect(bold,&QCheckBox::stateChanged,[this](int s){
			Utils::setConfig("/Danmaku/Effect",(effect->currentIndex()<<1)|(int)(s==Qt::Checked));
		});
		g->addWidget(bold);
		effect=new QComboBox(widget[0]);
		effect->addItem(tr("Stroke"));
		effect->addItem(tr("Projection"));
		effect->addItem(tr("Shadow"));
		effect->setCurrentIndex(ef>>1);
		connect<void (QComboBox::*)(int)>(effect,&QComboBox::currentIndexChanged,[this](int i){
			Utils::setConfig("/Danmaku/Effect",(i<<1)|(int)(bold->checkState()==Qt::Checked));
		});
		g->addWidget(effect);
		box[5]=new QGroupBox(tr("style"),widget[0]);
		box[5]->setLayout(g);

		auto f=new QHBoxLayout;
		dmfont=new QComboBox(widget[0]);
		dmfont->addItems(QFontDatabase().families());
		dmfont->setCurrentText(Utils::getConfig("/Danmaku/Font",QFont().family()));
		connect(dmfont,&QComboBox::currentTextChanged,[this](QString _font){
			Utils::setConfig("/Danmaku/Font",_font);
		});
		f->addWidget(dmfont);
		box[6]=new QGroupBox(tr("font"),widget[0]);
		box[6]->setLayout(f);

		auto v=new QHBoxLayout;
		v->addWidget(box[5],1);
		v->addWidget(box[6],1);
		list->addLayout(v);

		list->addStretch(10);
		tab->addTab(widget[0],tr("Playing"));
	}
	//Interface
	{
		widget[1]=new QWidget(this);
		auto lines=new QVBoxLayout(widget[1]);

		auto s=new QHBoxLayout;
		size=new QLineEdit(widget[1]);
		size->setText(Utils::getConfig("/Interface/Size",QString("960,540")).trimmed());
		connect(size,&QLineEdit::editingFinished,[this](){
			QRegularExpression r("\\D");
			r.setPatternOptions(QRegularExpression::UseUnicodePropertiesOption);
			QString s=size->text().trimmed();
			s.replace(r,",");
			size->setText(s);
			Utils::setConfig("/Interface/Size",s+" ");
		});
		s->addWidget(size);
		ui[0]=new QGroupBox(tr("initialize size"),widget[1]);
		ui[0]->setLayout(s);

		auto j=new QHBoxLayout;
		jump=new QLineEdit(widget[0]);
		jump->setText(QString::number(Utils::getConfig("/Interface/Interval",10),'f',2));
		connect(jump,&QLineEdit::editingFinished,[this](){
			Utils::setConfig("/Interface/Interval",jump->text().toDouble());
		});
		j->addWidget(jump);
		ui[1]=new QGroupBox(tr("skip time"),widget[1]);
		ui[1]->setLayout(j);

		auto q=new QHBoxLayout;
		q->addWidget(ui[0]);
		q->addWidget(ui[1]);
		lines->addLayout(q);

		auto t=new QGridLayout;
		acce=new QCheckBox(tr("hardware accelerated"),widget[1]);
		acce->setChecked(Utils::getConfig("/Interface/Accelerated",false));
		connect(acce,&QCheckBox::stateChanged,[this](int state){
			Utils::setConfig("/Interface/Accelerated",state==Qt::Checked);
		});
		t->addWidget(acce,0,0);
		vers=new QCheckBox(tr("version information"),widget[1]);
		vers->setChecked(Utils::getConfig("/Interface/Version",true));
		connect(vers,&QCheckBox::stateChanged,[this](int state){
			Utils::setConfig("/Interface/Version",state==Qt::Checked);
		});
		t->addWidget(vers,1,0);
		stay=new QCheckBox(tr("stay on top"),widget[1]);
		stay->setChecked(Utils::getConfig("/Interface/Top",false));
		connect(stay,&QCheckBox::stateChanged,[this](int state){
			Utils::setConfig("/Interface/Top",state==Qt::Checked);
		});
		t->addWidget(stay,0,1);
		less=new QCheckBox(tr("frameless"),widget[1]);
		less->setChecked(Utils::getConfig("/Interface/Frameless",false));
		connect(less,&QCheckBox::stateChanged,[this](int state){
			Utils::setConfig("/Interface/Frameless",state==Qt::Checked);
		});
		t->addWidget(less,1,1);
		ui[2]=new QGroupBox(tr("window flag"),widget[1]);
		ui[2]->setLayout(t);
		lines->addWidget(ui[2]);

		auto f=new QHBoxLayout;
		font=new QComboBox(widget[1]);
		font->addItems(QFontDatabase().families());
		font->setCurrentText(Utils::getConfig("/Interface/Font/Family",QFont().family()));
		connect(font,&QComboBox::currentTextChanged,[this](QString _font){
			Utils::setConfig("/Interface/Font/Family",_font);
		});
		f->addWidget(font);
		ui[3]=new QGroupBox(tr("interface font"),widget[1]);
		ui[3]->setLayout(f);

		auto r=new QHBoxLayout;
		reop=new QComboBox(widget[1]);
		reop->addItems(QStringList()<<tr("open in new window")<<tr("open in current window")<<tr("append to playing list"));
		reop->setCurrentIndex(Utils::getConfig("/Interface/Single",1));
		connect<void (QComboBox::*)(int)>(reop,&QComboBox::currentIndexChanged,[this](int i){
			Utils::setConfig("/Interface/Single",i);
		});
		r->addWidget(reop);
		ui[4]=new QGroupBox(tr("reopen action"),widget[1]);
		ui[4]->setLayout(r);

		auto v=new QHBoxLayout;
		v->addWidget(ui[3],1);
		v->addWidget(ui[4],1);
		lines->addLayout(v);

		auto b=new QHBoxLayout;
		back=new QLineEdit(widget[1]);
		back->setText(Utils::getConfig("/Interface/Background",QString()));
		connect(back,&QLineEdit::textChanged,[this](){
			Utils::setConfig("/Interface/Background",back->text());
		});
		b->addWidget(back);
		open=new QPushButton(tr("choose"),widget[1]);
		open->setFixedWidth(50);
		open->setFocusPolicy(Qt::NoFocus);
		connect(open,&QPushButton::clicked,[this](){
			QString path=back->text().isEmpty()?QDir::currentPath():QFileInfo(back->text()).absolutePath();
			QString file=QFileDialog::getOpenFileName(this,tr("Open File"),path);
			if(!file.isEmpty()){
				back->setText(file.startsWith(QDir::currentPath())?QDir::current().relativeFilePath(file):file);
			}
		});
		b->addWidget(open);
		ui[5]=new QGroupBox(tr("background"),widget[1]);
		ui[5]->setLayout(b);
		lines->addWidget(ui[5]);

		auto l=new QHBoxLayout;
		input[0]=new QLineEdit(widget[1]);
		input[0]->setPlaceholderText(tr("Username"));
		input[1]=new QLineEdit(widget[1]);
		input[1]->setPlaceholderText(tr("Password"));
		input[1]->setEchoMode(QLineEdit::Password);
		input[2]=new QLineEdit(widget[1]);
		input[2]->setPlaceholderText(tr("Identifier"));
		auto checkout=[this](){
			bool flag=true;
			for(QLineEdit *iter:input){
				if(iter->text().isEmpty()){
					flag=false;
					break;
				}
			}
			if(flag){
				click->click();
			}
		};
		for(QLineEdit *iter:input){
			connect(iter,&QLineEdit::editingFinished,checkout);
		}
		connect(input[2],&QLineEdit::textEdited,[this](QString text){
			input[2]->setText(text.toUpper());
		});
		l->addWidget(input[0]);
		l->addWidget(input[1]);
		l->addWidget(input[2]);
		info=new QLabel(widget[1]);
		info->setFixedWidth(100);
		info->setAlignment(Qt::AlignCenter);
		l->addWidget(info);
		manager=new QNetworkAccessManager(this);
		manager->setCookieJar(Cookie::instance());
		Cookie::instance()->setParent(NULL);
		auto loadValid=[this](){
			QString url=QString("https://secure.bilibili.tv/captcha?r=%1").arg(qrand()/(double)RAND_MAX);
			Utils::getReply(manager,QNetworkRequest(url),[this](QNetworkReply *reply){
				if(reply->error()==QNetworkReply::NoError){
					QPixmap pixmap;
					pixmap.loadFromData(reply->readAll());
					if(!pixmap.isNull()){
						info->setPixmap(pixmap.scaledToHeight(25,Qt::SmoothTransformation));
					}
				}
			});
		};
		auto setLogged=[this,loadValid](bool logged){
			info->clear();
			if(logged){
				info->setText(tr("logged"));
				input[1]->clear();
				input[2]->clear();
				click->setText(tr("logout"));
				setFocus();
			}
			else{
				loadValid();
				click->setText(tr("login"));
			}
			for(QLineEdit *iter:input){
				iter->setEnabled(!logged);
			}
		};
		auto sendLogin=[this,setLogged](){
			click->setEnabled(false);
			QUrlQuery query;
			query.addQueryItem("act","login");
			query.addQueryItem("userid",input[0]->text());
			query.addQueryItem("pwd",input[1]->text());
			query.addQueryItem("vdcode",input[2]->text());
			query.addQueryItem("keeptime","2592000");
			QByteArray data=query.query().toUtf8();
			QNetworkRequest request(QUrl("https://secure.bilibili.tv/login"));
			request.setHeader(QNetworkRequest::ContentTypeHeader,"application/x-www-form-urlencoded");
			request.setHeader(QNetworkRequest::ContentLengthHeader,data.length());
			QNetworkReply *reply=manager->post(request,data);
			connect(reply,&QNetworkReply::finished,[=](){
				bool flag=false;
				if(reply->error()==QNetworkReply::NoError){
					QString page(reply->readAll());
					if(page.indexOf("setTimeout('JumpUrl()',2000)")!=-1){
						flag=true;
					}
					else{
						int sta=page.indexOf("document.write(\"")+16;
						QMessageBox::warning(this,tr("Warning"),page.mid(sta,page.indexOf("\"",sta)-sta));
					}
				}
				click->setEnabled(true);
				setLogged(flag);
				reply->deleteLater();
			});
		};
		auto setLogout=[this,setLogged](){
			click->setEnabled(false);
			QString url="https://secure.bilibili.tv/login?act=exit";
			Utils::getReply(manager,QNetworkRequest(url),[=](QNetworkReply *reply){
				click->setEnabled(true);
				setLogged(reply->error()!=QNetworkReply::NoError);
			});
		};
		click=new QPushButton(tr("login"),widget[1]);
		click->setFixedWidth(50);
		click->setFocusPolicy(Qt::NoFocus);
		connect(click,&QPushButton::clicked,[=](){
			if(info->text().isEmpty()){
				for(QLineEdit *iter:input){
					if(iter->text().isEmpty()){
						return;
					}
				}
				sendLogin();
			}
			else{
				setLogout();
			}
		});
		Utils::getReply(manager,QNetworkRequest(QUrl("http://member.bilibili.tv/")),[=](QNetworkReply *reply){
			bool flag=false;
			if(reply->error()==QNetworkReply::NoError){
				QString page(reply->readAll());
				int sta=page.indexOf("<em>");
				if(sta!=-1){
					sta+=4;
					input[0]->setText(page.mid(sta,page.indexOf("<",sta)-sta));
					flag=true;
				}
			}
			setLogged(flag);
		});
		l->addWidget(click);
		ui[6]=new QGroupBox(tr("login"),widget[1]);
		ui[6]->setLayout(l);
		lines->addWidget(ui[6]);

		lines->addStretch(10);
		tab->addTab(widget[1],tr("Interface"));

		restart=getRestart();
	}
	//Shield
	{
		widget[2]=new QWidget(this);
		QStringList list;
		list<<tr("Top")<<tr("Bottom")<<tr("Slide")<<tr("Reverse")<<tr("Guest")<<tr("Advanced")<<tr("Color")<<tr("Whole");
		auto grid=new QGridLayout(widget[2]);

		auto g=new QHBoxLayout;
		for(int i=0;i<8;++i){
			check[i]=new QCheckBox(list[i],widget[2]);
			check[i]->setFixedHeight(40);
			check[i]->setChecked(Shield::shieldG[i]);
			connect(check[i],&QCheckBox::stateChanged,[=](int state){
				Shield::shieldG[i]=state==Qt::Checked;
			});
			g->addWidget(check[i]);
		}
		grid->addLayout(g,0,0,1,4);

		type=new QComboBox(widget[2]);
		type->addItem(tr("Text"));
		type->addItem(tr("User"));
		edit=new QLineEdit(widget[2]);
		edit->setFixedHeight(25);
		regexp=new QListView(widget[2]);
		sender=new QListView(widget[2]);
		regexp->setSelectionMode(QListView::ExtendedSelection);
		sender->setSelectionMode(QListView::ExtendedSelection);
		regexp->setModel(rm=new QStringListModel(regexp));
		sender->setModel(sm=new QStringListModel(sender));
		Utils::setSelection(regexp);
		Utils::setSelection(sender);
		connect(regexp,&QListView::pressed,[this](QModelIndex){sender->setCurrentIndex(QModelIndex());});
		connect(sender,&QListView::pressed,[this](QModelIndex){regexp->setCurrentIndex(QModelIndex());});
		QStringList r,s;
		for(const auto &i:Shield::shieldR){
			r.append(i.pattern());
		}
		for(const auto &i:Shield::shieldS){
			s.append(i);
		}
		std::sort(s.begin(),s.end());
		rm->setStringList(r);
		sm->setStringList(s);
		action[0]=new QAction(tr("Add"),widget[2]);
		action[1]=new QAction(tr("Del"),widget[2]);
		action[2]=new QAction(tr("Import"),widget[2]);
		action[3]=new QAction(tr("Export"),widget[2]);
		action[1]->setShortcut(QKeySequence("Del"));
		action[2]->setShortcut(QKeySequence("Ctrl+I"));
		action[3]->setShortcut(QKeySequence("Ctrl+E"));
		connect(action[0],&QAction::triggered,[this](){
			if(!edit->text().isEmpty()){
				QStringListModel *m=type->currentIndex()==0?rm:sm;
				m->insertRow(m->rowCount());
				m->setData(m->index(m->rowCount()-1),edit->text());
				edit->clear();
			}
		});
		connect(action[1],&QAction::triggered,[this](){
			auto remove=[this](QListView *v){
				QList<int> rows;
				for(const QModelIndex &i:v->selectionModel()->selectedRows()){
					rows.append(i.row());
				}
				std::sort(rows.begin(),rows.end());
				while(!rows.isEmpty()){
					int r=rows.takeFirst();
					v->model()->removeRow(r);
					for(int &i:rows){
						if(i>r){
							--i;
						}
					}
				}
			};
			if(regexp->hasFocus()){
				remove(regexp);
			}
			if(sender->hasFocus()){
				remove(sender);
			}
		});
		connect(action[2],&QAction::triggered,[this](){
			QString path=QFileDialog::getOpenFileName(this,tr("Import File"),QDir::homePath());
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
					regexp->setCurrentIndex(QModelIndex());
					sender->setCurrentIndex(QModelIndex());
				}
			}
		});
		connect(action[3],&QAction::triggered,[this](){
			QString path=QFileDialog::getSaveFileName(this,tr("Export File"),QDir::homePath()+"/shield.bililocal.xml");
			if(!path.isEmpty()){
				if(!path.endsWith(".xml")){
					path.append(".xml");
				}
				QFile file(path);
				file.open(QIODevice::WriteOnly|QIODevice::Text);
				QTextStream stream(&file);
				stream.setCodec("UTF-8");
				stream<<"<filters>"<<endl;
				for(const QString &iter:rm->stringList()){
					stream<<"  <item enabled=\"true\">t="<<iter<<"</item>"<<endl;
				}
				for(const QString &iter:sm->stringList()){
					stream<<"  <item enabled=\"true\">u="<<iter<<"</item>"<<endl;
				}
				stream<<"</filters>";
			}
		});
		widget[2]->addAction(action[2]);
		widget[2]->addAction(action[3]);
		button[0]=new QPushButton(tr("Add"),widget[2]);
		button[1]=new QPushButton(tr("Del"),widget[2]);
		button[0]->setFixedWidth(60);
		button[1]->setFixedWidth(60);
		button[0]->setFocusPolicy(Qt::NoFocus);
		button[1]->setFocusPolicy(Qt::NoFocus);
		connect(button[0],&QPushButton::clicked,action[0],&QAction::trigger);
		connect(button[1],&QPushButton::clicked,action[1],&QAction::trigger);
		regexp->addActions(widget[2]->actions());
		sender->addActions(widget[2]->actions());
		regexp->setContextMenuPolicy(Qt::ActionsContextMenu);
		sender->setContextMenuPolicy(Qt::ActionsContextMenu);

		grid->addWidget(type,1,0);
		grid->addWidget(edit,1,1);
		grid->addWidget(button[0],1,2);
		grid->addWidget(button[1],1,3);
		grid->addWidget(regexp,2,0,1,2);
		grid->addWidget(sender,2,2,1,2);

		limit[0]=new QLineEdit(widget[2]);
		limit[0]->setText(QString::number(Utils::getConfig("/Shield/Limit",5)));
		connect(limit[0],&QLineEdit::editingFinished,[this](){
			Utils::setConfig("/Shield/Limit",limit[0]->text().toInt());
		});
		auto a=new QHBoxLayout;
		a->addWidget(limit[0]);
		label[0]=new QGroupBox(tr("limit of the same"),widget[2]);
		label[0]->setToolTip(tr("0 means disabled"));
		label[0]->setLayout(a);
		grid->addWidget(label[0],3,0,1,4);

		limit[1]=new QLineEdit(widget[2]);
		limit[1]->setText(QString::number(Utils::getConfig("/Shield/Density",100)));
		connect(limit[1],&QLineEdit::editingFinished,[this](){
			Utils::setConfig("/Shield/Density",limit[1]->text().toInt());
		});
		auto d=new QHBoxLayout;
		d->addWidget(limit[1]);
		label[1]=new QGroupBox(tr("limit of density"),widget[2]);
		label[1]->setToolTip(tr("0 means disabled"));
		label[1]->setLayout(d);
		grid->addWidget(label[1],4,0,1,4);

		tab->addTab(widget[2],tr("Shield"));

		reparse=getReparse();
	}
	//Thanks
	{
		widget[3]=new QWidget(this);
		auto w=new QGridLayout(widget[3]);
		QFile t(":/Text/THANKS");
		t.open(QIODevice::ReadOnly|QIODevice::Text);
		thanks=new QTextEdit(widget[3]);
		thanks->setReadOnly(true);
		thanks->setText(t.readAll());
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
		license->setText(l.readAll());
		w->addWidget(license);
		tab->addTab(widget[4],tr("License"));
	}
	tab->setCurrentIndex(index);
	connect(this,&QDialog::finished,[this](){
		if(reparse!=getReparse()){
			Shield::shieldR.clear();
			for(QString item:rm->stringList()){
				Shield::shieldR.append(QRegularExpression(item));
			}
			Shield::shieldS=sm->stringList().toSet();
			Danmaku::instance()->parse(0x2);
		}
		if(restart!=getRestart()){
			bool flag=VPlayer::instance()->getState()==VPlayer::Stop&&Danmaku::instance()->rowCount()==0;
			if(flag||QMessageBox::warning(this,
											tr("Warning"),
											tr("Restart to apply changes?"),
											QMessageBox::Yes,QMessageBox::No)==QMessageBox::Yes){
				Danmaku::instance()->release();
				qApp->exit(12450);
			}
		}
	});
	setMinimumWidth(540);
	resize(540,outer->minimumSize().height());
	Utils::setCenter(this);
}

QHash<QString,QVariant> Config::getRestart()
{
	QStringList path;
	path<<"/Interface/Accelerated"<<
		  "/Interface/Background"<<
		  "/Interface/Font"<<
		  "/Interface/Frameless"<<
		  "/Interface/Single"<<
		  "/Interface/Top"<<
		  "/Interface/Version";
	QHash<QString,QVariant> data;
	for(QString iter:path){
		data[iter]=Utils::getConfig<QVariant>(iter);
	}
	return data;
}

QHash<QString,QVariant> Config::getReparse()
{
	QHash<QString,QVariant> data;
	int g=0;
	for(int i=0;i<8;++i){
		g=(g<<1)+Shield::shieldG[i];
	}
	data["/Shield/Group"]=g;
	data["/Shield/Regexp"]=rm->stringList();
	data["/Shield/Sender"]=sm->stringList();
	data["/Shield/Limit"]=Utils::getConfig("/Shield/Limit",5);
	return data;
}
