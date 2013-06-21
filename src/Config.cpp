#include "Config.h"

Config::Config(QWidget *parent,int index):
	QDialog(parent)
{
	resize(540,450);
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
		danm[0]->setChecked(Utils::getConfig("/Danmaku/Clear",true));
		connect(danm[0],&QCheckBox::stateChanged,[this](int state){
			Utils::setConfig("/Danmaku/Clear",state==Qt::Checked);
		});
		c->addWidget(danm[0]);
		danm[1]=new QCheckBox(tr("auto delay after loaded"),widget[0]);
		danm[1]->setChecked(Utils::getConfig("/Danmaku/Delay",false));
		connect(danm[1],&QCheckBox::stateChanged,[this](int state){
			Utils::setConfig("/Danmaku/Delay",state==Qt::Checked);
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
		box[1]->setLayout(s);
		list->addWidget(box[1]);

		auto l=new QHBoxLayout;
		play[1]=new QLineEdit(widget[0]);
		play[1]->setText(QString::number(Utils::getConfig("/Danmaku/Life",5),'f',2));
		connect(play[1],&QLineEdit::editingFinished,[this](){
			Utils::setConfig("/Danmaku/Life",play[1]->text().toDouble());
		});
		l->addWidget(play[1]);
		box[2]=new QGroupBox(tr("life time"),widget[0]);
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

		auto j=new QHBoxLayout;
		play[3]=new QLineEdit(widget[0]);
		play[3]->setText(QString::number(Utils::getConfig("/Playing/Interval",10),'f',2));
		connect(play[3],&QLineEdit::editingFinished,[this](){
			Utils::setConfig("/Playing/Interval",play[3]->text().toDouble());
		});
		j->addWidget(play[3]);
		box[4]=new QGroupBox(tr("skip time"),widget[0]);
		box[4]->setLayout(j);
		list->addWidget(box[4]);

		tab->addTab(widget[0],tr("Playing"));
	}
	//Shield
	{
		widget[1]=new QWidget(this);
		QStringList list={tr("Top"),tr("Bottom"),tr("Slide"),tr("Guest"),tr("Color"),tr("Whole")};
		auto lines=new QVBoxLayout(widget[1]);

		auto g=new QHBoxLayout;
		for(int i=0;i<6;++i){
			check[i]=new QCheckBox(list[i],widget[1]);
			check[i]->setFixedHeight(40);
			check[i]->setChecked(Shield::block[i]);
			connect(check[i],&QCheckBox::stateChanged,[=](int state){
				Shield::block[i]=state==Qt::Checked;
			});
			g->addWidget(check[i]);
		}
		lines->addLayout(g);

		edit=new QLineEdit(widget[1]);
		edit->setFixedHeight(25);
		regexp=new QListView(widget[1]);
		sender=new QListView(widget[1]);
		regexp->setModel(rm=new QStringListModel(regexp));
		sender->setModel(sm=new QStringListModel(sender));
		QStringList re;
		for(const auto &item:Shield::shieldR){
			re.append(item.pattern());
		}
		rm->setStringList(re);
		sm->setStringList(Shield::shieldU);
		action[0]=new QAction(tr("Add"),widget[1]);
		action[1]=new QAction(tr("Del"),widget[1]);
		action[2]=new QAction(tr("Import"),widget[1]);
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
		widget[1]->addAction(action[1]);
		widget[1]->addAction(action[2]);
		button[0]=new QPushButton(tr("Add"),widget[1]);
		button[1]=new QPushButton(tr("Del"),widget[1]);
		button[0]->setFixedHeight(25);
		button[1]->setFixedHeight(25);
		button[0]->setFocusPolicy(Qt::NoFocus);
		button[1]->setFocusPolicy(Qt::NoFocus);
		connect(button[0],&QPushButton::clicked,action[0],&QAction::trigger);
		connect(button[1],&QPushButton::clicked,action[1],&QAction::trigger);
		widget[1]->setContextMenuPolicy(Qt::ActionsContextMenu);
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

		limit=new QSlider(Qt::Horizontal,widget[1]);
		limit->setRange(5,20);
		int li=Utils::getConfig("/Shield/Limit",0);
		limit->setValue(li==0?20:li);
		connect(limit,&QSlider::valueChanged,[this](int value){
			Utils::setConfig("/Shield/Limit",value==20?0:value);
		});
		auto a=new QHBoxLayout;
		a->addWidget(limit);
		label=new QGroupBox(tr("limit of the same"),widget[1]);
		label->setLayout(a);
		lines->addWidget(label);

		tab->addTab(widget[1],tr("Shield"));
	}
	//Thanks
	{
		widget[2]=new QWidget(this);
		auto w=new QGridLayout(widget[2]);
		QFile t(":/Text/THANKS");
		t.open(QIODevice::ReadOnly|QIODevice::Text);
		thanks=new QTextEdit(widget[2]);
		thanks->setReadOnly(true);
		thanks->setAcceptRichText(true);
		thanks->setText(t.readAll());
		w->addWidget(thanks);
		tab->addTab(widget[2],tr("Thanks"));
	}
	//License
	{
		widget[3]=new QWidget(this);
		auto w=new QGridLayout(widget[3]);
		QFile l(":/Text/COPYING");
		l.open(QIODevice::ReadOnly|QIODevice::Text);
		license=new QTextEdit(widget[3]);
		license->setReadOnly(true);
		license->setAcceptRichText(true);
		license->setText(l.readAll());
		w->addWidget(license);
		tab->addTab(widget[3],tr("License"));
	}
	tab->setCurrentIndex(index);
}

Config::~Config()
{
	Shield::shieldR.clear();
	Shield::shieldU.clear();
	for(QString item:rm->stringList()){
		Shield::shieldR.append(QRegExp(item));
	}
	for(QString item:sm->stringList()){
		Shield::shieldU.append(item);
	}
}
