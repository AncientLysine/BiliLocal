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
#include "APlayer.h"
#include "Danmaku.h"
#include "Local.h"
#include "Plugin.h"
#include "Render.h"
#include "Shield.h"
#include "Utils.h"
#include <algorithm>

Config *Config::ins=nullptr;

Config *Config::instance()
{
	return ins?ins:new Config(qApp);
}

Config::Config(QObject *parent):
	QObject(parent)
{
	ins=this;
}

namespace
{
class Cookie:public QNetworkCookieJar
{
public:
	Cookie(QObject *parent=0):
		QNetworkCookieJar(parent)
	{
	}

	static void load()
	{
		data=new Cookie(qApp);
		QByteArray buff;
		buff=Config::getValue("/Network/Cookie",QString()).toUtf8();
		buff=buff.isEmpty()?buff:qUncompress(QByteArray::fromBase64(buff));
		QDataStream read(buff);
		QList<QNetworkCookie> all;
		int n,l;
		read>>n;
		for(int i=0;i<n;++i){
			read>>l;
			char *d=new char[l];
			read.readRawData(d,l);
			all.append(QNetworkCookie::parseCookies(QByteArray(d,l)));
			delete []d;
		}
		data->setAllCookies(all);
	}

	static void save()
	{

		QByteArray buff;
		QDataStream save(&buff,QIODevice::WriteOnly);
		const QList<QNetworkCookie> &all=data->allCookies();
		save<<all.count();
		for(const QNetworkCookie &iter:all){
			QByteArray d=iter.toRawForm();
			save<<d.size();
			save.writeRawData(d.data(),d.size());
		}
		Config::setValue("/Network/Cookie",QString(qCompress(buff).toBase64()));
	}

	static Cookie *data;
};

Cookie *Cookie::data=NULL;

class DCache:public QNetworkDiskCache
{
public:
	DCache(QObject *parent=0):
		QNetworkDiskCache(parent)
	{
	}

	static void load()
	{
		data=new DCache(qApp);
		data->setCacheDirectory("./cache");
		data->setMaximumCacheSize(Config::getValue("/Network/Cache/Maximum",100*1024*1024));
	}

	static DCache *data;
};

DCache *DCache::data=NULL;

class APorxy
{
public:
	static void load()
	{
		QNetworkProxy proxy;
		proxy.setType((QNetworkProxy::ProxyType)Config::getValue<int>("/Network/Proxy/Type",QNetworkProxy::NoProxy));
		proxy.setHostName(Config::getValue<QString>("/Network/Proxy/HostName"));
		proxy.setPort(Config::getValue<quint16>("/Network/Proxy/Port"));
		proxy.setUser(Config::getValue<QString>("/Network/Proxy/User"));
		proxy.setPassword(Config::getValue<QString>("/Network/Proxy/Password"));
		QNetworkProxy::setApplicationProxy(proxy);
	}

	static void save()
	{
		QNetworkProxy proxy=QNetworkProxy::applicationProxy();
		Config::setValue("/Network/Proxy/Type",proxy.type());
		Config::setValue("/Network/Proxy/HostName",proxy.hostName());
		Config::setValue("/Network/Proxy/Port",proxy.port());
		Config::setValue("/Network/Proxy/User",proxy.user());
		Config::setValue("/Network/Proxy/Password",proxy.password());
	}
};

class ConfigDialog:public QDialog
{
public:
	explicit ConfigDialog(QWidget *parent=0,int index=0);

private:
	QTabWidget *tab;
	QWidget *widget[9];

	//Playing
	QGroupBox *box[7];
	QCheckBox *load[4];
	QCheckBox *fitted[2];
	QLineEdit *factor;
	QCheckBox *bold;
	QComboBox *dmfont;
	QComboBox *effect;
	QLineEdit *play[2];

	//Interface
	QGroupBox *ui[8];
	QComboBox *font;
	QComboBox *reop;
	QCheckBox *vers;
	QCheckBox *sens;
	QCheckBox *less;
	QComboBox *loca;
	QComboBox *stay;
	QLineEdit *jump;
	QLineEdit *size;
	QLineEdit *back;
	QPushButton *open;

	//Performance
	QGroupBox *opt[2];
	QComboBox *render;
	QComboBox *decode;
	QLabel *retext;
	QLabel *detext;
	QList<QLabel *> relogo;
	QList<QLabel *> delogo;

	//Shiled
	QLineEdit *edit;
	QCheckBox *check[8];
	QComboBox *type;
	QListView *regexp;
	QListView *sender;
	QStringListModel *rm;
	QStringListModel *sm;
	QAction *action[4];
	QPushButton *button[2];
	QSlider *same;
	QLineEdit *limit;
	QGroupBox *label[2];

	//Network
	QLineEdit *sheet[3];
	QPushButton *click;
	QLabel *info;
	QGroupBox *login;
	QLabel *text;
	QPushButton *clear;
	QGroupBox *cache;
	QComboBox *arg;
	QLineEdit *input[4];
	QGroupBox *proxy;

	//Shortcut
	QTreeWidget *hotkey;

	//Plugin
	QTreeWidget *plugin;

	//Thanks
	QTextEdit *thanks;

	//License
	QTextEdit *license;

	QNetworkAccessManager *manager;

	QHash<QString,QVariant> restart;

	QHash<QString,QVariant> getRestart()
	{
		QStringList path;
		path<<"/Performance"<<
			  "/Interface/Font"<<
			  "/Interface/Frameless"<<
			  "/Interface/Single"<<
			  "/Interface/Version"<<
			  "/Interface/Locale"<<
			  "/Plugin";
		QHash<QString,QVariant> data;
		for(QString iter:path){
			data[iter]=Config::getValue<QVariant>(iter);
		}
		return data;
	}

	QHash<QString,QVariant> reparse;

	QHash<QString,QVariant> getReparse()
	{
		QHash<QString,QVariant> data;
		int g=0;
		for(int i=0;i<8;++i){
			g=(g<<1)+Shield::shieldG[i];
		}
		data["/Shield/Group"]=g;
		data["/Shield/Regexp"]=rm->stringList();
		data["/Shield/Sender"]=sm->stringList();
		data["/Shield/Limit"]=Config::getValue("/Shield/Limit",5);
		return data;
	}

