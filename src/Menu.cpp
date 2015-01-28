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
#include "List.h"
#include "Load.h"
#include "Local.h"
#include "Render.h"
#include "Search.h"
#include "Utils.h"

namespace{
class LoadProxyModel:public QAbstractProxyModel
{
public:
	explicit LoadProxyModel(QObject *parent=0):
		QAbstractProxyModel(parent)
	{
		setSourceModel(Load::instance()->getModel());
		connect(sourceModel(),&QAbstractItemModel::rowsInserted,this,&LoadProxyModel::endInsertRows);
		connect(sourceModel(),&QAbstractItemModel::rowsAboutToBeInserted,[this](const QModelIndex &parent,int sta,int end){
			beginInsertRows(mapFromSource(parent),sta,end);
		});
	}

	int columnCount(const QModelIndex &) const
	{
		return 1;
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
				return Menu::tr("Load All");
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
			return sourceModel()->data(mapToSource(index),role);
		}
	}

	Qt::ItemFlags flags(const QModelIndex &) const
	{
		return Qt::ItemIsSelectable|Qt::ItemIsEnabled;
	}

	QModelIndex index(int r,int c,const QModelIndex &p) const
	{
		if(!p.isValid()&&r==0&&c==0){
			return createIndex(0,0,1);
		}
		else{
			return createIndex(r,c);
		}
	}

	QModelIndex parent(const QModelIndex &) const
	{
		return QModelIndex();
	}

	int rowCount(const QModelIndex &parent) const
	{
		return sourceModel()->rowCount(mapToSource(parent))+(parent.isValid()?0:1);
	}

	bool isFakeItem(const QModelIndex &index) const
	{
		return index.internalId();
	}

	QModelIndex mapToSource  (const QModelIndex &i) const
	{
		return (isFakeItem(i)||!i.isValid())?QModelIndex():sourceModel()->index(i.row()-1,i.column(),mapToSource(i.parent()));
	}

	QModelIndex mapFromSource(const QModelIndex &i) const
	{
		return i.isValid()?index(i.row()+1,i.column(),mapFromSource(i.parent())):QModelIndex();
	}
};

class ListProxyModel:public QSortFilterProxyModel
{
public:
	explicit ListProxyModel(QObject *parent=0):
		QSortFilterProxyModel(parent)
	{
		setSourceModel(List::instance());
		setSortRole(List::DateRole);
		sort(0,Qt::DescendingOrder);
	}

	QVariant data(const QModelIndex &index,int role) const
	{
		return role==Qt::DecorationRole?QVariant():QSortFilterProxyModel::data(index,role);
	}

private:
	bool filterAcceptsRow(int row,const QModelIndex &parent) const
	{
		QModelIndex i=sourceModel()->index(row,0,parent);
		if (!i.data(List::DateRole).toDateTime().isValid()||i.data(List::CodeRole).toInt()==List::Inherit){
			return false;
		}
		QStandardItem *c=List::instance()->getCurrent();
		return !c||c->index()!=i;
	}
};

class FileEdit:public QLineEdit
{
public:
	explicit FileEdit(QCompleter *completer,QWidget *parent=0):
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

class DanmEdit:public QLineEdit
{
public:
	explicit DanmEdit(QCompleter *completer,QWidget *parent=0):
		QLineEdit(parent)
	{
		completer->popup()->installEventFilter(this);
		connect(this,&DanmEdit::textEdited,this,&DanmEdit::setFixedText);
	}

	bool eventFilter(QObject *,QEvent *e) override
	{
		if (e->type()==QEvent::Hide){
			Load::instance()->dequeue();
		}
		return false;
	}

	void setFixedText(QString text)
	{
		Load::instance()->fixCode(text);
		setText(text);
	}

	void focusInEvent(QFocusEvent *e)
	{
		if(!Config::getValue("/Danmaku/Local",false)){
			setFixedText(text());
		}
		QLineEdit::focusInEvent(e);
	}
};
}

