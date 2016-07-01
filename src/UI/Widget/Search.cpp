/*=======================================================================
*
*   Copyright (C) 2013-2015 Lysine.
*
*   Filename:    Search.cpp
*   Time:        2013/04/18
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

#include "Common.h"
#include "Search.h"
#include "WidgetUtils.h"
#include "../../Local.h"
#include "../../Access/Seek.h"
#include "../../Utils.h"

using namespace UI;

Search::Search(QWidget *parent) : QDialog(parent)
{
	isWaiting = false;
	pageNum = pageCur = -1;
	resultM = new QStandardItemModel(this);
	double x = logicalDpiX() / 72.0, y = logicalDpiY() / 72.0;
	auto outerLayout = new QVBoxLayout;

	//Head
	auto keywdLayout = new QHBoxLayout;

	keywE = new QLineEdit(this);
	keywdLayout->addWidget(keywE);

	orderC = new QComboBox(this);
	sitesC = new QComboBox(this);
	sitesC->addItems(lApp->findObject<Seek>()->modules());
	sitesC->setEditable(false);
	orderC->setEditable(false);
	orderC->setFixedWidth(60 * x);
	sitesC->setFixedWidth(60 * x);
	connect(orderC, &QComboBox::currentTextChanged, [this](){
		if (!isWaiting && resultM->rowCount()){
			startSearch();
		}
	});
	connect(sitesC, &QComboBox::currentTextChanged, [this](QString site){
		orderC->clear();
		orderC->addItems(lApp->findObject<Seek>()->getProc(site)->sort);
		if (!isWaiting && resultM->rowCount()){
			startSearch();
		}
	});
	sitesC->currentTextChanged(sitesC->currentText());
	keywdLayout->addWidget(orderC);
	keywdLayout->addWidget(sitesC);

	searchB = new QPushButton(this);
	searchB->setText(tr("Search"));
	connect(searchB, &QPushButton::clicked, [this](){
		if (isWaiting){
			QMessageBox::warning(this, tr("Warning"), tr("A request is pending."));
			return;
		}
		clearSearch();
		pageCur = 1;
		startSearch();
	});
	keywdLayout->addWidget(searchB);
	outerLayout->addLayout(keywdLayout);

	//Body
	resultV = new QTreeView(this);
	resultV->setIconSize(QSize(90 * x, 67.5*y));
	resultV->setModel(resultM);
	resultV->setSelectionMode(QAbstractItemView::SingleSelection);
	resultV->setIndentation(0);
	resultV->header()->setStretchLastSection(false);
	connect(resultV, &QTreeView::activated, this, &Search::accept);
	outerLayout->addWidget(resultV);

	//Tail
	auto pageLayout = new QHBoxLayout;
	statusL = new QLabel(tr("Ready"), this);
	pageLayout->addWidget(statusL);
	pageLayout->addStretch();

	pageT = new QLabel(tr("Page"), this);
	pageLayout->addWidget(pageT);

	pageE = new QLineEdit(this);
	pageE->setFixedWidth(30 * x);
	pageLayout->addWidget(pageE);

	pageL = new QLabel(this);
	pageL->setFixedWidth(30 * x);
	pageLayout->addWidget(pageL);

	pageGoB = new QPushButton(tr("Goto"), this);
	pageUpB = new QPushButton(tr("PgUp"), this);
	pageDnB = new QPushButton(tr("PgDn"), this);
	pageLayout->addWidget(pageGoB);
	pageLayout->addWidget(pageUpB);
	pageLayout->addWidget(pageDnB);
	
	auto jump = [this](int page){
		if (pageNum == -1) {
			QMessageBox::warning(this, tr("Warning"), tr("No search in progress."));
		}
		else if (isWaiting) {
			QMessageBox::warning(this, tr("Warning"), tr("A request is pending."));
		}
		else if (page < 1 || page > pageNum) {
			QMessageBox::warning(this, tr("Warning"), tr("Page num out of range."));
		}
		else{
			pageCur = page;
			startSearch();
		}
	};

	connect(pageGoB, &QPushButton::clicked, [jump, this](){jump(pageE->text().toInt()); });
	connect(pageUpB, &QPushButton::clicked, [jump, this](){jump(pageCur - 1); });
	connect(pageDnB, &QPushButton::clicked, [jump, this](){jump(pageCur + 1); });
	outerLayout->addLayout(pageLayout);

	auto responseLayout = new QHBoxLayout;
	okB = new QPushButton(tr("Confirm"), this);
	ccB = new QPushButton(tr("Cancel"), this);
	connect(okB, &QPushButton::clicked, this, &Search::accept);
	connect(ccB, &QPushButton::clicked, this, &Search::reject);
	responseLayout->addWidget(okB);
	responseLayout->addStretch();
	responseLayout->addWidget(ccB);
	outerLayout->addLayout(responseLayout);

	connect(lApp->findObject<Seek>(), &Seek::stateChanged, this, [=](int code){
		switch (code){
		case Seek::List:
			isWaiting = 1;
			statusL->setText(tr("Requesting"));
			break;
		case Seek::None:
			statusL->setText(tr("Finished"));
		case Seek::Data:
			isWaiting = 0;
			resultV->expandAll();
		case Seek::More:
			pageNum = lApp->findObject<Seek>()->getHead()->page.second;
			pageE->setText(QString::number(pageCur));
			pageL->setText(QString("/%1").arg(pageNum));
			resultV->setColumnWidth(0, resultV->iconSize().width() + 6);
			for (int i = 0;; ++i){
				QStandardItem *h = resultM->horizontalHeaderItem(i);
				if (!h){
					break;
				}
				switch (int s = h->data(Qt::UserRole).toInt())
				{
				case -1:
					resultV->setColumnWidth(i, resultV->iconSize().width() + 6);
					break;
				case 0:
					resultV->header()->setSectionResizeMode(i, QHeaderView::Stretch);
					break;
				default:
					resultV->setColumnWidth(i, s * x);
					break;
				}
			}
			break;
		}
	});
	connect(lApp->findObject<Seek>(), &Seek::errorOccured, this, [=](){
		clearSearch();
		isWaiting = false;
	});
	connect(this, &QWidget::destroyed, lApp->findObject<Seek>(), []()
	{
		if (lApp->findObject<Seek>()->getHead()){
			lApp->findObject<Seek>()->dequeue();
		}
	});

	setLayout(outerLayout);
	setWindowTitle(tr("Search"));
	setMinimumSize(450 * x, 300 * y);
	resize(675 * x, 390 * y);
	Utils::setCenter(this);
}

void Search::setKey(QString key)
{
	this->key = key;
	keywE->setText(key);
	clearSearch();
	pageCur = 1;
	startSearch();
}

void Search::startSearch()
{
	Seek::Task task;
	task.text = keywE->text();
	task.code = sitesC->currentText();
	if (0 > (task.sort = orderC->currentIndex())){
		return;
	}
	task.page.first = pageCur == -1 ? 1 : pageCur;
	task.cover = resultV->iconSize();
	task.model = resultM;
	task.processer = lApp->findObject<Seek>()->getProc(task.code);
	lApp->findObject<Seek>()->enqueue(task);
}

void Search::clearSearch()
{
	resultM->clear();
	pageNum = -1;
	pageCur = -1;
	pageE->clear();
	pageL->clear();
	statusL->setText(tr("Ready"));
}

void Search::accept()
{
	QModelIndex i = resultV->currentIndex();
	i = i.sibling(i.row(), 0);
	if (QStandardItem *item = resultM->itemFromIndex(i)){
		aid = item->data(Qt::UserRole).toString();
		if (aid.length()){
			QDialog::accept();
		}
	}
	else{
		QMessageBox::warning(this, tr("Warning"), tr("No video has been chosen."));
	}
}