	void fillPicture(QLabel *label,QString url,QString error,QSize limit)
	{
		QNetworkReply *reply=manager->get(QNetworkRequest(url));
		QObject::connect(reply,&QNetworkReply::finished,label,[=](){
			if (reply->error()==QNetworkReply::NoError){
				QPixmap pixmap;
				pixmap.loadFromData(reply->readAll());
				if(!pixmap.isNull()){
					label->setPixmap(pixmap.scaled(limit,Qt::KeepAspectRatio,Qt::SmoothTransformation));
				}
				else{
					label->setText(error);
				}
			}
			reply->deleteLater();
		});
	}

	QString getLogo(QString name)
	{
		static QHash<QString,QString> l;
		if(l.isEmpty()){
			QFile file(":/Text/DATA");
			file.open(QIODevice::ReadOnly|QIODevice::Text);
			QJsonObject data=QJsonDocument::fromJson(file.readAll()).object()["Logo"].toObject();
			for(auto iter=data.begin();iter!=data.end();++iter){
				l[iter.key()]=iter.value().toString();
			}
		}
		return l[name];
	}
};

class List:public QListView
{
public:
	List(QWidget *parent=0):
		QListView(parent)
	{
	}

	void currentChanged(const QModelIndex &c,
						const QModelIndex &p)
	{
		QListView::currentChanged(c,p);
		selectionModel()->setCurrentIndex(QModelIndex(),QItemSelectionModel::NoUpdate);
	}
};

ConfigDialog::ConfigDialog(QWidget *parent,int index):
	QDialog(parent)
{
	setWindowTitle(Config::tr("Config"));
	auto outer=new QGridLayout(this);
	tab=new QTabWidget(this);
	connect(tab,&QTabWidget::currentChanged,[this](int index){
		tab->widget(index)->setFocus();
	});
	outer->addWidget(tab);
	manager=new QNetworkAccessManager(this);
	Config::setManager(manager);
	//Playing
	{
		widget[0]=new QWidget(this);
		auto list=new QVBoxLayout(widget[0]);
		auto c=new QGridLayout;
		load[0]=new QCheckBox(Config::tr("clear when reloading"),widget[0]);
		load[0]->setChecked(Config::getValue("/Playing/Clear",true));
		connect(load[0],&QCheckBox::stateChanged,[this](int state){
			Config::setValue("/Playing/Clear",state==Qt::Checked);
		});
		c->addWidget(load[0],0,0);
		load[1]=new QCheckBox(Config::tr("auto delay after loaded"),widget[0]);
		load[1]->setChecked(Config::getValue("/Playing/Delay",false));
		connect(load[1],&QCheckBox::stateChanged,[this](int state){
			Config::setValue("/Playing/Delay",state==Qt::Checked);
		});
		c->addWidget(load[1],0,1);
		load[2]=new QCheckBox(Config::tr("load local subtitles"),widget[0]);
		load[2]->setChecked(Config::getValue("/Playing/Subtitle",true));
		connect(load[2],&QCheckBox::stateChanged,[this](int state){
			Config::setValue("/Playing/Subtitle",state==Qt::Checked);
		});
		c->addWidget(load[2],1,0);
		load[3]=new QCheckBox(Config::tr("auto play after loaded"),widget[0]);
		load[3]->setChecked(Config::getValue("/Playing/Immediate",false));
		connect(load[3],&QCheckBox::stateChanged,[this](int state){
			Config::setValue("/Playing/Immediate",state==Qt::Checked);
		});
		c->addWidget(load[3],1,1);
		box[0]=new QGroupBox(Config::tr("loading"),widget[0]);
		box[0]->setLayout(c);
		list->addWidget(box[0]);

		auto s=new QHBoxLayout;
		play[0]=new QLineEdit(widget[0]);
		play[0]->setText(Config::getValue("/Danmaku/Speed",QString("125+%{width}/5")));
		connect(play[0],&QLineEdit::editingFinished,[=](){
			Config::setValue("/Danmaku/Speed",play[0]->text());
		});
		s->addWidget(play[0]);
		box[1]=new QGroupBox(Config::tr("slide speed"),widget[0]);
		box[1]->setToolTip(Config::tr("%{width} means the width of an danmaku"));
		box[1]->setLayout(s);
		list->addWidget(box[1]);

		auto l=new QHBoxLayout;
		play[1]=new QLineEdit(widget[0]);
		play[1]->setText(Config::getValue("/Danmaku/Life",QString("5")));
		connect(play[1],&QLineEdit::editingFinished,[=](){
			Config::setValue("/Danmaku/Life",play[1]->text());
		});
		l->addWidget(play[1]);
		box[2]=new QGroupBox(Config::tr("life time"),widget[0]);
		box[2]->setToolTip(Config::tr("%{width} means the width of an danmaku"));
		box[2]->setLayout(l);
		list->addWidget(box[2]);

		auto e=new QHBoxLayout;
		int state=Config::getValue("/Danmaku/Scale/Fitted",0x1);
		fitted[0]=new QCheckBox(Config::tr("ordinary"),widget[0]);
		fitted[0]->setChecked((state&0x2)>0);
		fitted[1]=new QCheckBox(Config::tr("advanced"),widget[0]);
		fitted[1]->setChecked((state&0x1)>0);
		auto slot=[=](){
			int n=fitted[0]->checkState()==Qt::Checked;
			int a=fitted[1]->checkState()==Qt::Checked;
			Config::setValue("/Danmaku/Scale/Fitted",(n<<1)+a);
		};
		connect(fitted[0],&QCheckBox::stateChanged,slot);
		connect(fitted[1],&QCheckBox::stateChanged,slot);
		e->addWidget(fitted[0],1);
		e->addWidget(fitted[1],1);
		box[3]=new QGroupBox(Config::tr("scale to fitted"),widget[0]);
		box[3]->setLayout(e);

		auto a=new QHBoxLayout;
		factor=new QLineEdit(widget[0]);
		factor->setText(QString::number(Config::getValue("/Danmaku/Scale/Factor",1.0),'f',2));
		connect(factor,&QLineEdit::editingFinished,[=](){
			Config::setValue("/Danmaku/Scale/Factor",factor->text().toDouble());
		});
		a->addWidget(factor);
		box[4]=new QGroupBox(Config::tr("scale by factor"),widget[0]);
		box[4]->setLayout(a);

		auto o=new QHBoxLayout;
		o->addWidget(box[3],1);
		o->addWidget(box[4],1);
		list->addLayout(o);

		auto g=new QHBoxLayout;
		int ef=Config::getValue("/Danmaku/Effect",5);
		bold=new QCheckBox(Config::tr("Bold"),widget[0]);
		bold->setChecked(ef&1);
		connect(bold,&QCheckBox::stateChanged,[=](int s){
			Config::setValue("/Danmaku/Effect",(effect->currentIndex()<<1)|(int)(s==Qt::Checked));
		});
		g->addWidget(bold);
		effect=new QComboBox(widget[0]);
		effect->addItem(Config::tr("Stroke"));
		effect->addItem(Config::tr("Projection"));
		effect->addItem(Config::tr("Shadow"));
		effect->setCurrentIndex(ef>>1);
		connect<void (QComboBox::*)(int)>(effect,&QComboBox::currentIndexChanged,[=](int i){
			Config::setValue("/Danmaku/Effect",(i<<1)|(int)(bold->checkState()==Qt::Checked));
		});
		g->addWidget(effect);
		box[5]=new QGroupBox(Config::tr("style"),widget[0]);
		box[5]->setLayout(g);

		auto f=new QHBoxLayout;
		dmfont=new QComboBox(widget[0]);
		dmfont->addItems(QFontDatabase().families());
		dmfont->setCurrentText(Config::getValue("/Danmaku/Font",QFont().family()));
		connect(dmfont,&QComboBox::currentTextChanged,[this](QString _font){
			Config::setValue("/Danmaku/Font",_font);
		});
		f->addWidget(dmfont);
		box[6]=new QGroupBox(Config::tr("font"),widget[0]);
		box[6]->setLayout(f);

		auto v=new QHBoxLayout;
		v->addWidget(box[5],1);
		v->addWidget(box[6],1);
		list->addLayout(v);

		list->addStretch(10);
		tab->addTab(widget[0],Config::tr("Playing"));
	}
	//Interface
	{
		widget[1]=new QWidget(this);
		auto list=new QVBoxLayout(widget[1]);

		auto s=new QHBoxLayout;
		size=new QLineEdit(widget[1]);
		size->setText(Config::getValue("/Interface/Size",QString("720,405")).trimmed());
		connect(size,&QLineEdit::editingFinished,[=](){
			QRegularExpression r("\\D");
			r.setPatternOptions(QRegularExpression::UseUnicodePropertiesOption);
			QString s=size->text().trimmed();
			s.replace(r,",");
			size->setText(s);
			Config::setValue("/Interface/Size",s+" ");
		});
		s->addWidget(size);
		ui[0]=new QGroupBox(Config::tr("initialize size"),widget[1]);
		ui[0]->setLayout(s);

		auto j=new QHBoxLayout;
		jump=new QLineEdit(widget[0]);
		jump->setText(QString::number(Config::getValue("/Interface/Interval",10),'f',2));
		connect(jump,&QLineEdit::editingFinished,[=](){
			Config::setValue("/Interface/Interval",jump->text().toDouble());
		});
		j->addWidget(jump);
		ui[1]=new QGroupBox(Config::tr("skip time"),widget[1]);
		ui[1]->setLayout(j);

		auto q=new QHBoxLayout;
		q->addWidget(ui[0]);
		q->addWidget(ui[1]);
		list->addLayout(q);

		auto t=new QGridLayout;
		less=new QCheckBox(Config::tr("frameless"),widget[1]);
		less->setChecked(Config::getValue("/Interface/Frameless",false));
		connect(less,&QCheckBox::stateChanged,[=](int state){
			Config::setValue("/Interface/Frameless",state==Qt::Checked);
		});
		t->addWidget(less,0,1);
		vers=new QCheckBox(Config::tr("version information"),widget[1]);
		vers->setChecked(Config::getValue("/Interface/Version",true));
		connect(vers,&QCheckBox::stateChanged,[=](int state){
			Config::setValue("/Interface/Version",state==Qt::Checked);
		});
		t->addWidget(vers,0,0);
		sens=new QCheckBox(Config::tr("sensitive pausing"),widget[1]);
		sens->setChecked(Config::getValue("/Interface/Sensitive",false));
		connect(sens,&QCheckBox::stateChanged,[=](int state){
			Config::setValue("/Interface/Sensitive",state==Qt::Checked);
		});
		t->addWidget(sens,1,0);
		ui[2]=new QGroupBox(Config::tr("window flag"),widget[1]);
		ui[2]->setLayout(t);
		list->addWidget(ui[2]);

		auto f=new QHBoxLayout;
		font=new QComboBox(widget[1]);
		font->addItems(QFontDatabase().families());
		font->setCurrentText(Config::getValue("/Interface/Font/Family",QFont().family()));
		connect(font,&QComboBox::currentTextChanged,[=](QString _font){
			Config::setValue("/Interface/Font/Family",_font);
		});
		f->addWidget(font);
		ui[3]=new QGroupBox(Config::tr("interface font"),widget[1]);
		ui[3]->setLayout(f);

		auto r=new QHBoxLayout;
		reop=new QComboBox(widget[1]);
		reop->addItems({Config::tr("open in new window"),Config::tr("open in current window"),Config::tr("append to playlist")});
		reop->setCurrentIndex(Config::getValue("/Interface/Single",1));
		connect<void (QComboBox::*)(int)>(reop,&QComboBox::currentIndexChanged,[=](int i){
			Config::setValue("/Interface/Single",i);
		});
		r->addWidget(reop);
		ui[4]=new QGroupBox(Config::tr("reopen action"),widget[1]);
		ui[4]->setLayout(r);

		auto v=new QHBoxLayout;
		v->addWidget(ui[3],1);
		v->addWidget(ui[4],1);
		list->addLayout(v);

		auto l=new QHBoxLayout;
		loca=new QComboBox(widget[1]);
		loca->addItem("English",QString());
		for(const QFileInfo &info:QDir("./locale/").entryInfoList(QDir::Dirs|QDir::Files|QDir::NoDotAndDotDot)){
			QLocale l(info.baseName());
			QString name=l.name();
			QString text=l.nativeLanguageName();
			loca->addItem(text,name);
			if (name==Config::getValue("/Interface/Locale",QVariant())){
				loca->setCurrentIndex(loca->count()-1);
			}
		}
		connect<void(QComboBox::*)(int)>(loca,&QComboBox::currentIndexChanged,[this](int i){
			Config::setValue("/Interface/Locale",loca->itemData(i).toString());
		});
		l->addWidget(loca);
		ui[5]=new QGroupBox(Config::tr("locale"));
		ui[5]->setLayout(l);

		auto m=new QHBoxLayout;
		stay=new QComboBox(widget[1]);
		stay->addItems({Config::tr("never"),Config::tr("playing"),Config::tr("always")});
		stay->setCurrentIndex(Config::getValue("/Interface/Top",0));
		connect<void(QComboBox::*)(int)>(stay,&QComboBox::currentIndexChanged,[this](int i){
			Config::setValue("/Interface/Top",i);
			QMetaObject::invokeMethod(lApp->mainWidget(),"setWindowFlags");
		});
		m->addWidget(stay);
		ui[6]=new QGroupBox(Config::tr("stay on top"));
		ui[6]->setLayout(m);

		auto n=new QHBoxLayout;
		n->addWidget(ui[5]);
		n->addWidget(ui[6]);
		list->addLayout(n);

		auto b=new QHBoxLayout;
		back=new QLineEdit(widget[1]);
		back->setText(Config::getValue("/Interface/Background",QString()));
		connect(back,&QLineEdit::textChanged,[=](){
			Render::instance()->setBackground(back->text());
		});
		b->addWidget(back);
		open=new QPushButton(Config::tr("choose"),widget[1]);
		open->setFixedWidth(50*logicalDpiX()/96);
		open->setFocusPolicy(Qt::NoFocus);
		connect(open,&QPushButton::clicked,[this](){
			QString path=back->text().isEmpty()?QDir::currentPath():QFileInfo(back->text()).absolutePath();
			QString file=QFileDialog::getOpenFileName(this,Config::tr("Open File"),path);
			if(!file.isEmpty()){
				back->setText(file.startsWith(QDir::currentPath())?QDir::current().relativeFilePath(file):file);
			}
		});
		b->addWidget(open);
		ui[7]=new QGroupBox(Config::tr("background"),widget[1]);
		ui[7]->setLayout(b);
		list->addWidget(ui[7]);


		list->addStretch(10);
		tab->addTab(widget[1],Config::tr("Interface"));
	}
	//Performance
	{
		widget[2]=new QWidget(this);
		auto out=new QHBoxLayout(widget[2]);
		opt[0]=new QGroupBox(Config::tr("Render"),widget[2]);
		opt[1]=new QGroupBox(Config::tr("Decode"),widget[2]);
		auto r=new QVBoxLayout;
		auto e=new QVBoxLayout;
		opt[0]->setLayout(r);
		opt[1]->setLayout(e);
		render=new QComboBox(widget[2]);
		decode=new QComboBox(widget[2]);

		QStringList relist=Utils::getRenderModules();
		QStringList delist=Utils::getDecodeModules();
		render->addItems(relist);
		decode->addItems(delist);

		if(relist.size()>=2){
			QString r=Config::getValue("/Performance/Render",QString("OpenGL"));
			render->setCurrentText(r);
		}
		else{
			render->setEnabled(false);
		}
		if(delist.size()>=2){
			QString r=Config::getValue("/Performance/Decode",QString("VLC"));
			decode->setCurrentText(r);
		}
		else{
			decode->setEnabled(false);
		}

		auto updateLogo=[this](QVBoxLayout *layout,QList<QLabel *> &pool,QStringList urls){
			qDeleteAll(pool);
			pool.clear();
			for(QString url:urls){
				QLabel *l=new QLabel(widget[2]);
				l->setFixedHeight(100);
				l->setAlignment(Qt::AlignCenter);
				fillPicture(l,url,QString(),QSize(200,80));
				layout->insertWidget(1,l);
				pool.append(l);
			}
		};
		
		connect(render,&QComboBox::currentTextChanged,[=](QString text){
			QString desc;
			if(text=="Raster"){
				desc=Config::tr("software render\n"
						"libswscale for size and chroma transform\n"
						"libqtgui for alpha blending and output\n"
						"high compatibility but a little bit slower");
				updateLogo(r,relogo,QStringList()
						   <<getLogo("Qt")
						   <<getLogo("FFmpeg"));
			}
			if(text=="OpenGL"){
				desc=Config::tr("opengl es2 render\n"
						"texture unit for size transform\n"
						"glsl code for chroma transform\n"
						"only accept YUV420 but significantly faster");
				updateLogo(r,relogo,QStringList()
						   <<""
						   <<getLogo("OpenGL"));
			}
			if(text=="Detach"){
				desc=Config::tr("detach window render\n"
						"transparent opengl window on top\n"
						"video frames won't be displayed\n"
						"for danmaku only playback");
				updateLogo(r,relogo,QStringList()
						   <<""
						   <<"");
			}
			retext->setText(desc);
			if(relist.size()>=2){
				Config::setValue("/Performance/Render",text);
			}
		});
		connect(decode,&QComboBox::currentTextChanged,[=](QString text){
			QString desc;
			if(text=="VLC"){
				desc=Config::tr("libvlc backend\n"
						"all platform supported\n"
						"no additional codecs required");
				updateLogo(e,delogo,QStringList()
						   <<""
						   <<getLogo("VLC"));
			}
			if(text=="QMM"){
				desc=Config::tr("libqtmultimedia backend\n"
						"support directshow on windows\n"
						"k-lite/win7codecs recommended");
				updateLogo(e,delogo,QStringList()
						   <<getLogo("DirectX")
						   <<getLogo("Qt"));
			}
			if(text=="NIL"){
				desc=Config::tr("dummy backend\n"
						"no need for actual media file\n"
						"for danmaku only playback");
				updateLogo(e,delogo,QStringList()
						   <<""
						   <<"");
			}
			detext->setText(desc);
			if(delist.size()>=2){
				Config::setValue("/Performance/Decode",text);
			}
		});

		r->addWidget(render);
		e->addWidget(decode);

		r->addSpacing(10);
		e->addSpacing(10);

		retext=new QLabel(widget[2]);
		detext=new QLabel(widget[2]);
		retext->setWordWrap(true);
		detext->setWordWrap(true);
		retext->setAlignment(Qt::AlignLeft|Qt::AlignTop);
		detext->setAlignment(Qt::AlignLeft|Qt::AlignTop);
		r->addWidget(retext);
		e->addWidget(detext);
		out->addWidget(opt[0],5);
		out->addWidget(opt[1],5);

		render->currentTextChanged(render->currentText());
		decode->currentTextChanged(decode->currentText());

		tab->addTab(widget[2],Config::tr("Performance"));
	}
	//Shield
	{
		widget[3]=new QWidget(this);
		QStringList list;
		list<<Config::tr("Top")<<Config::tr("Bottom")<<Config::tr("Slide")<<Config::tr("Reverse")<<Config::tr("Guest")<<Config::tr("Advanced")<<Config::tr("Color")<<Config::tr("Whole");
		auto grid=new QGridLayout(widget[3]);

		auto g=new QHBoxLayout;
		for(int i=0;i<8;++i){
			check[i]=new QCheckBox(list[i],widget[3]);
			check[i]->setFixedHeight(40);
			check[i]->setChecked(Shield::shieldG[i]);
			connect(check[i],&QCheckBox::stateChanged,[=](int state){
				Shield::shieldG[i]=state==Qt::Checked;
			});
			g->addWidget(check[i]);
		}
		grid->addLayout(g,0,0,1,4);

		type=new QComboBox(widget[3]);
		type->addItem(Config::tr("Text"));
		type->addItem(Config::tr("User"));
		edit=new QLineEdit(widget[3]);
		regexp=new List(widget[3]);
		sender=new List(widget[3]);
		regexp->setSelectionMode(QListView::ExtendedSelection);
		sender->setSelectionMode(QListView::ExtendedSelection);
		regexp->setModel(rm=new QStringListModel(regexp));
		sender->setModel(sm=new QStringListModel(sender));
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
		action[0]=new QAction(Config::tr("Add"),widget[3]);
		action[1]=new QAction(Config::tr("Del"),widget[3]);
		action[2]=new QAction(Config::tr("Import"),widget[3]);
		action[3]=new QAction(Config::tr("Export"),widget[3]);
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
			else{
				edit->setFocus();
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
			QFile file(QFileDialog::getOpenFileName(this,Config::tr("Import File"),QDir::homePath(),Config::tr("Shield files (*.xml *.sol)")));
			if(file.open(QIODevice::ReadOnly|QIODevice::Text)){
				QStringList rl=rm->stringList();
				QStringList sl=sm->stringList();
				{
					QTextStream read(&file);
					read.setCodec("UTF-8");
					QStringList list;
					QString type=QFileInfo(file).suffix();
					QRegularExpression rule;
					rule.setPatternOptions(QRegularExpression::UseUnicodePropertiesOption);
					if (type=="xml"){
						rule.setPattern("[tu]\\=[^\\s<]*");
						QRegularExpressionMatchIterator iter=rule.globalMatch(read.readAll());
						while(iter.hasNext()){
							list.append(Utils::decodeXml(iter.next().captured()));
						}
					}
					if (type=="sol"){
						rule.setPattern("[tu]\\=\\S*");
						QRegularExpressionMatchIterator iter=rule.globalMatch(read.readAll().replace('\x3', ' '));
						while(iter.hasNext()){
							list.append(iter.next().captured());
						}
					}
					for (QString item:list){
						(item[0]=='t'?rl:sl).append(item.mid(2));
					}
				}
				rl.removeDuplicates();
				sl.removeDuplicates();
				rm->setStringList(rl);
				sm->setStringList(sl);
				regexp->setCurrentIndex(QModelIndex());
				sender->setCurrentIndex(QModelIndex());
			}
			file.close();
		});
		connect(action[3],&QAction::triggered,[this](){
			QString path=QFileDialog::getSaveFileName(this,Config::tr("Export File"),QDir::homePath()+"/shield.bililocal.xml",Config::tr("Shield files (*.xml *.sol)"));
			if(!path.isEmpty()){
				QFile file(path);
				if(!path.endsWith(".sol")){
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
				else if(file.open(QIODevice::ReadOnly)){
					QByteArray data=file.readAll();
					file.close();
					QString back=path+".bak";
					QFile::remove(back);
					QFile::rename(path,back);
					file.open(QIODevice::WriteOnly);
					QStringList list;
					for(const QString &iter:rm->stringList()){
						list.append("t="+iter);
					}
					for(const QString &iter:sm->stringList()){
						list.append("u="+iter);
					}
					auto encode=[](quint64 n){
						QByteArray data;
						while(n>=128){
							data.append((n&0x127)|128);
							n=n>>7;
						}
						data.append(n);
						return data;
					};
					QByteArray buff;
					buff+="\x09list\x09"+encode(list.size()*2+1)+"\x01";
					for(const QString &iter:list){
						QByteArray text=iter.toUtf8();
						buff+="\x09\x05\x01\x06"+encode(text.length()*2+1)+text+"\x03";
					}
					buff+=QByteArray("\x01\x00",2);
					int pos=data.indexOf("\x09list\x09");
					int len=data.indexOf('\x00',pos)-pos+1;
					data.replace(pos,len,buff);
					int all=data.length()-6;
					QByteArray size;
					for(int i=0;i<4;++i){
						size.prepend(all&0xFF);
						all=all>>8;
					}
					data.replace(2,4,size);
					file.write(data);
				}
				else{
					QMessageBox::warning(this,Config::tr("Warning"),Config::tr("Please select an existing sol file."));
				}
			}
		});
		for(QAction *action:action){
			widget[3]->addAction(action);
		}
		button[0]=new QPushButton(Config::tr("Add"),widget[3]);
		button[1]=new QPushButton(Config::tr("Del"),widget[3]);
		button[0]->setFixedWidth(60*logicalDpiX()/96);
		button[1]->setFixedWidth(60*logicalDpiX()/96);
		button[0]->setFocusPolicy(Qt::NoFocus);
		button[1]->setFocusPolicy(Qt::NoFocus);
		connect(button[0],&QPushButton::clicked,action[0],&QAction::trigger);
		connect(button[1],&QPushButton::clicked,action[1],&QAction::trigger);
		regexp->addActions(widget[3]->actions());
		sender->addActions(widget[3]->actions());
		regexp->setContextMenuPolicy(Qt::ActionsContextMenu);
		sender->setContextMenuPolicy(Qt::ActionsContextMenu);

		grid->addWidget(type,1,0);
		grid->addWidget(edit,1,1);
		grid->addWidget(button[0],1,2);
		grid->addWidget(button[1],1,3);
		grid->addWidget(regexp,2,0,1,2);
		grid->addWidget(sender,2,2,1,2);

		same=new QSlider(Qt::Horizontal,widget[3]);
		same->setRange(0,40);
		same->setValue(Config::getValue("/Shield/Limit",5));
		connect(same,&QSlider::valueChanged,[=](int value){
			Config::setValue("/Shield/Limit",value);
			QPoint p;
			p.setX(QCursor::pos().x());
			p.setY(same->mapToGlobal(same->rect().center()).y());
			QToolTip::showText(p,QString::number(value));
		});
		auto a=new QHBoxLayout;
		a->addWidget(same);
		label[0]=new QGroupBox(Config::tr("limit of the same"),widget[3]);
		label[0]->setToolTip(Config::tr("0 means disabled"));
		label[0]->setLayout(a);
		grid->addWidget(label[0],3,0,1,2);

		limit=new QLineEdit(widget[3]);
		limit->setText(QString::number(Config::getValue("/Shield/Density",100)));
		connect(limit,&QLineEdit::editingFinished,[=](){
			Config::setValue("/Shield/Density",limit->text().toInt());
		});
		auto m=new QHBoxLayout;
		m->addWidget(limit);
		label[1]=new QGroupBox(Config::tr("limit of density"),widget[3]);
		label[1]->setToolTip(Config::tr("0 means disabled"));
		label[1]->setLayout(m);
		grid->addWidget(label[1],3,2,1,2);

		tab->addTab(widget[3],Config::tr("Shield"));
	}
	//Network
	{
		widget[4]=new QWidget(this);
		auto list=new QVBoxLayout(widget[4]);

		auto l=new QGridLayout;
		l->setColumnStretch(0,1);
		l->setColumnStretch(1,1);
		l->setColumnStretch(2,1);
		l->setColumnStretch(3,1);
		sheet[0]=new QLineEdit(widget[4]);
		sheet[0]->setPlaceholderText(Config::tr("Username"));
		sheet[1]=new QLineEdit(widget[4]);
		sheet[1]->setPlaceholderText(Config::tr("Password"));
		sheet[1]->setEchoMode(QLineEdit::Password);
		sheet[2]=new QLineEdit(widget[4]);
		sheet[2]->setPlaceholderText(Config::tr("Identifier"));
		auto checkout=[this](){
			bool flag=true;
			for(QLineEdit *iter:sheet){
				if(iter->text().isEmpty()){
					flag=false;
					break;
				}
			}
			if(flag){
				click->click();
			}
		};
		for(QLineEdit *iter:sheet){
			connect(iter,&QLineEdit::editingFinished,checkout);
		}
		connect(sheet[2],&QLineEdit::textEdited,[this](QString text){
			sheet[2]->setText(text.toUpper());
		});
		l->addWidget(sheet[0],0,0);
		l->addWidget(sheet[1],0,1);
		l->addWidget(sheet[2],0,2);
		info=new QLabel(widget[4]);
		info->setAlignment(Qt::AlignCenter);
		info->setText(Config::tr("waiting"));
		l->addWidget(info,0,3);
		auto loadValid=[=](){
			QString url("https://secure.%1/captcha?r=%2");
			url=url.arg(Utils::customUrl(Utils::Bilibili));
			url=url.arg(qrand()/(double)RAND_MAX);
			fillPicture(info,url,Config::tr("error"),QSize(200,25));
		};
		auto setLogged=[this,loadValid](bool logged){
			if(logged){
				info->setText(Config::tr("logged"));
				sheet[1]->clear();
				sheet[2]->clear();
				click->setText(Config::tr("logout"));
			}
			else{
				loadValid();
				click->setText(Config::tr("login"));
			}
			for(QLineEdit *iter:sheet){
				iter->setEnabled(!logged);
			}
		};
		auto sendLogin=[this,setLogged](){
			click->setEnabled(false);
			QUrlQuery query;
			query.addQueryItem("act","login");
			query.addQueryItem("userid",sheet[0]->text());
			query.addQueryItem("pwd",sheet[1]->text());
			query.addQueryItem("vdcode",sheet[2]->text());
			query.addQueryItem("keeptime","2592000");
			QByteArray data=query.query().toUtf8();
			QString url("https://secure.%1/login");
			url=url.arg(Utils::customUrl(Utils::Bilibili));
			QNetworkRequest request(url);
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
						QMessageBox::warning(this,Config::tr("Warning"),page.mid(sta,page.indexOf("\"",sta)-sta));
					}
				}
				click->setEnabled(true);
				setLogged(flag);
				reply->deleteLater();
			});
		};
		auto setLogout=[=](){
			click->setEnabled(false);
			QString url="https://secure.%1/login?act=exit";
			url=url.arg(Utils::customUrl(Utils::Bilibili));
			QNetworkReply *reply=manager->get(QNetworkRequest(url));
			connect(reply,&QNetworkReply::finished,[=](){
				click->setEnabled(true);
				setLogged(reply->error()!=QNetworkReply::NoError);
			});
		};
		click=new QPushButton(Config::tr("login"),widget[1]);
		click->setFocusPolicy(Qt::NoFocus);
		connect(click,&QPushButton::clicked,[=](){
			if(click->text()==Config::tr("login")){
				for(QLineEdit *iter:sheet){
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
		QString url("http://member.%1/main.html");
		url=url.arg(Utils::customUrl(Utils::Bilibili));
		QNetworkReply *reply=manager->get(QNetworkRequest(url));
		connect(reply,&QNetworkReply::finished,[=](){
			bool flag=false;
			if(reply->error()==QNetworkReply::NoError){
				QString user=QRegularExpression("(?<=\\>).*(?=\\<\\/h3\\>)").match(reply->readAll()).captured();
				if(!user.isEmpty()){
					sheet[0]->setText(user);
					flag=true;
				}
			}
			setLogged(flag);
		});
		l->addWidget(click);
		login=new QGroupBox(Config::tr("login"),widget[4]);
		login->setLayout(l);
		list->addWidget(login);

		auto c=new QHBoxLayout;
		text=new QLabel(widget[4]);
		auto m=[=](){
			QString s=Config::tr("current size is %.2fMB");
			s.sprintf(s.toUtf8(),DCache::data->cacheSize()/(1024.0*1024));
			text->setText(s);
		};
		m();
		c->addWidget(text,2);
		c->addStretch(1);
		clear=new QPushButton(Config::tr("clear"),widget[4]);
		clear->setFocusPolicy(Qt::NoFocus);
		connect(clear,&QPushButton::clicked,[m](){
			DCache::data->clear();
			m();
		});
		c->addWidget(clear,1);
		cache=new QGroupBox(Config::tr("cache"),widget[4]);
		cache->setLayout(c);
		list->addWidget(cache);

		auto p=new QGridLayout;
		arg=new QComboBox(widget[4]);
		arg->addItems({Config::tr("No Proxy"),Config::tr("Http Proxy"),Config::tr("Socks5 Proxy")});
		connect<void (QComboBox::*)(int)>(arg,&QComboBox::currentIndexChanged,[=](int index){
			QNetworkProxy proxy=QNetworkProxy::applicationProxy();
			switch(index){
			case 0:
				proxy.setType(QNetworkProxy::NoProxy);
				break;
			case 1:
				proxy.setType(QNetworkProxy::HttpProxy);
				break;
			case 2:
				proxy.setType(QNetworkProxy::Socks5Proxy);
				break;
			}
			QNetworkProxy::setApplicationProxy(proxy);
			for(QLineEdit *iter:input){
				iter->setEnabled(index!=0);
			}
		});
		p->addWidget(arg,0,0);
		input[0]=new QLineEdit(widget[4]);
		input[0]->setText(QNetworkProxy::applicationProxy().hostName());
		input[0]->setPlaceholderText(Config::tr("HostName"));
		connect(input[0],&QLineEdit::editingFinished,[this](){
			QNetworkProxy proxy=QNetworkProxy::applicationProxy();
			proxy.setHostName(input[0]->text());
			QNetworkProxy::setApplicationProxy(proxy);
		});
		p->addWidget(input[0],1,0);
		input[1]=new QLineEdit(widget[4]);
		input[1]->setPlaceholderText(Config::tr("Port"));
		int port=QNetworkProxy::applicationProxy().port();
		input[1]->setText(port?QString::number(port):QString());
		connect(input[1],&QLineEdit::editingFinished,[this](){
			QNetworkProxy proxy=QNetworkProxy::applicationProxy();
			proxy.setPort(input[1]->text().toInt());
			if(proxy.port()==0){
				input[1]->clear();
			}
			QNetworkProxy::setApplicationProxy(proxy);
		});
		p->addWidget(input[1],1,1);
		input[2]=new QLineEdit(widget[4]);
		input[2]->setPlaceholderText(Config::tr("User"));
		input[2]->setText(QNetworkProxy::applicationProxy().user());
		connect(input[2],&QLineEdit::editingFinished,[this](){
			QNetworkProxy proxy=QNetworkProxy::applicationProxy();
			proxy.setUser(input[2]->text());
			QNetworkProxy::setApplicationProxy(proxy);
		});
		p->addWidget(input[2],1,2);
		input[3]=new QLineEdit(widget[4]);
		input[3]->setPlaceholderText(Config::tr("Password"));
		input[3]->setText(QNetworkProxy::applicationProxy().password());
		connect(input[3],&QLineEdit::editingFinished,[this](){
			QNetworkProxy proxy=QNetworkProxy::applicationProxy();
			proxy.setPassword(input[3]->text());
			QNetworkProxy::setApplicationProxy(proxy);
		});
		p->addWidget(input[3],1,3);
		for(QLineEdit *iter:input){
			iter->setEnabled(false);
		}
		switch(QNetworkProxy::applicationProxy().type()){
		case QNetworkProxy::HttpProxy:
			arg->setCurrentIndex(1);
			break;
		case QNetworkProxy::Socks5Proxy:
			arg->setCurrentIndex(2);
			break;
		default:
			arg->setCurrentIndex(0);
			break;
		}
		proxy=new QGroupBox(Config::tr("proxy"),widget[4]);
		proxy->setLayout(p);
		list->addWidget(proxy);

		list->addStretch(10);
		tab->addTab(widget[4],Config::tr("Network"));
	}
	//Plugin
	{
		widget[5]=new QWidget(this);
		auto w=new QGridLayout(widget[5]);
		plugin=new QTreeWidget(widget[5]);
		plugin->setSelectionMode(QAbstractItemView::NoSelection);
		QStringList header;
		header<<Config::tr("Enable")<<Config::tr("Name")<<Config::tr("Version")<<Config::tr("Description")<<Config::tr("Author")<<"";
		plugin->setHeaderLabels(header);
		double s=logicalDpiX()/72.0;
		plugin->header()->setStretchLastSection(false);
		plugin->header()->setSectionResizeMode(3,QHeaderView::Stretch);
		plugin->setColumnWidth(0,50);
		plugin->setColumnWidth(1,55*s);
		plugin->setColumnWidth(2,35*s);
		plugin->setColumnWidth(4,50*s);
		plugin->setColumnWidth(5,30*s);
		plugin->setIndentation(15);
		w->addWidget(plugin);
		for(Plugin &iter:Plugin::plugins){
			QStringList content;
			content+="";
			content+=iter.string("Name");
			content+=iter.string("Version");
			content+=iter.string("Description");
			content+=iter.string("Author");
			content+=Config::tr("options");
			QTreeWidgetItem *row=new QTreeWidgetItem(plugin,content);
			row->setCheckState(0,Config::getValue("/Plugin/"+iter.string("Name"),true)?Qt::Checked:Qt::Unchecked);
			row->setData(0,Qt::UserRole,(quintptr)&iter);
			QFont f;
			f.setUnderline(true);
			row->setData(5,Qt::FontRole,f);
			row->setData(5,Qt::ForegroundRole,qApp->palette().color(QPalette::Highlight));
			row->setTextAlignment(5,Qt::AlignCenter);
			row->setSizeHint(0,QSize(60,30*logicalDpiY()/72));
		}
		connect(plugin,&QTreeWidget::itemChanged,[=](QTreeWidgetItem *item){
			Plugin *p=(Plugin *)item->data(0,Qt::UserRole).value<quintptr>();
			Config::setValue("/Plugin/"+p->string("Name"),item->checkState(0)==Qt::Checked);
		});
		connect(plugin,&QTreeWidget::itemClicked,[=](QTreeWidgetItem *item,int column){
			if(column==5){
				((Plugin *)item->data(0,Qt::UserRole).value<quintptr>())->config(this);
			}
		});
		connect(plugin,&QTreeWidget::currentItemChanged,[this](){
			plugin->setCurrentItem(NULL);
		});
		tab->addTab(widget[5],Config::tr("Plugin"));
	}
	//Shortcut
	{
		widget[6]=new QWidget(this);
		auto w=new QGridLayout(widget[6]);
		hotkey=new QTreeWidget(widget[6]);
		hotkey->setSelectionMode(QAbstractItemView::NoSelection);
		hotkey->setColumnCount(2);
		hotkey->header()->hide();
		hotkey->header()->setStretchLastSection(false);
		hotkey->header()->setSectionResizeMode(0,QHeaderView::Stretch);
		hotkey->setColumnWidth(1,1.2*logicalDpiX());
		hotkey->setEditTriggers(QAbstractItemView::NoEditTriggers);
		for(QAction *iter:lApp->mainWidget()->findChildren<QAction *>(QRegularExpression(".{4}"))){
			QTreeWidgetItem *item=new QTreeWidgetItem;
			item->setData(0,Qt::DisplayRole,iter->text());
			item->setData(1,Qt::DisplayRole,iter->shortcut().toString());
			item->setData(1,Qt::UserRole,iter->objectName());
			item->setFlags(Qt::ItemIsEditable|Qt::ItemIsEnabled);
			item->setSizeHint(0,QSize(60,25*logicalDpiY()/72));
			hotkey->addTopLevelItem(item);
		}
		w->addWidget(hotkey);
		connect(hotkey,&QTreeWidget::currentItemChanged,[this](){
			hotkey->setCurrentItem(NULL);
		});
		connect(hotkey,&QTreeWidget::itemClicked,[=](QTreeWidgetItem *item,int column){
			if(column==1){
				hotkey->editItem(item,1);
			}
		});
		connect(hotkey,&QTreeWidget::itemChanged,[=](QTreeWidgetItem *item,int column){
			if(column==1){
				QVariant v=item->data(1,Qt::UserRole);
				if(v.isValid()){
					QAction *a=lApp->mainWidget()->findChild<QAction *>(v.toString());
					QString ns=item->text(1);
					a->setShortcut(ns);
					Config::setValue("/Shortcut/"+a->objectName(),ns);
				}
			}
		});
		tab->addTab(widget[6],Config::tr("Shortcut"));
	}
	//Thanks
	{
		widget[7]=new QWidget(this);
		auto w=new QGridLayout(widget[7]);
		QFile t(":/Text/THANKS");
		t.open(QIODevice::ReadOnly|QIODevice::Text);
		thanks=new QTextEdit(widget[7]);
		thanks->setReadOnly(true);
		thanks->setText(t.readAll());
		w->addWidget(thanks);
		tab->addTab(widget[7],Config::tr("Thanks"));
	}
	//License
	{
		widget[8]=new QWidget(this);
		auto w=new QGridLayout(widget[8]);
		QFile l(":/Text/COPYING");
		l.open(QIODevice::ReadOnly|QIODevice::Text);
		license=new QTextEdit(widget[8]);
		license->setReadOnly(true);
		license->setText(l.readAll());
		w->addWidget(license);
		tab->addTab(widget[8],Config::tr("License"));
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
			if((APlayer::instance()->getState()==APlayer::Stop&&
				Danmaku::instance()->rowCount()==0)||
					QMessageBox::warning(this,
										 Config::tr("Warning"),
										 Config::tr("Restart to apply changes?"),
										 QMessageBox::Yes,QMessageBox::No)==QMessageBox::Yes){
				lApp->exit(12450);
			}
		}
	});
	int h=outer->minimumSize().height(),w=1.15*h;
	setMinimumSize(w,h);
	Utils::setCenter(this);

	restart=getRestart();
	reparse=getReparse();
}
}

QJsonObject Config::config;

void Config::exec(QWidget *parent,int index)
{
	static ConfigDialog *executing;
	if(!executing){
		ConfigDialog config(parent,index);
		executing=&config;
		config.exec();
		executing=nullptr;
	}
	else{
		executing->activateWindow();
	}
}

void Config::load()
{
	QFile conf("./Config.txt");
	if(conf.open(QIODevice::ReadOnly|QIODevice::Text)){
		config=QJsonDocument::fromJson(conf.readAll()).object();
		conf.close();
	}
	Cookie::load();
	DCache::load();
	APorxy::load();
}

void Config::save()
{
	Cookie::save();
	APorxy::save();
	QFile conf("./Config.txt");
	conf.open(QIODevice::WriteOnly|QIODevice::Text);
	conf.write(QJsonDocument(config).toJson());
	conf.close();
}

void Config::setManager(QNetworkAccessManager *manager)
{
	manager->setCache(DCache::data);
	DCache::data->setParent(NULL);
	manager->setCookieJar(Cookie::data);
	Cookie::data->setParent(NULL);
}

void     Config::setVariant(QString key,QVariant val)
{
	       setValue(key,val);
}

QVariant Config::getVariant(QString key,QVariant val)
{
	return getValue(key,val);
}