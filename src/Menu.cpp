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

Menu::Menu(QWidget *parent) :
	QWidget(parent)
{
	isPop=false;
	isTurn=false;
	isLocal=false;
	lastPath=QDir::homePath();
	setAutoFillBackground(true);
	Utils::setBack(this,Qt::white);
	animation=new QPropertyAnimation(this,"pos",this);
	animation->setDuration(200);
	animation->setEasingCurve(QEasingCurve::OutCubic);
	fileL=new QLineEdit(this);
	danmL=new QLineEdit(this);
	sechL=new QLineEdit(this);
	fileL->setReadOnly(true);
	danmL->setText("av");
	fileL->setGeometry(QRect(10,25, 120,25));
	danmL->setGeometry(QRect(10,65, 120,25));
	sechL->setGeometry(QRect(10,105,120,25));
	connect(danmL,&QLineEdit::textEdited,[this](QString text){
		QRegExp regex("([0-9]+)(#)?([0-9]+)?");
		regex.indexIn(text);
		danmL->setText("av"+regex.cap());
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
		QString _file=QFileDialog::getOpenFileName(parentWidget(),tr("Open File"),lastPath);
		if(!_file.isEmpty()){
			setFile(_file);
		}
	});
	connect(danmA,&QAction::triggered,[this](){
		if(isLocal){
			QString _file=QFileDialog::getOpenFileName(parentWidget(),tr("Open File"),lastPath,tr("XML files (*.xml)"));
			if(!_file.isEmpty()){
				setDm(_file);
			}
		}
		else{
			QString _danm=danmL->text();
			emit load(_danm);
		}
	});
	connect(sechA,&QAction::triggered,[this](){
		Search searchBox;
		if(!sechL->text().isEmpty()){
			searchBox.setKey(sechL->text());
		}
		if(searchBox.exec()) {
			QString aid("av"+searchBox.getAid());
			danmL->setText(aid);
			emit load(aid);
		}
		sechL->setText(searchBox.getKey());
	});
	fileA->setShortcut(QKeySequence("Ctrl+O"));
	danmA->setShortcut(QKeySequence("Ctrl+L"));
	sechA->setShortcut(QKeySequence("Ctrl+S"));
	this->addAction(fileA);
	this->addAction(danmA);
	this->addAction(sechA);
	connect(fileB,&QPushButton::clicked,fileA,&QAction::trigger);
	connect(danmB,&QPushButton::clicked,danmA,&QAction::trigger);
	connect(sechB,&QPushButton::clicked,sechA,&QAction::trigger);
	alphaT=new QLabel(this);
	alphaT->setGeometry(QRect(10,145,100,25));
	alphaT->setText(tr("Danmaku Alpha"));
	alphaS=new QSlider(this);
	alphaS->setOrientation(Qt::Horizontal);
	alphaS->setGeometry(QRect(10,170,180,15));
	alphaS->setRange(0,100);
	alphaS->setValue(100);
	connect(alphaS,&QSlider::valueChanged,[this](int _alpha){emit alpha(_alpha/100.0);});
	delayT=new QLabel(this);
	delayT->setGeometry(QRect(10,205,100,20));
	delayT->setText(tr("Danmaku Delay"));
	delayL=new QLineEdit(this);
	delayL->setGeometry(QRect(160,205,30,20));
	connect(delayL,&QLineEdit::textEdited,[this](QString text){
		QRegExp regex("([0-9]+)");
		regex.indexIn(text);
		delayL->setText(regex.cap());
	});
	connect(delayL,&QLineEdit::editingFinished,[this](){
		emit delay(delayL->text().toInt()*1000);
		if(delayL->text()=="0"){
			delayL->setText("");
		}
	});
	powerT=new QLabel(this);
	powerT->setGeometry(QRect(10,240,100,20));
	powerT->setText(tr("Danmaku Power"));
	powerL=new QLineEdit(this);
	powerL->setGeometry(QRect(160,240,30,20));
	connect(powerL,&QLineEdit::textEdited,[this](QString text){
		QRegExp regex("([0-9]+)");
		regex.indexIn(text);
		powerL->setText(regex.cap());
	});
	connect(powerL,&QLineEdit::editingFinished,[this](){
		int fps=powerL->text().toInt();
		if(fps==0){
			powerL->setText("");
		}
		else{
			if(fps>200){
				fps=200;
				powerL->setText(QString::number(fps));
			}
			if(fps<30){
				fps=30;
				powerL->setText(QString::number(fps));
			}
			powerL->setText(QString::number(fps));
		}
		emit power(fps==0?-1:1000/fps);
	});
	localT=new QLabel(this);
	localT->setGeometry(QRect(10,275,100,25));
	localT->setText(tr("Local XML File"));
	localC=new QCheckBox(this);
	localC->setGeometry(QRect(168,275,25,25));
	connect(localC,&QCheckBox::stateChanged,[this](int state){
		if(state==Qt::Checked){
			danmL->setText("");
			danmL->setReadOnly(true);
			danmB->setText(tr("Open"));
			sechL->setText("");
			sechL->setEnabled(false);
			sechB->setEnabled(false);
			isLocal=true;
		}
		else{
			danmL->setText("av");
			danmL->setReadOnly(false);
			danmB->setText(tr("Load"));
			sechL->setEnabled(true);
			sechB->setEnabled(true);
			isLocal=false;
		}
	});
	subT=new QLabel(this);
	subT->setGeometry(QRect(10,310,100,25));
	subT->setText(tr("Protect Sub"));
	subC=new QCheckBox(this);
	subC->setGeometry(QRect(168,310,25,25));
	connect(subC,&QCheckBox::stateChanged,[this](int state){
		if(state==Qt::Checked){
			emit protect(true);
		}
		else{
			emit protect(false);
		}
	});
	fontT=new QLabel(this);
	fontT->setText(tr("Font"));
	fontT->setGeometry(QRect(10,345,50,25));
	fontC=new QComboBox(this);
	fontC->setGeometry(QRect(100,345,90,25));
	fontC->addItems(QFontDatabase().families());
	connect(fontC,&QComboBox::currentTextChanged,[this](QString _font){
		emit dfont(_font);
	});
#ifdef Q_OS_LINUX
	fontC->setCurrentText("文泉驿正黑");
#endif
#ifdef Q_OS_WIN
	fontC->setCurrentText("黑体");
#endif
	Utils::delayExec(this,0,[this](){
		QJsonObject menu=Utils::getConfig("Menu");
		if(!menu.isEmpty()){
			if(menu.contains("Alpha")){
				alphaS->setValue(menu["Alpha"].toDouble());
			}
			if(menu.contains("Power")){
				int _power=menu["Power"].toDouble();
				powerL->setText(_power==0?"":QString::number(_power));
				emit power(_power==0?-1:1000/_power);
			}
			if(menu.contains("Local")){
				localC->setChecked(menu["Local"].toBool());
			}
			if(menu.contains("Sub")){
				subC->setChecked(menu["Sub"].toBool());
			}
			if(menu.contains("Font")){
				fontC->setCurrentText(menu["Font"].toString());
			}
			if(menu.contains("Path")){
				lastPath=menu["Path"].toString();
			}
		}
	});
	if(QApplication::arguments().count()>=2){
		Utils::delayExec(this,0,[this](){
			setFile(QApplication::arguments()[1]);
		});
	}
}

