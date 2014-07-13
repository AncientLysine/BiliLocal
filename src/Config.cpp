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
#include "Local.h"
#include "Utils.h"
#include "Shield.h"
#include "Plugin.h"
#include "Danmaku.h"
#include "APlayer.h"

class ConfigPrivate
{
public:
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
	QGroupBox *ui[7];
	QComboBox *font;
	QComboBox *reop;
	QCheckBox *vers;
	QCheckBox *stay;
	QCheckBox *less;
	QCheckBox *skip;
	QCheckBox *sing;
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
			  "/Interface/Background"<<
			  "/Interface/Font"<<
			  "/Interface/Frameless"<<
			  "/Interface/Single"<<
			  "/Interface/Top"<<
			  "/Interface/Version"<<
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
			}
			if(!label->pixmap()){
				label->setText(error);
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

QJsonObject Config::config;

void Config::exec(QWidget *parent,int index)
{
	static bool isExecuting;
	if(!isExecuting){
		isExecuting=1;
		Config config(parent,index);
		config.QDialog::exec();
		isExecuting=0;
	}
}

Config::Config(QWidget *parent,int index):
	QDialog(parent),d_ptr(new ConfigPrivate)
{
	Q_D(Config);
	setWindowTitle(tr("Config"));
	auto outer=new QGridLayout(this);
	d->tab=new QTabWidget(this);
	connect(d->tab,&QTabWidget::currentChanged,[d](int index){
		d->tab->widget(index)->setFocus();
	});
	outer->addWidget(d->tab);
	d->manager=new QNetworkAccessManager(this);
	setManager(d->manager);
	//Playing
	{
		d->widget[0]=new QWidget(this);
		auto list=new QVBoxLayout(d->widget[0]);
		auto c=new QGridLayout;
		d->load[0]=new QCheckBox(tr("clear when reloading"),d->widget[0]);
		d->load[0]->setChecked(Config::getValue("/Playing/Clear",true));
		connect(d->load[0],&QCheckBox::stateChanged,[this](int state){
			Config::setValue("/Playing/Clear",state==Qt::Checked);
		});
		c->addWidget(d->load[0],0,0);
		d->load[1]=new QCheckBox(tr("auto delay after loaded"),d->widget[0]);
		d->load[1]->setChecked(Config::getValue("/Playing/Delay",false));
		connect(d->load[1],&QCheckBox::stateChanged,[this](int state){
			Config::setValue("/Playing/Delay",state==Qt::Checked);
		});
		c->addWidget(d->load[1],0,1);
		d->load[2]=new QCheckBox(tr("load local subtitles"),d->widget[0]);
		d->load[2]->setChecked(Config::getValue("/Playing/Subtitle",true));
		connect(d->load[2],&QCheckBox::stateChanged,[this](int state){
			Config::setValue("/Playing/Subtitle",state==Qt::Checked);
		});
		c->addWidget(d->load[2],1,0);
		d->load[3]=new QCheckBox(tr("auto play after loaded"),d->widget[0]);
		d->load[3]->setChecked(Config::getValue("/Playing/Immediate",false));
		connect(d->load[3],&QCheckBox::stateChanged,[this](int state){
			Config::setValue("/Playing/Immediate",state==Qt::Checked);
		});
		c->addWidget(d->load[3],1,1);
		d->box[0]=new QGroupBox(tr("loading"),d->widget[0]);
		d->box[0]->setLayout(c);
		list->addWidget(d->box[0]);

		auto s=new QHBoxLayout;
		d->play[0]=new QLineEdit(d->widget[0]);
		d->play[0]->setText(Config::getValue("/Danmaku/Speed",QString("125+%{width}/5")));
		connect(d->play[0],&QLineEdit::editingFinished,[d](){
			Config::setValue("/Danmaku/Speed",d->play[0]->text());
		});
		s->addWidget(d->play[0]);
		d->box[1]=new QGroupBox(tr("slide speed"),d->widget[0]);
		d->box[1]->setToolTip(tr("%{width} means the width of an danmaku"));
		d->box[1]->setLayout(s);
		list->addWidget(d->box[1]);

		auto l=new QHBoxLayout;
		d->play[1]=new QLineEdit(d->widget[0]);
		d->play[1]->setText(Config::getValue("/Danmaku/Life",QString("5")));
		connect(d->play[1],&QLineEdit::editingFinished,[d](){
			Config::setValue("/Danmaku/Life",d->play[1]->text());
		});
		l->addWidget(d->play[1]);
		d->box[2]=new QGroupBox(tr("life time"),d->widget[0]);
		d->box[2]->setToolTip(tr("%{width} means the width of an danmaku"));
		d->box[2]->setLayout(l);
		list->addWidget(d->box[2]);

		auto e=new QHBoxLayout;
		int state=Config::getValue("/Danmaku/Scale/Fitted",0x1);
		d->fitted[0]=new QCheckBox(tr("ordinary"),d->widget[0]);
		d->fitted[0]->setChecked((state&0x2)>0);
		d->fitted[1]=new QCheckBox(tr("advanced"),d->widget[0]);
		d->fitted[1]->setChecked((state&0x1)>0);
		auto slot=[d](){
			int n=d->fitted[0]->checkState()==Qt::Checked;
			int a=d->fitted[1]->checkState()==Qt::Checked;
			Config::setValue("/Danmaku/Scale/Fitted",(n<<1)+a);
		};
		connect(d->fitted[0],&QCheckBox::stateChanged,slot);
		connect(d->fitted[1],&QCheckBox::stateChanged,slot);
		e->addWidget(d->fitted[0],1);
		e->addWidget(d->fitted[1],1);
		d->box[3]=new QGroupBox(tr("scale to fitted"),d->widget[0]);
		d->box[3]->setLayout(e);

		auto a=new QHBoxLayout;
		d->factor=new QLineEdit(d->widget[0]);
		d->factor->setText(QString::number(Config::getValue("/Danmaku/Scale/Factor",1.0),'f',2));
		connect(d->factor,&QLineEdit::editingFinished,[d](){
			Config::setValue("/Danmaku/Scale/Factor",d->factor->text().toDouble());
		});
		a->addWidget(d->factor);
		d->box[4]=new QGroupBox(tr("scale by factor"),d->widget[0]);
		d->box[4]->setLayout(a);

		auto o=new QHBoxLayout;
		o->addWidget(d->box[3],1);
		o->addWidget(d->box[4],1);
		list->addLayout(o);

		auto g=new QHBoxLayout;
		int ef=Config::getValue("/Danmaku/Effect",5);
		d->bold=new QCheckBox(tr("Bold"),d->widget[0]);
		d->bold->setChecked(ef&1);
		connect(d->bold,&QCheckBox::stateChanged,[d](int s){
			Config::setValue("/Danmaku/Effect",(d->effect->currentIndex()<<1)|(int)(s==Qt::Checked));
		});
		g->addWidget(d->bold);
		d->effect=new QComboBox(d->widget[0]);
		d->effect->addItem(tr("Stroke"));
		d->effect->addItem(tr("Projection"));
		d->effect->addItem(tr("Shadow"));
		d->effect->setCurrentIndex(ef>>1);
		connect<void (QComboBox::*)(int)>(d->effect,&QComboBox::currentIndexChanged,[d](int i){
			Config::setValue("/Danmaku/Effect",(i<<1)|(int)(d->bold->checkState()==Qt::Checked));
		});
		g->addWidget(d->effect);
		d->box[5]=new QGroupBox(tr("style"),d->widget[0]);
		d->box[5]->setLayout(g);

		auto f=new QHBoxLayout;
		d->dmfont=new QComboBox(d->widget[0]);
		d->dmfont->addItems(QFontDatabase().families());
		d->dmfont->setCurrentText(Config::getValue("/Danmaku/Font",QFont().family()));
		connect(d->dmfont,&QComboBox::currentTextChanged,[this](QString _font){
			Config::setValue("/Danmaku/Font",_font);
		});
		f->addWidget(d->dmfont);
		d->box[6]=new QGroupBox(tr("font"),d->widget[0]);
		d->box[6]->setLayout(f);

		auto v=new QHBoxLayout;
		v->addWidget(d->box[5],1);
		v->addWidget(d->box[6],1);
		list->addLayout(v);

		list->addStretch(10);
		d->tab->addTab(d->widget[0],tr("Playing"));
	}
	//Interface
	{
		d->widget[1]=new QWidget(this);
		auto list=new QVBoxLayout(d->widget[1]);

		auto s=new QHBoxLayout;
		d->size=new QLineEdit(d->widget[1]);
		d->size->setText(Config::getValue("/Interface/Size",QString("960,540")).trimmed());
		connect(d->size,&QLineEdit::editingFinished,[d](){
			QRegularExpression r("\\D");
			r.setPatternOptions(QRegularExpression::UseUnicodePropertiesOption);
			QString s=d->size->text().trimmed();
			s.replace(r,",");
			d->size->setText(s);
			Config::setValue("/Interface/Size",s+" ");
		});
		s->addWidget(d->size);
		d->ui[0]=new QGroupBox(tr("initialize size"),d->widget[1]);
		d->ui[0]->setLayout(s);

		auto j=new QHBoxLayout;
		d->jump=new QLineEdit(d->widget[0]);
		d->jump->setText(QString::number(Config::getValue("/Interface/Interval",10),'f',2));
		connect(d->jump,&QLineEdit::editingFinished,[d](){
			Config::setValue("/Interface/Interval",d->jump->text().toDouble());
		});
		j->addWidget(d->jump);
		d->ui[1]=new QGroupBox(tr("skip time"),d->widget[1]);
		d->ui[1]->setLayout(j);

		auto q=new QHBoxLayout;
		q->addWidget(d->ui[0]);
		q->addWidget(d->ui[1]);
		list->addLayout(q);

		auto t=new QGridLayout;
		d->stay=new QCheckBox(tr("stay on top"),d->widget[1]);
		d->stay->setChecked(Config::getValue("/Interface/Top",false));
		connect(d->stay,&QCheckBox::stateChanged,[d](int state){
			Config::setValue("/Interface/Top",state==Qt::Checked);
		});
		t->addWidget(d->stay,0,0);
		d->less=new QCheckBox(tr("frameless"),d->widget[1]);
		d->less->setChecked(Config::getValue("/Interface/Frameless",false));
		connect(d->less,&QCheckBox::stateChanged,[d](int state){
			Config::setValue("/Interface/Frameless",state==Qt::Checked);
		});
		t->addWidget(d->less,1,0);
		d->vers=new QCheckBox(tr("version information"),d->widget[1]);
		d->vers->setChecked(Config::getValue("/Interface/Version",true));
		connect(d->vers,&QCheckBox::stateChanged,[d](int state){
			Config::setValue("/Interface/Version",state==Qt::Checked);
		});
		t->addWidget(d->vers,0,1);
		d->ui[2]=new QGroupBox(tr("window flag"),d->widget[1]);
		d->ui[2]->setLayout(t);
		list->addWidget(d->ui[2]);

		auto f=new QHBoxLayout;
		d->font=new QComboBox(d->widget[1]);
		d->font->addItems(QFontDatabase().families());
		d->font->setCurrentText(Config::getValue("/Interface/Font/Family",QFont().family()));
		connect(d->font,&QComboBox::currentTextChanged,[d](QString _font){
			Config::setValue("/Interface/Font/Family",_font);
		});
		f->addWidget(d->font);
		d->ui[3]=new QGroupBox(tr("interface font"),d->widget[1]);
		d->ui[3]->setLayout(f);

		auto r=new QHBoxLayout;
		d->reop=new QComboBox(d->widget[1]);
		d->reop->addItems(QStringList()<<tr("open in new window")<<tr("open in current window"));
		d->reop->setCurrentIndex(Config::getValue("/Interface/Single",1));
		connect<void (QComboBox::*)(int)>(d->reop,&QComboBox::currentIndexChanged,[d](int i){
			Config::setValue("/Interface/Single",i);
		});
		r->addWidget(d->reop);
		d->ui[4]=new QGroupBox(tr("reopen action"),d->widget[1]);
		d->ui[4]->setLayout(r);

		auto v=new QHBoxLayout;
		v->addWidget(d->ui[3],1);
		v->addWidget(d->ui[4],1);
		list->addLayout(v);

		auto a=new QHBoxLayout;
		d->skip=new QCheckBox(tr("skip blocked ones"),d->widget[1]);
		d->skip->setChecked(Config::getValue("/Interface/Save/Skip",false));
		connect(d->skip,&QCheckBox::stateChanged,[d](int state){
			Config::setValue("/Interface/Save/Skip",state==Qt::Checked);
		});
		a->addWidget(d->skip);
		d->sing=new QCheckBox(tr("as a single file"),d->widget[1]);
		d->sing->setChecked(Config::getValue("/Interface/Save/Single",true));
		connect(d->sing,&QCheckBox::stateChanged,[d](int state){
			Config::setValue("/Interface/Save/Single",state==Qt::Checked);
		});
		a->addWidget(d->sing);
		d->ui[5]=new QGroupBox(tr("saving option"));
		d->ui[5]->setLayout(a);
		list->addWidget(d->ui[5]);

		auto b=new QHBoxLayout;
		d->back=new QLineEdit(d->widget[1]);
		d->back->setText(Config::getValue("/Interface/Background",QString()));
		connect(d->back,&QLineEdit::textChanged,[d](){
			Config::setValue("/Interface/Background",d->back->text());
		});
		b->addWidget(d->back);
		d->open=new QPushButton(tr("choose"),d->widget[1]);
		d->open->setFixedWidth(50);
		d->open->setFocusPolicy(Qt::NoFocus);
		connect(d->open,&QPushButton::clicked,[d,this](){
			QString path=d->back->text().isEmpty()?QDir::currentPath():QFileInfo(d->back->text()).absolutePath();
			QString file=QFileDialog::getOpenFileName(this,tr("Open File"),path);
			if(!file.isEmpty()){
				d->back->setText(file.startsWith(QDir::currentPath())?QDir::current().relativeFilePath(file):file);
			}
		});
		b->addWidget(d->open);
		d->ui[6]=new QGroupBox(tr("background"),d->widget[1]);
		d->ui[6]->setLayout(b);
		list->addWidget(d->ui[6]);


		list->addStretch(10);
		d->tab->addTab(d->widget[1],tr("Interface"));
	}
	//Performance
	{
		d->widget[2]=new QWidget(this);
		auto out=new QHBoxLayout(d->widget[2]);
		d->opt[0]=new QGroupBox(tr("Render"),d->widget[2]);
		d->opt[1]=new QGroupBox(tr("Decode"),d->widget[2]);
		auto r=new QVBoxLayout;
		auto e=new QVBoxLayout;
		d->opt[0]->setLayout(r);
		d->opt[1]->setLayout(e);
		d->render=new QComboBox(d->widget[2]);
		d->decode=new QComboBox(d->widget[2]);

		QStringList relist=Utils::getRenderModules();
		QStringList delist=Utils::getDecodeModules();
		d->render->addItems(relist);
		d->decode->addItems(delist);

		if(relist.size()>=2){
			QString r=getValue("/Performance/Render",QString("OpenGL"));
			d->render->setCurrentText(r);
		}
		else{
			d->render->setEnabled(false);
		}
		if(delist.size()>=2){
			QString r=getValue("/Performance/Decode",QString("VLC"));
			d->decode->setCurrentText(r);
		}
		else{
			d->decode->setEnabled(false);
		}

		auto updateLogo=[d](QVBoxLayout *layout,QList<QLabel *> &pool,QStringList urls){
			qDeleteAll(pool);
			pool.clear();
			for(QString url:urls){
				QLabel *l=new QLabel(d->widget[2]);
				l->setFixedHeight(100);
				l->setAlignment(Qt::AlignCenter);
				d->fillPicture(l,url,QString(),QSize(200,80));
				layout->insertWidget(1,l);
				pool.append(l);
			}
		};

		connect(d->render,&QComboBox::currentTextChanged,[=](QString text){
			QString desc;
			if(text=="Raster"){
				desc=tr("software render\n"
						"libswscale for size and chroma transform\n"
						"libqtgui for alpha blending and output\n"
						"high compatibility but a little bit slower");
				updateLogo(r,d->relogo,QStringList()
						   <<d->getLogo("Qt")
						   <<d->getLogo("FFmpeg"));
			}
			if(text=="OpenGL"){
				desc=tr("opengl es2 renedr\n"
						"texture unit for size transform\n"
						"glsl code for chroma transform\n"
						"only accept YV12/I420 but significantly faster");
				updateLogo(r,d->relogo,QStringList()
						   <<""
						   <<d->getLogo("OpenGL"));
			}
			d->retext->setText(desc);
			if(relist.size()>=2){
				setValue("/Performance/Render",text);
			}
		});
		connect(d->decode,&QComboBox::currentTextChanged,[=](QString text){
			QString desc;
			if(text=="VLC"){
				desc=tr("libvlc backend\n"
						"all platform supported\n"
						"no additional codecs required");
				updateLogo(e,d->delogo,QStringList()
						   <<""
						   <<d->getLogo("VLC"));
			}
			if(text=="QMM"){
				desc=tr("libqtmultimedia backend\n"
						"support directshow on windows\n"
						"k-lite/win7codecs recommended");
				updateLogo(e,d->delogo,QStringList()
						   <<d->getLogo("DirectX")
						   <<d->getLogo("Qt"));
			}
			d->detext->setText(desc);
			if(delist.size()>=2){
				setValue("/Performance/Decode",text);
			}
		});

		r->addWidget(d->render);
		e->addWidget(d->decode);

		r->addSpacing(10);
		e->addSpacing(10);

		d->retext=new QLabel(d->widget[2]);
		d->detext=new QLabel(d->widget[2]);
		d->retext->setWordWrap(true);
		d->detext->setWordWrap(true);
		d->retext->setAlignment(Qt::AlignLeft|Qt::AlignTop);
		d->detext->setAlignment(Qt::AlignLeft|Qt::AlignTop);
		r->addWidget(d->retext);
		e->addWidget(d->detext);
		out->addWidget(d->opt[0],5);
		out->addWidget(d->opt[1],5);

		d->render->currentTextChanged(d->render->currentText());
		d->decode->currentTextChanged(d->decode->currentText());

		d->tab->addTab(d->widget[2],tr("Performance"));
	}
	//Shield
	{
		d->widget[3]=new QWidget(this);
		QStringList list;
		list<<tr("Top")<<tr("Bottom")<<tr("Slide")<<tr("Reverse")<<tr("Guest")<<tr("Advanced")<<tr("Color")<<tr("Whole");
		auto grid=new QGridLayout(d->widget[3]);

		auto g=new QHBoxLayout;
		for(int i=0;i<8;++i){
			d->check[i]=new QCheckBox(list[i],d->widget[3]);
			d->check[i]->setFixedHeight(40);
			d->check[i]->setChecked(Shield::shieldG[i]);
			connect(d->check[i],&QCheckBox::stateChanged,[=](int state){
				Shield::shieldG[i]=state==Qt::Checked;
			});
			g->addWidget(d->check[i]);
		}
		grid->addLayout(g,0,0,1,4);

		d->type=new QComboBox(d->widget[3]);
		d->type->addItem(tr("Text"));
		d->type->addItem(tr("User"));
		d->edit=new QLineEdit(d->widget[3]);
		d->edit->setFixedHeight(25);
		d->regexp=new QListView(d->widget[3]);
		d->sender=new QListView(d->widget[3]);
		d->regexp->setSelectionMode(QListView::ExtendedSelection);
		d->sender->setSelectionMode(QListView::ExtendedSelection);
		d->regexp->setModel(d->rm=new QStringListModel(d->regexp));
		d->sender->setModel(d->sm=new QStringListModel(d->sender));
		Utils::setSelection(d->regexp);
		Utils::setSelection(d->sender);
		connect(d->regexp,&QListView::pressed,[d](QModelIndex){d->sender->setCurrentIndex(QModelIndex());});
		connect(d->sender,&QListView::pressed,[d](QModelIndex){d->regexp->setCurrentIndex(QModelIndex());});
		QStringList r,s;
		for(const auto &i:Shield::shieldR){
			r.append(i.pattern());
		}
		for(const auto &i:Shield::shieldS){
			s.append(i);
		}
		std::sort(s.begin(),s.end());
		d->rm->setStringList(r);
		d->sm->setStringList(s);
		d->action[0]=new QAction(tr("Add"),d->widget[3]);
		d->action[1]=new QAction(tr("Del"),d->widget[3]);
		d->action[2]=new QAction(tr("Import"),d->widget[3]);
		d->action[3]=new QAction(tr("Export"),d->widget[3]);
		d->action[1]->setShortcut(QKeySequence("Del"));
		d->action[2]->setShortcut(QKeySequence("Ctrl+I"));
		d->action[3]->setShortcut(QKeySequence("Ctrl+E"));
		connect(d->action[0],&QAction::triggered,[d](){
			if(!d->edit->text().isEmpty()){
				QStringListModel *m=d->type->currentIndex()==0?d->rm:d->sm;
				m->insertRow(m->rowCount());
				m->setData(m->index(m->rowCount()-1),d->edit->text());
				d->edit->clear();
			}
		});
		connect(d->action[1],&QAction::triggered,[d](){
			auto remove=[d](QListView *v){
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
			if(d->regexp->hasFocus()){
				remove(d->regexp);
			}
			if(d->sender->hasFocus()){
				remove(d->sender);
			}
		});
		connect(d->action[2],&QAction::triggered,[d,this](){
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
						if(item.startsWith("u=")&&!d->sm->stringList().contains(text)){
							d->sm->insertRow(d->sm->rowCount());
							d->sm->setData(d->sm->index(d->sm->rowCount()-1),text);
						}
						if(item.startsWith("t=")&&!d->rm->stringList().contains(text)){
							d->rm->insertRow(d->rm->rowCount());
							d->rm->setData(d->rm->index(d->rm->rowCount()-1),text);
						}
					}
					d->regexp->setCurrentIndex(QModelIndex());
					d->sender->setCurrentIndex(QModelIndex());
				}
			}
		});
		connect(d->action[3],&QAction::triggered,[d,this](){
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
				for(const QString &iter:d->rm->stringList()){
					stream<<"  <item enabled=\"true\">t="<<iter<<"</item>"<<endl;
				}
				for(const QString &iter:d->sm->stringList()){
					stream<<"  <item enabled=\"true\">u="<<iter<<"</item>"<<endl;
				}
				stream<<"</filters>";
			}
		});
		d->widget[3]->addAction(d->action[2]);
		d->widget[3]->addAction(d->action[3]);
		d->button[0]=new QPushButton(tr("Add"),d->widget[3]);
		d->button[1]=new QPushButton(tr("Del"),d->widget[3]);
		d->button[0]->setFixedWidth(60);
		d->button[1]->setFixedWidth(60);
		d->button[0]->setFocusPolicy(Qt::NoFocus);
		d->button[1]->setFocusPolicy(Qt::NoFocus);
		connect(d->button[0],&QPushButton::clicked,d->action[0],&QAction::trigger);
		connect(d->button[1],&QPushButton::clicked,d->action[1],&QAction::trigger);
		d->regexp->addActions(d->widget[3]->actions());
		d->sender->addActions(d->widget[3]->actions());
		d->regexp->setContextMenuPolicy(Qt::ActionsContextMenu);
		d->sender->setContextMenuPolicy(Qt::ActionsContextMenu);

		grid->addWidget(d->type,1,0);
		grid->addWidget(d->edit,1,1);
		grid->addWidget(d->button[0],1,2);
		grid->addWidget(d->button[1],1,3);
		grid->addWidget(d->regexp,2,0,1,2);
		grid->addWidget(d->sender,2,2,1,2);

		d->same=new QSlider(Qt::Horizontal,d->widget[3]);
		d->same->setRange(0,40);
		d->same->setValue(Config::getValue("/Shield/Limit",5));
		connect(d->same,&QSlider::valueChanged,[d](int value){
			Config::setValue("/Shield/Limit",value);
			QPoint p;
			p.setX(QCursor::pos().x());
			p.setY(d->same->mapToGlobal(d->same->rect().center()).y());
			QToolTip::showText(p,QString::number(value));
		});
		auto a=new QHBoxLayout;
		a->addWidget(d->same);
		d->label[0]=new QGroupBox(tr("limit of the same"),d->widget[3]);
		d->label[0]->setToolTip(tr("0 means disabled"));
		d->label[0]->setLayout(a);
		grid->addWidget(d->label[0],3,0,1,2);

		d->limit=new QLineEdit(d->widget[3]);
		d->limit->setText(QString::number(Config::getValue("/Shield/Density",100)));
		connect(d->limit,&QLineEdit::editingFinished,[d](){
			Config::setValue("/Shield/Density",d->limit->text().toInt());
		});
		auto m=new QHBoxLayout;
		m->addWidget(d->limit);
		d->label[1]=new QGroupBox(tr("limit of density"),d->widget[3]);
		d->label[1]->setToolTip(tr("0 means disabled"));
		d->label[1]->setLayout(m);
		grid->addWidget(d->label[1],3,2,1,2);

		d->tab->addTab(d->widget[3],tr("Shield"));
	}
	//Network
	{
		d->widget[4]=new QWidget(this);
		auto list=new QVBoxLayout(d->widget[4]);

		auto l=new QGridLayout;
		l->setColumnStretch(0,1);
		l->setColumnStretch(1,1);
		l->setColumnStretch(2,1);
		l->setColumnStretch(3,1);
		d->sheet[0]=new QLineEdit(d->widget[4]);
		d->sheet[0]->setPlaceholderText(tr("Username"));
		d->sheet[1]=new QLineEdit(d->widget[4]);
		d->sheet[1]->setPlaceholderText(tr("Password"));
		d->sheet[1]->setEchoMode(QLineEdit::Password);
		d->sheet[2]=new QLineEdit(d->widget[4]);
		d->sheet[2]->setPlaceholderText(tr("Identifier"));
		auto checkout=[d](){
			bool flag=true;
			for(QLineEdit *iter:d->sheet){
				if(iter->text().isEmpty()){
					flag=false;
					break;
				}
			}
			if(flag){
				d->click->click();
			}
		};
		for(QLineEdit *iter:d->sheet){
			connect(iter,&QLineEdit::editingFinished,checkout);
		}
		connect(d->sheet[2],&QLineEdit::textEdited,[d](QString text){
			d->sheet[2]->setText(text.toUpper());
		});
		l->addWidget(d->sheet[0],0,0);
		l->addWidget(d->sheet[1],0,1);
		l->addWidget(d->sheet[2],0,2);
		d->info=new QLabel(d->widget[4]);
		d->info->setFixedWidth(100);
		d->info->setAlignment(Qt::AlignCenter);
		d->info->setText(tr("waiting"));
		l->addWidget(d->info,0,3,Qt::AlignCenter);
		auto loadValid=[=](){
			QString url=QString("https://secure.bilibili.com/captcha?r=%1").arg(qrand()/(double)RAND_MAX);
			d->fillPicture(d->info,url,tr("error"),QSize(200,25));
		};
		auto setLogged=[d,this,loadValid](bool logged){
			if(logged){
				d->info->setText(tr("logged"));
				d->sheet[1]->clear();
				d->sheet[2]->clear();
				d->click->setText(tr("logout"));
			}
			else{
				loadValid();
				d->click->setText(tr("login"));
			}
			for(QLineEdit *iter:d->sheet){
				iter->setEnabled(!logged);
			}
		};
		auto sendLogin=[d,this,setLogged](){
			d->click->setEnabled(false);
			QUrlQuery query;
			query.addQueryItem("act","login");
			query.addQueryItem("userid",d->sheet[0]->text());
			query.addQueryItem("pwd",d->sheet[1]->text());
			query.addQueryItem("vdcode",d->sheet[2]->text());
			query.addQueryItem("keeptime","2592000");
			QByteArray data=query.query().toUtf8();
			QNetworkRequest request(QUrl("https://secure.bilibili.com/login"));
			request.setHeader(QNetworkRequest::ContentTypeHeader,"application/x-www-form-urlencoded");
			request.setHeader(QNetworkRequest::ContentLengthHeader,data.length());
			QNetworkReply *reply=d->manager->post(request,data);
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
				d->click->setEnabled(true);
				setLogged(flag);
				reply->deleteLater();
			});
		};
		auto setLogout=[=](){
			d->click->setEnabled(false);
			QString url="https://secure.bilibili.com/login?act=exit";
			QNetworkReply *reply=d->manager->get(QNetworkRequest(url));
			connect(reply,&QNetworkReply::finished,[=](){
				d->click->setEnabled(true);
				setLogged(reply->error()!=QNetworkReply::NoError);
			});
		};
		d->click=new QPushButton(tr("login"),d->widget[1]);
		d->click->setFocusPolicy(Qt::NoFocus);
		connect(d->click,&QPushButton::clicked,[=](){
			if(d->click->text()==tr("login")){
				for(QLineEdit *iter:d->sheet){
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
		QNetworkReply *reply=d->manager->get(QNetworkRequest(QString("http://member.bilibili.com/main.html")));
		connect(reply,&QNetworkReply::finished,[=](){
			bool flag=false;
			if(reply->error()==QNetworkReply::NoError){
				QString user=QRegularExpression("(?<=\\>).*(?=\\<\\/h3\\>)").match(reply->readAll()).captured();
				if(!user.isEmpty()){
					d->sheet[0]->setText(user);
					flag=true;
				}
			}
			setLogged(flag);
		});
		l->addWidget(d->click);
		d->login=new QGroupBox(tr("login"),d->widget[4]);
		d->login->setLayout(l);
		list->addWidget(d->login);

		auto c=new QHBoxLayout;
		d->text=new QLabel(d->widget[4]);
		auto m=[d](){
			QString s=tr("current size is %.2fMB");
			s.sprintf(s.toUtf8(),DCache::data->cacheSize()/(1024.0*1024));
			d->text->setText(s);
		};
		m();
		c->addWidget(d->text,2);
		c->addStretch(1);
		d->clear=new QPushButton(tr("clear"),d->widget[4]);
		d->clear->setFocusPolicy(Qt::NoFocus);
		connect(d->clear,&QPushButton::clicked,[d,m](){
			DCache::data->clear();
			m();
		});
		c->addWidget(d->clear,1);
		d->cache=new QGroupBox(tr("cache"),d->widget[4]);
		d->cache->setLayout(c);
		list->addWidget(d->cache);

		auto p=new QGridLayout;
		d->arg=new QComboBox(d->widget[4]);
		d->arg->addItems(QStringList()<<tr("No Proxy")<<tr("Http Proxy")<<tr("Socks5 Proxy"));
		connect<void (QComboBox::*)(int)>(d->arg,&QComboBox::currentIndexChanged,[d](int index){
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
			for(QLineEdit *iter:d->input){
				iter->setEnabled(index!=0);
			}
		});
		p->addWidget(d->arg,0,0);
		d->input[0]=new QLineEdit(d->widget[4]);
		d->input[0]->setText(QNetworkProxy::applicationProxy().hostName());
		d->input[0]->setPlaceholderText(tr("HostName"));
		connect(d->input[0],&QLineEdit::editingFinished,[d](){
			QNetworkProxy proxy=QNetworkProxy::applicationProxy();
			proxy.setHostName(d->input[0]->text());
			QNetworkProxy::setApplicationProxy(proxy);
		});
		p->addWidget(d->input[0],1,0);
		d->input[1]=new QLineEdit(d->widget[4]);
		d->input[1]->setPlaceholderText(tr("Port"));
		int port=QNetworkProxy::applicationProxy().port();
		d->input[1]->setText(port?QString::number(port):QString());
		connect(d->input[1],&QLineEdit::editingFinished,[d](){
			QNetworkProxy proxy=QNetworkProxy::applicationProxy();
			proxy.setPort(d->input[1]->text().toInt());
			if(proxy.port()==0){
				d->input[1]->clear();
			}
			QNetworkProxy::setApplicationProxy(proxy);
		});
		p->addWidget(d->input[1],1,1);
		d->input[2]=new QLineEdit(d->widget[4]);
		d->input[2]->setPlaceholderText(tr("User"));
		d->input[2]->setText(QNetworkProxy::applicationProxy().user());
		connect(d->input[2],&QLineEdit::editingFinished,[d](){
			QNetworkProxy proxy=QNetworkProxy::applicationProxy();
			proxy.setUser(d->input[2]->text());
			QNetworkProxy::setApplicationProxy(proxy);
		});
		p->addWidget(d->input[2],1,2);
		d->input[3]=new QLineEdit(d->widget[4]);
		d->input[3]->setPlaceholderText(tr("Password"));
		d->input[3]->setText(QNetworkProxy::applicationProxy().password());
		connect(d->input[3],&QLineEdit::editingFinished,[d](){
			QNetworkProxy proxy=QNetworkProxy::applicationProxy();
			proxy.setPassword(d->input[3]->text());
			QNetworkProxy::setApplicationProxy(proxy);
		});
		p->addWidget(d->input[3],1,3);
		for(QLineEdit *iter:d->input){
			iter->setEnabled(false);
		}
		switch(QNetworkProxy::applicationProxy().type()){
		case QNetworkProxy::HttpProxy:
			d->arg->setCurrentIndex(1);
			break;
		case QNetworkProxy::Socks5Proxy:
			d->arg->setCurrentIndex(2);
			break;
		default:
			d->arg->setCurrentIndex(0);
			break;
		}
		d->proxy=new QGroupBox(tr("proxy"),d->widget[4]);
		d->proxy->setLayout(p);
		list->addWidget(d->proxy);

		list->addStretch(10);
		d->tab->addTab(d->widget[4],tr("Network"));
	}
	//Plugin
	{
		d->widget[5]=new QWidget(this);
		auto w=new QGridLayout(d->widget[5]);
		d->plugin=new QTreeWidget(d->widget[5]);
		d->plugin->setSelectionMode(QAbstractItemView::NoSelection);
		QStringList header;
		header<<tr("Enable")<<tr("Name")<<tr("Version")<<tr("Description")<<tr("Author")<<"";
		d->plugin->setHeaderLabels(header);
		d->plugin->setColumnWidth(0,60);
		d->plugin->setColumnWidth(1,75);
		d->plugin->setColumnWidth(2,45);
		d->plugin->setColumnWidth(3,180);
		d->plugin->setColumnWidth(4,75);
		d->plugin->setColumnWidth(5,30);
		w->addWidget(d->plugin);
		for(Plugin &iter:Plugin::plugins){
			QStringList content;
			content+="";
			content+=iter.string("Name");
			content+=iter.string("Version");
			content+=iter.string("Description");
			content+=iter.string("Author");
			content+=tr("options");
			QTreeWidgetItem *row=new QTreeWidgetItem(d->plugin,content);
			row->setCheckState(0,Config::getValue("/Plugin/"+iter.string("Name"),true)?Qt::Checked:Qt::Unchecked);
			row->setData(0,Qt::UserRole,(quintptr)&iter);
			QFont f;
			f.setUnderline(true);
			row->setData(5,Qt::FontRole,f);
			row->setData(5,Qt::ForegroundRole,qApp->palette().color(QPalette::Highlight));
			row->setTextAlignment(5,Qt::AlignCenter);
			row->setSizeHint(0,QSize(60,40));
		}
		connect(d->plugin,&QTreeWidget::itemChanged,[d](QTreeWidgetItem *item){
			Plugin *p=(Plugin *)item->data(0,Qt::UserRole).value<quintptr>();
			Config::setValue("/Plugin/"+p->string("Name"),item->checkState(0)==Qt::Checked);
		});
		connect(d->plugin,&QTreeWidget::itemClicked,[=](QTreeWidgetItem *item,int column){
			if(column==5){
				((Plugin *)item->data(0,Qt::UserRole).value<quintptr>())->config(this);
			}
		});
		connect(d->plugin,&QTreeWidget::currentItemChanged,[d](){
			d->plugin->setCurrentItem(NULL);
		});
		d->tab->addTab(d->widget[5],tr("Plugin"));
	}
	//Shortcut
	{
		d->widget[6]=new QWidget(this);
		auto w=new QGridLayout(d->widget[6]);
		d->hotkey=new QTreeWidget(d->widget[6]);
		d->hotkey->setSelectionMode(QAbstractItemView::NoSelection);
		d->hotkey->header()->hide();
		d->hotkey->setColumnCount(2);
		d->hotkey->setColumnWidth(0,350);
		d->hotkey->setEditTriggers(QAbstractItemView::NoEditTriggers);
		for(QAction *iter:Local::mainWidget()->findChildren<QAction *>(QRegularExpression(".{4}"))){
			QTreeWidgetItem *item=new QTreeWidgetItem;
			item->setData(0,Qt::DisplayRole,iter->text());
			item->setData(1,Qt::DisplayRole,iter->shortcut().toString());
			item->setData(1,Qt::UserRole,iter->objectName());
			item->setFlags(Qt::ItemIsEditable|Qt::ItemIsEnabled);
			item->setSizeHint(0,QSize(60,35));
			d->hotkey->addTopLevelItem(item);
		}
		w->addWidget(d->hotkey);
		connect(d->hotkey,&QTreeWidget::currentItemChanged,[d](){
			d->hotkey->setCurrentItem(NULL);
		});
		connect(d->hotkey,&QTreeWidget::itemClicked,[=](QTreeWidgetItem *item,int column){
			if(column==1){
				d->hotkey->editItem(item,1);
			}
		});
		connect(d->hotkey,&QTreeWidget::itemChanged,[=](QTreeWidgetItem *item,int column){
			if(column==1){
				QVariant v=item->data(1,Qt::UserRole);
				if(v.isValid()){
					QAction *a=Local::mainWidget()->findChild<QAction *>(v.toString());
					QString ns=item->text(1);
					a->setShortcut(ns);
					Config::setValue("/Shortcut/"+a->objectName(),ns);
				}
			}
		});
		d->tab->addTab(d->widget[6],tr("Shortcut"));
	}
	//Thanks
	{
		d->widget[7]=new QWidget(this);
		auto w=new QGridLayout(d->widget[7]);
		QFile t(":/Text/THANKS");
		t.open(QIODevice::ReadOnly|QIODevice::Text);
		d->thanks=new QTextEdit(d->widget[7]);
		d->thanks->setReadOnly(true);
		d->thanks->setText(t.readAll());
		w->addWidget(d->thanks);
		d->tab->addTab(d->widget[7],tr("Thanks"));
	}
	//License
	{
		d->widget[8]=new QWidget(this);
		auto w=new QGridLayout(d->widget[8]);
		QFile l(":/Text/COPYING");
		l.open(QIODevice::ReadOnly|QIODevice::Text);
		d->license=new QTextEdit(d->widget[8]);
		d->license->setReadOnly(true);
		d->license->setText(l.readAll());
		w->addWidget(d->license);
		d->tab->addTab(d->widget[8],tr("License"));
	}
	d->tab->setCurrentIndex(index);
	connect(this,&QDialog::finished,[d,this](){
		if(d->reparse!=d->getReparse()){
			Shield::shieldR.clear();
			for(QString item:d->rm->stringList()){
				Shield::shieldR.append(QRegularExpression(item));
			}
			Shield::shieldS=d->sm->stringList().toSet();
			Danmaku::instance()->parse(0x2);
		}
		if(d->restart!=d->getRestart()){
			if((APlayer::instance()->getState()==APlayer::Stop&&
				Danmaku::instance()->rowCount()==0)||
					QMessageBox::warning(this,
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

	d->restart=d->getRestart();
	d->reparse=d->getReparse();
}

Config::~Config()
{
	delete d_ptr;
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
