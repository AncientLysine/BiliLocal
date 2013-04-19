/*=======================================================================
*
*   Copyright (C) 2013 Lysine.
*
*   Filename:    Search.cpp
*   Time:        2013/04/18
*   Author:      Chaserhkj
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

#include "Search.h"

Search::Search(QWidget *parent) : QDialog(parent)
{
	auto outerLayout=new QVBoxLayout;
	auto keywordLayout=new QHBoxLayout;
	keywordE=new QLineEdit;
	searchB=new QPushButton;
	searchB->setText(tr("Search"));
	keywordLayout->addWidget(keywordE);
	keywordLayout->addWidget(searchB);
	outerLayout->addLayout(keywordLayout);
	resultW=new QListWidget;
	outerLayout->addWidget(resultW);
	auto pageLayout=new QHBoxLayout;
	pageL=new QLabel;
	pageL->setText(tr("Page"));
	pageE=new QLineEdit;
	pagegotoB=new QPushButton;
	pagegotoB->setText(tr("Goto"));
	pageupB=new QPushButton;
	pageupB->setText(tr("Page up"));
	pagedownB=new QPushButton;
	pagedownB->setText(tr("Page down"));
	pageLayout->addWidget(pageL);
	pageLayout->addWidget(pageE);
	pageLayout->addWidget(pagegotoB);
	pageLayout->addWidget(pageupB);
	pageLayout->addWidget(pagedownB);
	outerLayout->addLayout(pageLayout);
	auto responseLayout=new QHBoxLayout;
	okB=new QPushButton;
	okB->setText(tr("OK"));
	cancelB=new QPushButton;
	cancelB->setText(tr("Cancel"));
	responseLayout->addWidget(okB);
	responseLayout->addWidget(cancelB);
	outerLayout->addLayout(responseLayout);

	this->setLayout(outerLayout);
	this->setWindowTitle(tr("Search"));
	
	connect(okB,&QPushButton::clicked,this,&QDialog::accept);
	connect(cancelB,&QPushButton::clicked,this,&QDialog::reject);
}

Search::~Search()
{
	
}