Menu::Menu(QWidget *parent):
	QWidget(parent)
{
	setObjectName("Menu");
	isStay=isPoped=false;
	Utils::setGround(this,Qt::white);
	ListProxyModel *fileM=new ListProxyModel(this);
	fileC=new QCompleter(fileM,this);
	LoadProxyModel *danmM=new LoadProxyModel(this);
	danmC=new QCompleter(danmM,this);
	fileL=new FileEdit(fileC,this);
	danmL=new DanmEdit(danmC,this);
	sechL=new QLineEdit(this);
	fileL->installEventFilter(this);
	danmL->installEventFilter(this);
	sechL->installEventFilter(this);
	fileL->setReadOnly(true);
	fileL->setPlaceholderText(tr("choose a local media"));
	danmL->setPlaceholderText(tr("input av/ac number"));
	sechL->setPlaceholderText(tr("search danmaku online"));
	QAbstractItemView *popup;
	fileC->setCompletionMode(QCompleter::UnfilteredPopupCompletion);
	fileC->setWidget(fileL);
	popup=fileC->popup();
	popup->setMouseTracking(true);
	QAction *hdelA=new QAction(popup);
	hdelA->setShortcut(QKeySequence(Qt::Key_Delete));
	connect(hdelA,&QAction::triggered,[=](){
		QModelIndex index=popup->currentIndex();
		index=dynamic_cast<QAbstractProxyModel *>(fileC->completionModel())->mapToSource(index);
		index=fileM->mapToSource(index);
		List::instance()->itemFromIndex(index)->setData(QVariant(),List::DateRole);
	});
	popup->addAction(hdelA);
	connect(popup,SIGNAL(entered(QModelIndex)),popup,SLOT(setCurrentIndex(QModelIndex)));
	connect<void (QCompleter::*)(const QModelIndex &)>(fileC,&QCompleter::activated,[=](const QModelIndex &index){
		setFocus();
		List::instance()->jumpToIndex(fileM->mapToSource(dynamic_cast<QAbstractProxyModel *>(fileC->completionModel())->mapToSource(index)));
	});
	danmC->setCompletionMode(QCompleter::UnfilteredPopupCompletion);
	danmC->setWidget(danmL);
	popup=danmC->popup();
	popup->setMouseTracking(true);
	connect(popup,SIGNAL(entered(QModelIndex)),popup,SLOT(setCurrentIndex(QModelIndex)));
	connect<void (QCompleter::*)(const QModelIndex &)>(danmC,&QCompleter::activated,[=](const QModelIndex &index){
		setFocus();
		Load::instance()->loadDanmaku(danmM->mapToSource(dynamic_cast<QAbstractProxyModel *>(danmC->completionModel())->mapToSource(index)));
	});
	fileB=new QPushButton(this);
	sechB=new QPushButton(this);
	danmB=new QPushButton(this);
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
		QString _file=QFileDialog::getOpenFileName(lApp->mainWidget(),
												   tr("Open File"),
												   List::instance()->defaultPath(Utils::Video|Utils::Audio),
												   tr("Media files (%1);;All files (*.*)").arg(Utils::getSuffix(Utils::Video|Utils::Audio,"*.%1").join(' ')));
		if(!_file.isEmpty()){
			APlayer::instance()->setMedia(_file);
		}
	});
	connect(danmA,&QAction::triggered,[this](){
		if(Config::getValue("/Danmaku/Local",false)){
			QString _file=QFileDialog::getOpenFileName(lApp->mainWidget(),
													   tr("Open File"),
													   List::instance()->defaultPath(Utils::Danmaku),
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
		Search searchBox(lApp->mainWidget());
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
	alphaT->setText(tr("Danmaku Alpha"));
	alphaS=new QSlider(this);
	alphaS->setOrientation(Qt::Horizontal);
	alphaS->setRange(0,100);
	connect(alphaS,&QSlider::valueChanged,[this](int _alpha){
		Danmaku::instance()->setAlpha(_alpha);
		if (alphaS->isVisible()){
			QPoint p;
			p.setX(QCursor::pos().x());
			p.setY(alphaS->mapToGlobal(alphaS->rect().center()).y());
			QToolTip::showText(p,QString::number(_alpha));
		}
	});
	connect(Danmaku::instance(),&Danmaku::alphaChanged,alphaS,&QSlider::setValue);
	powerT=new QLabel(this);
	powerT->setText(tr("Danmaku Power"));
	powerL=new QLineEdit(this);
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
	localT->setText(tr("Local Danmaku"));
	localC=new QCheckBox(this);
	connect(localC,&QCheckBox::stateChanged,[this](int state){
		bool local=state==Qt::Checked;
		danmL->clear();
		danmL->setReadOnly(local);
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
	subT->setText(tr("Protect Sub"));
	subC=new QCheckBox(this);
	subC->setChecked(Config::getValue("/Danmaku/Protect",false));
	connect(subC,&QCheckBox::stateChanged,[this](int state){
		Config::setValue("/Danmaku/Protect",state==Qt::Checked);
	});
	loopT=new QLabel(this);
	loopT->setText(tr("Loop Playback"));
	loopC=new QCheckBox(this);
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
			lApp->mainWidget()->setFocus();
		}
	});
	connect(Load::instance(),&Load::stateChanged,[this](int state){
		Load::Task *task=Load::instance()->getHead();
		auto syncDanmL=[&](){
			QString fix(task->code);
			if(!task->code.isEmpty()&&task->processer->regular(fix)){
				danmL->setText(fix);
				danmL->clearFocus();
			}
		};
		switch(state){
		case Load::Page:
			isStay=1;
			syncDanmL();
			break;
		case Load::Part:
			if(isPoped&&animation->state()==QAbstractAnimation::Stopped){
				danmC->complete();
				danmC->popup()->setCurrentIndex(QModelIndex());
			}
		case Load::File:
			localC->setChecked(task->request.url().isLocalFile());
			syncDanmL();
		default:
			isStay=0;
			break;
		}
	});
	connect(APlayer::instance(),&APlayer::mediaChanged,[this](QString _file){
		fileL->setText(QFileInfo(_file).fileName());
		fileL->setCursorPosition(0);
	});
	hide();
}

void Menu::resizeEvent(QResizeEvent *e)
{
	int w=e->size().width(),h=e->size().height();
	double f=font().pointSizeF();
	int x=logicalDpiX()*f/72,y=logicalDpiY()*f/72;
	fileL->setGeometry(QRect(0.83*x,2.08*y,w-6.67*x,2.08*y));
	danmL->setGeometry(QRect(0.83*x,5.42*y,w-6.67*x,2.08*y));
	sechL->setGeometry(QRect(0.83*x,8.75*y,w-6.67*x,2.08*y));
	fileB->setGeometry(QRect(w-5.42*x,2.08*y,4.58*x,2.08*y));
	danmB->setGeometry(QRect(w-5.42*x,5.42*y,4.58*x,2.08*y));
	sechB->setGeometry(QRect(w-5.42*x,8.75*y,4.58*x,2.08*y));
	alphaT->setGeometry(QRect(0.83*x,12.08*y,w-1.67*x,2.08*y));
	alphaS->setGeometry(QRect(0.83*x,14.17*y,w-1.67*x,1.25*y));
	powerT->setGeometry(QRect(0.83*x,17.08*y,w-1.67*x,1.67*y));
	powerL->setGeometry(QRect(w-3.33*x,17.08*y,2.50*x,1.67*y));
	localT->setGeometry(QRect(0.83*x,20.00*y,w-1.67*x,2.08*y));
	localC->setGeometry(QRect(w-7-2.08*x,20.00*y,15,2.08*y));
	subT->setGeometry(QRect(0.83*x,22.92*y,w-1.67*x,2.08*y));
	subC->setGeometry(QRect(w-7-2.08*x,22.92*y,15,2.08*y));
	loopT->setGeometry(QRect(0.83*x,25.83*y,w-1.67*x,2.08*y));
	loopC->setGeometry(QRect(w-7-2.08*x,25.83*y,15,2.08*y));
	Q_UNUSED(h);
	QWidget::resizeEvent(e);
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
		return 0;
	case QEvent::FocusOut:
		isStay=0;
		return 0;
	default:
		return 0;
	}
}

void Menu::pop()
{
	if(!isPoped&&animation->state()==QAbstractAnimation::Stopped){
		show();
		animation->setStartValue(pos());
		animation->setEndValue(pos()+QPoint(width(),0));
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
		animation->setEndValue(pos()-QPoint(width(),0));
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
