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
#include "Load.h"
#include "Utils.h"
#include "Config.h"
#include "Search.h"
#include "VPlayer.h"
#include "Danmaku.h"

Menu::Menu(QWidget *parent):
	QWidget(parent)
{
	isStay=isPoped=false;
	Utils::setGround(this,Qt::white);
	fileL=new QLineEdit(this);
	danmL=new QLineEdit(this);
	sechL=new QLineEdit(this);
	fileL->installEventFilter(this);
	danmL->installEventFilter(this);
	sechL->installEventFilter(this);
	fileL->setReadOnly(true);
	danmL->setPlaceholderText(tr("av/ac/dd"));
	fileL->setGeometry(QRect(10,25, 120,25));
	danmL->setGeometry(QRect(10,65, 120,25));
	sechL->setGeometry(QRect(10,105,120,25));
	connect(danmL,&QLineEdit::textEdited,[this](QString text){
		QRegularExpression regexp("[ad]([cvd]((\\d+)([#_])?(\\d+)?)?)?");
		regexp.setPatternOptions(QRegularExpression::CaseInsensitiveOption);
		auto iter=regexp.globalMatch(text);
		QString match;
		while(iter.hasNext()){
			match=iter.next().captured();
		}
		danmL->setText(match.toLower().replace('_','#'));
	});
	danmC=new QCompleter(Load::instance()->getModel(),this);
	danmC->setCompletionMode(QCompleter::UnfilteredPopupCompletion);
	danmC->setCaseSensitivity(Qt::CaseInsensitive);
	danmC->setWidget(danmL);
	QAbstractItemView *popup=danmC->popup();
	popup->setMouseTracking(true);
	connect(popup,SIGNAL(entered(QModelIndex)),popup,SLOT(setCurrentIndex(QModelIndex)));
	connect<void (QCompleter::*)(const QModelIndex &)>(danmC,&QCompleter::activated,[this](const QModelIndex &index){
		Load::instance()->getReply(QNetworkRequest(index.data(Qt::UserRole).toUrl()),index.data(Qt::UserRole+1).toString());
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
	danmA=new QAction(tr("Load Danmaku"),this);
	sechA=new QAction(tr("Search Danmaku"),this);
	connect(fileA,&QAction::triggered,[this](){
		QString _file=QFileDialog::getOpenFileName(parentWidget(),
												   tr("Open File"),
												   Utils::defaultPath());
		if(!_file.isEmpty()){
			setMedia(_file);
		}
	});
	connect(danmA,&QAction::triggered,[this](){
		if(Config::getValue("/Danmaku/Local",false)){
			QString _file=QFileDialog::getOpenFileName(parentWidget(),
													   tr("Open File"),
													   Utils::defaultPath(),
													   tr("Danmaku files (*.xml *.json)"));
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
		Search searchBox(parentWidget());
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
	powerC=new QTimer(this);
	powerC->setTimerType(Qt::PreciseTimer);
	setPower(Config::getValue("/Danmaku/Power",60));
	connect(powerL,&QLineEdit::editingFinished,[this](){
		int p=powerL->text().toInt();
		setPower(p);
		Config::setValue("/Danmaku/Power",p);
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
		danmL->setPlaceholderText(local?QString():tr("av/ac/dd"));
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
			if(isVisible()){
				danmC->complete();
				danmC->popup()->setCurrentIndex(Load::instance()->getModel()->index(0,0));
			}
		case Load::File:
			danmL->setText(Load::instance()->getString());
			danmL->setCursorPosition(0);
		case Load::Code:
		case Load::None:
			isStay=0;
			break;
		default:
			QMessageBox::warning(parentWidget(),tr("Network Error"),tr("Network error occurred, error code: %1").arg(state));
			isStay=0;
			break;
		}
	});
	connect(VPlayer::instance(),&VPlayer::mediaChanged,[this](QString _file){
		fileL->setText(QFileInfo(_file).fileName());
		fileL->setCursorPosition(0);
	});
	hide();
}

bool Menu::eventFilter(QObject *o,QEvent *e)
{
	if(e->type()==QEvent::ContextMenu){
		isStay=1;
		QMenu *m=dynamic_cast<QLineEdit *>(o)->createStandardContextMenu();
		m->exec(dynamic_cast<QContextMenuEvent *>(e)->globalPos());
		delete m;
		isStay=0;
		return 1;
	}
	else{
		return 0;
	}
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

void Menu::setPower(qint16 fps)
{
	if(fps==0){
		powerC->stop();
		powerL->setText("");
	}
	else{
		fps=qBound<qint16>(30,fps,200);
		powerC->start(1000/fps);
		powerL->setText(QString::number(fps));
	}
}

void Menu::setMedia(QString _file)
{
	QFileInfo file(_file);
	fileL->setText(file.fileName());
	fileL->setCursorPosition(0);
	Config::setValue("/Playing/Path",file.absolutePath());
	VPlayer::instance()->setMedia(file.absoluteFilePath());
	bool only=Config::getValue("/Playing/Clear",true);
	if(Config::getValue("/Danmaku/Local",false)&&(Danmaku::instance()->rowCount()==0||only)){
		for(const QFileInfo &info:file.dir().entryInfoList()){
			QString suffix=info.suffix().toLower();
			if((suffix=="xml"||suffix=="json")&&file.baseName()==info.baseName()){
				Load::instance()->loadDanmaku(info.absoluteFilePath());
				if(only) break;
			}
		}
	}
}

void Menu::tryLocal(QString _file)
{
	QFileInfo info(_file);
	if(info.exists()){
		QString suffix=info.suffix().toLower();
		if(suffix=="xml"||suffix=="json"){
			Load::instance()->loadDanmaku(_file);
		}
		else{
			setMedia(_file);
		}
	}
}
