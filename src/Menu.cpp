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
	QPalette options;
	options.setColor(QPalette::Background,Qt::white);
	setPalette(options);
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
	connect(fileB,&QPushButton::clicked,[this](){
		QWidget *p=dynamic_cast<QWidget *>(this->parent());
		QString _file=QFileDialog::getOpenFileName(p,tr("Open File"),lastPath);
		if(!_file.isEmpty()){
			setFile(_file);
		}
	});
	connect(danmB,&QPushButton::clicked,[this](){
		if(isLocal){
			QWidget *p=dynamic_cast<QWidget *>(this->parent());
			QString _file=QFileDialog::getOpenFileName(p,tr("Open File"),lastPath,tr("XML files (*.xml)"));
			if(!_file.isEmpty()){
				setDm(_file);
			}
		}
		else{
			QString _danm=danmL->text();
			emit load(_danm);
		}
	});
	connect(sechB,&QPushButton::clicked,[this](){
		Search searchBox;
		if(!sechL->text().isEmpty()){
			searchBox.setKey(sechL->text());
		}
		if(searchBox.exec()) {
			QString aid("av"+searchBox.getAid());
			danmL->setText(aid);
			sechL->setText(searchBox.getKey());
			emit load(aid);
		}
	});
	alphaT=new QLabel(this);
	alphaT->setGeometry(QRect(10,140,100,25));
	alphaT->setText(tr("Danmaku Alpha"));
	alphaS=new QSlider(this);
	alphaS->setOrientation(Qt::Horizontal);
	alphaS->setGeometry(QRect(10,165,180,15));
	alphaS->setRange(0,100);
	alphaS->setValue(100);
	connect(alphaS,&QSlider::valueChanged,[this](int _alpha){emit alpha(_alpha/100.0);});
	delayT=new QLabel(this);
	delayT->setGeometry(QRect(10,200,100,20));
	delayT->setText(tr("Danmaku Delay"));
	delayL=new QLineEdit(this);
	delayL->setGeometry(QRect(160,200,30,20));
	connect(delayL,&QLineEdit::textEdited,[this](QString text){
		QRegExp regex("([0-9]+)");
		regex.indexIn(text);
		delayL->setText(regex.cap());
	});
	connect(delayL,&QLineEdit::editingFinished,[this](){
		emit delay(delayL->text().toInt()*1000);
	});
	powerT=new QLabel(this);
	powerT->setGeometry(QRect(10,235,100,20));
	powerT->setText(tr("Danmaku Power"));
	powerL=new QLineEdit(this);
	powerL->setGeometry(QRect(160,235,30,20));
	connect(powerL,&QLineEdit::textEdited,[this](QString text){
		QRegExp regex("([0-9]+)");
		regex.indexIn(text);
		powerL->setText(regex.cap());
	});
	connect(powerL,&QLineEdit::editingFinished,[this](){
		quint8 fps=powerL->text().toInt();
		if(fps==0){
			powerL->setText("");
		}
		else if(fps>200){
			fps=200;
			powerL->setText(QString::number(fps));
		}
		else if(fps<40){
			fps=40;
			powerL->setText(QString::number(fps));
		}
		emit power(fps==0?-1:1000/fps);
	});
	localT=new QLabel(this);
	localT->setGeometry(QRect(10,270,100,25));
	localT->setText(tr("Local XML File"));
	localC=new QCheckBox(this);
	localC->setGeometry(QRect(168,270,25,25));
	connect(localC,&QCheckBox::stateChanged,[this](int state){
		if(state==Qt::Checked){
			danmL->setText("");
			danmL->setReadOnly(true);
			danmB->setText(tr("Open"));
			sechB->setEnabled(false);
			isLocal=true;
		}
		else{
			danmL->setText("av");
			danmL->setReadOnly(false);
			danmB->setText(tr("Load"));
			sechB->setEnabled(true);
			isLocal=false;
		}
	});
	subT=new QLabel(this);
	subT->setGeometry(QRect(10,305,100,25));
	subT->setText(tr("Protect Sub"));
	subC=new QCheckBox(this);
	subC->setGeometry(QRect(168,305,25,25));
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
	fontT->setGeometry(QRect(10,340,50,25));
	fontC=new QComboBox(this);
	fontC->setGeometry(QRect(100,340,90,25));
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
	QTimer *delay=new QTimer(this);
	delay->setSingleShot(true);
	delay->start(0);
	connect(delay,&QTimer::timeout,[this](){
		QFile option("./Option.txt");
		if(!option.exists()){
			return;
		}
		option.open(QIODevice::ReadOnly|QIODevice::Text);
		QTextStream stream(&option);
		QString line;
		while(!stream.atEnd()){
			line=stream.readLine();
			QString argument=line.mid(10).simplified();
			if(line.indexOf("[Alpha] =")!=-1){
				int _alpha=argument.toInt();
				alphaS->setValue(_alpha);
			}
			if(line.indexOf("[Power] =")!=-1){
				int _power=argument.toInt();
				powerL->setText(_power==0?"":argument);
				emit power(_power==0?-1:1000/_power);
			}
			if(line.indexOf("[Local] =")!=-1){
				localC->setChecked(argument!="0");
			}
			if(line.indexOf("[Sub]	 =")!=-1){
				subC->setChecked(argument!="0");
			}
			if(line.indexOf("[Font]	 =")!=-1){
				fontC->setCurrentText(argument);
			}
			if(line.indexOf("[Path]	 =")!=-1){
				if(!argument.isEmpty()){
					lastPath=argument;
				}
			}
		}
		option.close();
	});

	auto fileSC=new QShortcut(this);
	fileSC->setKey(QString("Ctrl+O"));
	connect(fileSC,SIGNAL(activated()),fileB,SIGNAL(clicked()));

	auto danmSC=new QShortcut(this);
	danmSC->setKey(QString("Ctrl+L"));
	connect(danmSC,SIGNAL(activated()),danmB,SIGNAL(clicked()));

	auto sechSC=new QShortcut(this);
	sechSC->setKey(QString("Ctrl+S"));
	connect(sechSC,SIGNAL(activated()),sechB,SIGNAL(clicked()));
}

Menu::~Menu()
{
	QFile option("./Option.txt");
	option.open(QIODevice::WriteOnly|QIODevice::Text);
	QTextStream stream(&option);
	stream<<"[Alpha] = "<<alphaS->value()<<endl<<
			"[Power] = "<<powerL->text().toInt()<<endl<<
			"[Local] = "<<(localC->checkState()==Qt::Checked)<<endl<<
			"[Sub]	 = "<<(subC->checkState()==Qt::Checked)<<endl<<
			"[Font]	 = "<<fontC->currentText()<<endl<<
			"[Path]	 = "<<lastPath<<endl;
	option.close();
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
