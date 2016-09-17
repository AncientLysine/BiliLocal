/*=======================================================================
*
*   Copyright (C) 2013-2016 Lysine.
*
*   Filename:    Type.cpp
*   Time:        2015/04/26
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

#include "Common.h"
#include "Type.h"
#include "../Interface.h"
#include "../../Local.h"
#include "../../Access/Post.h"
#include "../../Define/Record.h"
#include "../../Model/Danmaku.h"
#include "../../Player/APlayer.h"

using namespace UI;

namespace
{
	QList<const Record *> getRecords()
	{
		QList<const Record *> list;
		for (const Record &r : lApp->findObject<Danmaku>()->getPool()){
			if (lApp->findObject<Post>()->canPost(r.access)){
				list.append(&r);
			}
		}
		return list;
	}
}

Type::Type(QWidget *parent) :
QWidget(parent)
{
	setObjectName("Type");
	setFixedSize(parent->minimumWidth(), 25);

	auto layout = new QHBoxLayout(this);
	layout->setMargin(0);
	layout->setSpacing(0);
	commentM = new QComboBox(this);
	commentM->addItems({ tr("Top"), tr("Slide"), tr("Bottom") });
	commentM->setCurrentIndex(1);
	commentM->setFixedWidth(commentM->sizeHint().width());
	layout->addWidget(commentM);
	commentC = new QPushButton(this);
	commentC->setFixedWidth(25);
	setColor(Qt::white);
	connect(commentC, &QPushButton::clicked, [this](){
		QColor color = QColorDialog::getColor(getColor(), lApp->findObject<Interface>()->widget());
		if (color.isValid()){
			setColor(color);
		}
	});
	layout->addWidget(commentC);
	commentL = new QLineEdit(this);
	commentL->setFocus();
	layout->addWidget(commentL);
	commentS = new QComboBox(this);
	layout->addWidget(commentS);
	commentB = new QPushButton(tr("Post"), this);
	commentB->setDefault(true);
	commentB->setFixedWidth(55);
	commentB->setToolTip(tr("DA☆ZE!"));
	layout->addWidget(commentB);
	commentA = new QAction(this);
	commentA->setShortcut(QKeySequence("Ctrl+Enter"));
	connect(commentL, &QLineEdit::returnPressed, commentA, &QAction::trigger);
	connect(commentB, &QPushButton::clicked,     commentA, &QAction::trigger);
	connect(commentA, &QAction::triggered, [this](){
		if (!commentL->text().isEmpty()){
			auto r = (const Record *)commentS->currentData().value<quintptr>();
			auto c = getComment();
			lApp->findObject<Post>()->postComment(r, &c);
		}
	});
	connect(lApp->findObject<Post>(), &Post::stateChanged, [this](int code){
		switch (code){
		case Post::None:
			setEnabled(1);
			hide();
			commentL->clear();
			break;
		case Post::Code:
			setEnabled(0);
			break;
		default:
			setEnabled(1);
			break;
		}
	});
	connect(lApp->findObject<Danmaku>(), &Danmaku::modelReset, [this](){
		commentS->clear();
		int w = 0;
		for (const Record  *r : getRecords()){
			commentS->addItem(r->string, (quintptr)r);
			w = qMax(w, commentS->fontMetrics().width(r->string));
		}
		commentS->setVisible(commentS->count() >= 2);
		commentS->setFixedWidth(w + 30);
	});
	hide();
}

void Type::setVisible(bool visible)
{
	QWidget::setVisible(visible);
	QWidget *fw = visible ? commentL : lApp->findObject<Interface>()->widget();
	if (fw) {
		fw->setFocus();
	}
}

QColor Type::getColor()
{
	QString sheet = commentC->styleSheet();
	return QColor(sheet.mid(sheet.indexOf('#')));
}

void Type::setColor(QColor color)
{
	commentC->setStyleSheet(QString("background-color:%1").arg(color.name()));
}

namespace
{
	int translateMode(int i)
	{
		switch (i){
		case 0:
			return 5;
		case 1:
			return 1;
		case 2:
			return 4;
		default:
			return 0;
		}
	}
}

Comment Type::getComment()
{
	Comment c;
	c.mode = translateMode(commentM->currentIndex());
	c.font = 25;
	c.time = qMax<qint64>(0, lApp->findObject<APlayer>()->getTime());
	c.color = getColor().rgb() & 0xFFFFFF;
	c.string = commentL->text();
	return c;
}
