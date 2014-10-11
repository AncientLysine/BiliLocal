/*=======================================================================
*
*   Copyright (C) 2013 Lysine.
*
*   Filename:    Menu.cpp
*   Time:        2013/04/05
*   Author:      Lysine
*   Contributor: Chaserhkj
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

#include "Menu.h"
#include "APlayer.h"
#include "Config.h"
#include "Danmaku.h"
#include "History.h"
#include "Load.h"
#include "Local.h"
#include "Render.h"
#include "Search.h"
#include "Utils.h"

namespace{
class LoadModelWapper:public QAbstractItemModel
{
public:
	explicit LoadModelWapper(QAbstractItemModel *m,QString s=QString()):
		QAbstractItemModel(m),s(s),m(m)
	{
	}

	int columnCount(const QModelIndex &parent) const
	{
		return m->columnCount(parent);
	}

	QVariant data(const QModelIndex &index,int role) const
	{
		if(isFakeItem(index)){
			switch(role){
			case Qt::TextAlignmentRole:
				return Qt::AlignCenter;
			case Qt::FontRole:
			{
				QFont f;
				f.setBold(true);
				return f;
			}
			case Qt::DisplayRole:
			case Qt::EditRole:
				return s;
			case Qt::BackgroundRole:
				return QColor(0xA0A0A4);
			case Qt::ForegroundRole:
				return QColor(0xFFFFFF);
			case Qt::SizeHintRole:
				return QSize(0,20);
			case Load::UrlRole:
				return QUrl();
			case Load::StrRole:
				return "";
			default:
				return QVariant();
			}
		}
		else{
			return m->data(index,role);
		}
	}

	Qt::ItemFlags flags(const QModelIndex &) const
	{
		return Qt::ItemIsSelectable|Qt::ItemIsEnabled;
	}

	QModelIndex index(int row,int column,const QModelIndex &parent) const
	{
		if(!parent.isValid()&&row==0){
			//'F'+'A'+'K'+'E'==279
			return createIndex(row,column,279);
		}
		else{
			return m->index(row-1,column,parent);
		}
	}

	QModelIndex parent(const QModelIndex &child) const
	{
		return isFakeItem(child)?QModelIndex():m->parent(child);
	}

	int rowCount(const QModelIndex &parent) const
	{
		return m->rowCount(parent)+(parent.isValid()?0:1);
	}

	bool isFakeItem(const QModelIndex &index) const
	{
		return index.internalId()==279;
	}

private:
	QString s;
	QAbstractItemModel *m;
};

class MFileEdit:public QLineEdit
{
public:
	MFileEdit(QCompleter *completer,QWidget *parent=0):
		QLineEdit(parent),completer(completer)
	{
		historyFlag=0;
		connect(this,&QLineEdit::selectionChanged,[this](){historyFlag=0;});
	}

	void mousePressEvent(QMouseEvent *e)
	{
		if (e->button()==Qt::LeftButton){
			historyFlag=1;
		}
		QLineEdit::mousePressEvent(e);
	}

	void mouseReleaseEvent(QMouseEvent *e)
	{
		if (e->button()==Qt::LeftButton){
			if (historyFlag){
				completer->complete();
				completer->popup()->setCurrentIndex(completer->model()->index(0,0));
			}
			historyFlag=0;
		}
		QLineEdit::mouseReleaseEvent(e);
	}

private:
	bool historyFlag;
	QCompleter *completer;
};
}

Menu::Menu(QWidget *parent):
	QWidget(parent)
{
	setObjectName("Menu");
	isStay=isPoped=false;
	Utils::setGround(this,Qt::white);
	fileC=new QCompleter(History::instance()->getModel(),this);
	fileL=new MFileEdit(fileC,this);
	danmL=new QLineEdit(this);
	sechL=new QLineEdit(this);
	fileL->installEventFilter(this);
	danmL->installEventFilter(this);
	sechL->installEventFilter(this);
	fileL->setReadOnly(true);
	fileL->setPlaceholderText(tr("choose a local media"));
	danmL->setPlaceholderText(tr("input av/ac number"));
	sechL->setPlaceholderText(tr("search danmaku online"));
	fileL->setGeometry(QRect(10,25, 120,25));
	danmL->setGeometry(QRect(10,65, 120,25));
	sechL->setGeometry(QRect(10,105,120,25));
	connect(danmL,&QLineEdit::textEdited,[this](QString text){
		QRegularExpression regexp("(av|ac|dd)((\\d+)([#_])?(\\d+)?)?|[ad]");
		regexp.setPatternOptions(QRegularExpression::CaseInsensitiveOption);
		auto iter=regexp.globalMatch(text);
		QString match;
		while(iter.hasNext()){
			match=iter.next().captured();
		}
		danmL->setText(match.toLower().replace('_','#'));
	});
	QAbstractItemView *popup;
	fileC->setCompletionMode(QCompleter::UnfilteredPopupCompletion);
	fileC->setWidget(fileL);
	popup=fileC->popup();
	popup->setMouseTracking(true);
	QAction *hdelA=new QAction(popup);
	hdelA->setShortcut(QKeySequence(Qt::Key_Delete));
	connect(hdelA,&QAction::triggered,[this](){
		fileC->model()->removeRow(fileC->popup()->currentIndex().row());
	});
	popup->addAction(hdelA);
	connect(popup,SIGNAL(entered(QModelIndex)),popup,SLOT(setCurrentIndex(QModelIndex)));
	connect<void (QCompleter::*)(const QModelIndex &)>(fileC,&QCompleter::activated,[this](const QModelIndex &index){
		setFocus();
		History::instance()->rollback(index);
	});

	danmC=new QCompleter(new LoadModelWapper(Load::instance()->getModel(),tr("Load All")),this);
	danmC->setCompletionMode(QCompleter::UnfilteredPopupCompletion);
	danmC->setWidget(danmL);
	popup=danmC->popup();
	popup->setMouseTracking(true);
	connect(popup,SIGNAL(entered(QModelIndex)),popup,SLOT(setCurrentIndex(QModelIndex)));
	connect<void (QCompleter::*)(const QModelIndex &)>(danmC,&QCompleter::activated,[this](const QModelIndex &index){
		QModelIndex i;
		QVariant v=index.data(Load::UrlRole);
		if(v.isNull()||v.toUrl().isValid()){
			i=index;
		}
		Load::instance()->dequeue();
		Load::instance()->loadDanmaku(i);
	});
	fileB=new QPushButton(this);
	sechB=new QPushButton(this);
	danmB=new QPushButton(this);
	fileB->setGeometry(QRect(135,25, 55,25));
	danmB->setGeometry(QRect(135,65, 55,25));
	sechB->setGeometry(QRect(135,105,55,25));
	fileB->setText(tr("Open"));
	danmB->setText(tr("Load"));
	sechB->setText(tr("Search"));
	fileA=new QAction(tr("Open File"),this);
	fileA->setObjectName("File");
	fileA->setShortcut(Config::getValue("/Shortcut/File",QString()));
	danmA=new QAction(tr("Load Danmaku"),this);
	danmA->setObjectName("Danm");
	danmA->setShortcut(Config::getValue("/Shortcut/Danm",QString()));
	sechA=new QAction(tr("Search Danmaku"),this);
	sechA->setObjectName("Sech");
	sechA->setShortcut(Config::getValue("/Shortcut/Sech",QString()));
	connect(fileA,&QAction::triggered,[this](){
		QString _file=QFileDialog::getOpenFileName(Local::mainWidget(),
												   tr("Open File"),
												   Utils::defaultPath(),
												   tr("Media files (%1);;All files (*.*)").arg(Utils::getSuffix(Utils::Video|Utils::Audio,"*.%1").join(' ')));
		if(!_file.isEmpty()){
			APlayer::instance()->setMedia(_file);
		}
	});
	connect(danmA,&QAction::triggered,[this](){
		if(Config::getValue("/Danmaku/Local",false)){
			QString _file=QFileDialog::getOpenFileName(Local::mainWidget(),
													   tr("Open File"),
													   Utils::defaultPath(),
													   tr("Danmaku files (%1);;All files (*.*)").arg(Utils::getSuffix(Utils::Danmaku,"*.%1").join(' ')));
			if(!_file.isEmpty()){
				Load::instance()->loadDanmaku(_file);
			}
		}
		else{
			if(danmL->text().isEmpty()){
				pop();
				isStay=true;
				danmL->setFocus();
			}
			else{
				Load::instance()->loadDanmaku(danmL->text());
			}
		}
	});
	connect(sechA,&QAction::triggered,[this](){
		Search searchBox(Local::mainWidget());
		sechL->setText(sechL->text().simplified());
		if(!sechL->text().isEmpty()){
			searchBox.setKey(sechL->text());
		}
		if(searchBox.exec()) {
			Load::instance()->loadDanmaku(searchBox.getAid());
		}
		sechL->setText(searchBox.getKey());
	});
	addAction(fileA);
	addAction(danmA);
	addAction(sechA);
	connect(fileB,&QPushButton::clicked,fileA,&QAction::trigger);
	connect(danmB,&QPushButton::clicked,danmA,&QAction::trigger);
	connect(sechB,&QPushButton::clicked,sechA,&QAction::trigger);
	connect(danmL,&QLineEdit::returnPressed,danmA,&QAction::trigger);
	connect(sechL,&QLineEdit::returnPressed,sechA,&QAction::trigger);
	alphaT=new QLabel(this);
	alphaT->setGeometry(QRect(10,145,100,25));
	alphaT->setText(tr("Danmaku Alpha"));
	alphaS=new QSlider(this);
	alphaS->setOrientation(Qt::Horizontal);
	alphaS->setGeometry(QRect(10,170,180,15));
	alphaS->setRange(0,100);
	alphaS->setValue(Config::getValue("/Danmaku/Alpha",100));
	connect(alphaS,&QSlider::valueChanged,[this](int _alpha){
		Config::setValue("/Danmaku/Alpha",_alpha);
		QPoint p;
		p.setX(QCursor::pos().x());
		p.setY(alphaS->mapToGlobal(alphaS->rect().center()).y());
		QToolTip::showText(p,QString::number(_alpha));
	});
	powerT=new QLabel(this);
	powerT->setGeometry(QRect(10,205,100,20));
	powerT->setText(tr("Danmaku Power"));
	powerL=new QLineEdit(this);
	powerL->setGeometry(QRect(160,205,30,20));
	powerL->setValidator(new QRegularExpressionValidator(QRegularExpression("^\\w*$"),powerL));
	connect(powerL,&QLineEdit::editingFinished,[this](){
		Render::instance()->setRefreshRate(powerL->text().toInt());
	});
	connect(Render::instance(),&Render::refreshRateChanged,[this](int fps){
		if(fps==0){
			powerL->clear();
		}
		else{
			powerL->setText(QString::number(fps));
		}
	});
	localT=new QLabel(this);
	localT->setGeometry(QRect(10,240,100,25));
	localT->setText(tr("Local Danmaku"));
	localC=new QCheckBox(this);
	localC->setGeometry(QRect(168,240,25,25));
	connect(localC,&QCheckBox::stateChanged,[this](int state){
		bool local=state==Qt::Checked;
		danmL->setText("");
		sechL->setText("");
		danmL->setReadOnly(local);
		sechL->setEnabled(!local);
		sechB->setEnabled(!local);
		sechA->setEnabled(!local);
		danmB->setText(local?tr("Open"):tr("Load"));
		danmL->setPlaceholderText(local?tr("choose a local danmaku"):tr("input av/ac number"));
		for(const Record &r:Danmaku::instance()->getPool()){
			if(QUrl(r.source).isLocalFile()==local){
				danmL->setText(r.string);
			}
		}
		danmL->setCursorPosition(0);
		Config::setValue("/Danmaku/Local",local);
	});
	localC->setChecked(Config::getValue("/Danmaku/Local",false));
	subT=new QLabel(this);
	subT->setGeometry(QRect(10,275,100,25));
	subT->setText(tr("Protect Sub"));
	subC=new QCheckBox(this);
	subC->setGeometry(QRect(168,275,25,25));
	subC->setChecked(Config::getValue("/Danmaku/Protect",false));
	connect(subC,&QCheckBox::stateChanged,[this](int state){
		Config::setValue("/Danmaku/Protect",state==Qt::Checked);
	});
	loopT=new QLabel(this);
	loopT->setGeometry(QRect(10,310,100,25));
	loopT->setText(tr("Loop Playback"));
	loopC=new QCheckBox(this);
	loopC->setGeometry(QRect(168,310,25,25));
	loopC->setChecked(Config::getValue("/Playing/Loop",false));
	connect(loopC,&QCheckBox::stateChanged,[this](int state){
		Config::setValue("/Playing/Loop",state==Qt::Checked);
	});

	animation=new QPropertyAnimation(this,"pos",this);
	animation->setDuration(200);
	animation->setEasingCurve(QEasingCurve::OutCubic);
	connect(animation,&QPropertyAnimation::finished,[this](){
		if(!isPoped){
			hide();
		}
	});
	connect(Load::instance(),&Load::stateChanged,[this](int state){
		switch(state){
		case Load::Page:
			isStay=1;
			break;
		case Load::Part:
			danmL->setText(Load::instance()->getStr());
			if(isPoped&&animation->state()==QAbstractAnimation::Stopped){
				danmC->complete();
				danmC->popup()->setCurrentIndex(danmC->model()->index(0,0));
			}
		case Load::File:
		case Load::Pool:
			localC->setChecked(QUrl(Load::instance()->getUrl()).isLocalFile());
			danmL->setText(Load::instance()->getStr());
			danmL->setCursorPosition(0);
		case Load::Code:
		case Load::None:
			isStay=0;
			break;
		default:
		{
			QString info=tr("Network error occurred, error code: %1").arg(state);
			QString sugg=Local::instance()->suggestion(state);
			QMessageBox::warning(Local::mainWidget(),tr("Network Error"),sugg.isEmpty()?info:(info+'\n'+sugg));
			isStay=0;
			break;
		}
		}
	});
	connect(APlayer::instance(),&APlayer::mediaChanged,[this](QString _file){
		fileL->setText(QFileInfo(_file).fileName());
		fileL->setCursorPosition(0);
	});
	hide();
}

bool Menu::eventFilter(QObject *o,QEvent *e)
{
	switch(e->type()){
	case QEvent::ContextMenu:
	{
		isStay=1;
		QMenu *m=dynamic_cast<QLineEdit *>(o)->createStandardContextMenu();
		m->exec(dynamic_cast<QContextMenuEvent *>(e)->globalPos());
		delete m;
		isStay=0;
		return 1;
	}
	case QEvent::FocusIn:
		isStay=1;
		break;
	case QEvent::FocusOut:
		isStay=0;
		break;
	}
	return 0;
}

void Menu::pop()
{
	if(!isPoped&&animation->state()==QAbstractAnimation::Stopped){
		show();
		animation->setStartValue(pos());
		animation->setEndValue(pos()+QPoint(200,0));
		animation->start();
		isPoped=true;
	}
}

void Menu::push(bool force)
{
	if(isPoped&&animation->state()==QAbstractAnimation::Stopped&&(!preferStay()||force)){
		if(force){
			isStay=false;
		}
		animation->setStartValue(pos());
		animation->setEndValue(pos()-QPoint(200,0));
		animation->start();
		isPoped=false;
	}
}

void Menu::terminate()
{
	if(animation->state()!=QAbstractAnimation::Stopped){
		animation->setCurrentTime(animation->totalDuration());
	}
}