Menu::~Menu()
{
	QJsonObject menu;
	menu["Alpha"]=alphaS->value();
	menu["Power"]=powerL->text().toInt();
	menu["Local"]=localC->checkState()==Qt::Checked;
	menu["Sub"]=subC->checkState()==Qt::Checked;
	menu["Font"]=fontC->currentText();
	menu["Path"]=lastPath;
	Utils::setConfig(menu,"Menu",true);
}

void Menu::pop()
{
	if(!isPop&&animation->state()==QAbstractAnimation::Stopped){
		animation->setStartValue(pos());
		animation->setEndValue(pos()+QPoint(200,0));
		animation->start();
		isPop=true;
	}
}

void Menu::push()
{
	if(isPop&&animation->state()==QAbstractAnimation::Stopped){
		animation->setStartValue(pos());
		animation->setEndValue(pos()-QPoint(200,0));
		animation->start();
		isPop=false;
		parentWidget()->setFocus();
	}
}

void Menu::setDm(QString _file)
{
	if(isLocal){
		danmL->setText(_file);
		emit load(_file);
	}
}

void Menu::setFile(QString _file)
{
	lastPath=_file.mid(0,_file.lastIndexOf("/"));
#ifdef Q_OS_WIN
	_file.replace('/','\\');
#endif
	fileL->setText(_file);
	emit open(_file);
}

void Menu::setDelay(qint64 _delay)
{
	_delay=_delay<=0?0:_delay;
	delayL->setText(_delay/1000==0?QString():QString::number(_delay/1000));
	emit delay(_delay);
}
