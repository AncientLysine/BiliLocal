/*=======================================================================
*
*   Copyright (C) 2013-2015 Lysine.
*
*   Filename:    Info.cpp
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

#include "Common.h"
#include "Info.h"
#include "Editor.h"
#include "Prefer.h"
#include "WidgetInterfacePrivate.h"
#include "../Interface.h"
#include "../../Config.h"
#include "../../Local.h"
#include "../../Utils.h"
#include "../../Access/Load.h"
#include "../../Player/APlayer.h"
#include "../../Model/Danmaku.h"
#include "../../Model/List.h"
#include "../../Model/Shield.h"

using namespace UI;

namespace
{
	class MTableView :public QTableView
	{
	public:
		MTableView(QWidget *parent = 0) :
			QTableView(parent)
		{
		}

		void currentChanged(const QModelIndex &c,
			const QModelIndex &p)
		{
			QTableView::currentChanged(c, p);
			selectionModel()->setCurrentIndex(QModelIndex(), QItemSelectionModel::NoUpdate);
		}
	};
}

Info::Info(QWidget *parent) :
QWidget(parent)
{
	setObjectName("Info");
	isStay = isPoped = updating = false;
	Utils::setGround(this, Qt::white);
	duration = -1;
	animation = new QPropertyAnimation(this, "pos", this);
	animation->setDuration(200);
	animation->setEasingCurve(QEasingCurve::OutCubic);
	timeT = new QLabel(tr("Time"), this);
	volmT = new QLabel(tr("Volume"), this);
	timeS = new QSlider(this);
	volmS = new QSlider(this);
	timeS->setOrientation(Qt::Horizontal);
	volmS->setOrientation(Qt::Horizontal);
	timeS->setRange(0, 0);
	volmS->setRange(0, 100);
	timeS->setValue(0);
	volmS->setValue(Config::getValue("/Player/Volume", 50));
	timeS->setTracking(false);
	connect(timeS, &QSlider::valueChanged, [this](int _time){
		if (duration != -1 && !updating){
			lApp->findObject<APlayer>()->setTime(duration*_time / 400);
		}
	});
	connect(volmS, &QSlider::sliderMoved, [this](int _volm){
		QPoint p;
		p.setX(QCursor::pos().x());
		p.setY(volmS->mapToGlobal(volmS->rect().center()).y());
		QToolTip::showText(p, QString::number(_volm));
	});
	connect(volmS, &QSlider::valueChanged, [this](int _volm){
		if (!updating){
			lApp->findObject<APlayer>()->setVolume(_volm);
		}
	});
	playB = new QPushButton(this);
	stopB = new QPushButton(this);
	playI = QIcon::fromTheme("media-playback-start", QIcon(":/Picture/play.png"));
	stopI = QIcon::fromTheme("media-playback-stop", QIcon(":/Picture/stop.png"));
	pausI = QIcon::fromTheme("media-playback-pause", QIcon(":/Picture/pause.png"));
	playB->setIcon(playI);
	stopB->setIcon(stopI);
	playA = new QAction(playI, tr("Play"), this);
	stopA = new QAction(stopI, tr("Stop"), this);
	playA->setObjectName("Play");
	stopA->setObjectName("Stop");
	QList<QKeySequence> playS;
	playS << Config::getValue("/Shortcut/Play", QString("Space")) <<
		Qt::Key_MediaPlay <<
		Qt::Key_MediaPause <<
		Qt::Key_MediaTogglePlayPause;
	playA->setShortcuts(playS);
	stopA->setShortcut(Config::getValue("/Shortcut/Stop", QString()));
	addAction(playA);
	addAction(stopA);
	connect(playA, SIGNAL(triggered()), lApp->findObject<APlayer>(), SLOT(play()));
	connect(stopA, SIGNAL(triggered()), lApp->findObject<APlayer>(), SLOT(stop()));
	connect(playB, &QPushButton::clicked, playA, &QAction::trigger);
	connect(stopB, &QPushButton::clicked, stopA, &QAction::trigger);
	duraT = new QLabel(this);
	duraT->setAlignment(Qt::AlignRight | Qt::AlignBottom);
	duraT->setText("00:00/00:00");
	danmV = new MTableView(this);
	danmV->setWordWrap(false);
	danmV->setSelectionBehavior(QAbstractItemView::SelectRows);
	danmV->setSelectionMode(QAbstractItemView::ExtendedSelection);
	danmV->verticalHeader()->hide();
	danmV->setAlternatingRowColors(true);
	danmV->setContextMenuPolicy(Qt::CustomContextMenu);
	danmV->setModel(lApp->findObject<Danmaku>());
	QHeaderView *header;
	header = danmV->horizontalHeader();
	header->setSectionResizeMode(0, QHeaderView::Fixed);
	header->setSectionResizeMode(1, QHeaderView::Stretch);
	header->setHighlightSections(false);
	resizeHeader();
	header = danmV->verticalHeader();
	header->setDefaultSectionSize(22.5*logicalDpiY() / 72);
	connect(lApp->findObject<Danmaku>(), &Danmaku::layoutChanged, this, &Info::resizeHeader);
	connect(danmV, &QTableView::doubleClicked, [this](QModelIndex index){
		lApp->findObject<APlayer>()->setTime(index.data(Danmaku::TimeRole).value<qint64>());
	});

	QAction *saveA = new QAction(tr("Save Danmaku to File"), this);
	saveA->setObjectName("Save");
	saveA->setShortcut(Config::getValue("/Shortcut/Save", QString()));
	saveA->setEnabled(false);
	connect(saveA, &QAction::triggered, [](){
		QFileDialog save(lApp->findObject<Interface>()->widget(), tr("Save File"));
		save.setAcceptMode(QFileDialog::AcceptSave);
		QFileInfo info(lApp->findObject<APlayer>()->getMedia());
		if (info.isFile()){
			save.setDirectory(info.absolutePath());
			save.selectFile(info.completeBaseName());
		}
		else{
			save.setDirectory(lApp->findObject<List>()->defaultPath(Utils::Danmaku));
		}
		save.setDefaultSuffix("json");
		QStringList type;
		type << tr("Bilibili Danmaku Format (*.xml)") << tr("AcFun Danmaku Format (*.json)");
		save.setNameFilters(type);
		save.connect(&save, &QFileDialog::filterSelected, [&](QString filter){
			save.setDefaultSuffix(filter.indexOf("xml") == -1 ? "json" : "xml");
		});
		if (save.exec() == QDialog::Accepted){
			QStringList file = save.selectedFiles();
			if (file.size() == 1){
				lApp->findObject<Danmaku>()->saveToFile(file.first());
			}
		}
	});
	danmV->addAction(saveA);

	QAction *fullA = new QAction(tr("Full Danmaku"), this);
	fullA->setObjectName("Char");
	fullA->setShortcut(Config::getValue("/Shortcut/Char", QString()));
	fullA->setEnabled(false);
	connect(fullA, &QAction::triggered, [](){
		Load *load = lApp->findObject<Load>();
		for (const Record &r : lApp->findObject<Danmaku>()->getPool()){
			if (load->canFull(&r)){
				load->fullDanmaku(&r);
			}
		}
	});
	danmV->addAction(fullA);

	connect(lApp->findObject<Danmaku>(), &Danmaku::modelReset, [=](){
		const QList<Record> &pool = lApp->findObject<Danmaku>()->getPool();
		fullA->setEnabled(false);
		for (const Record &r : pool){
			if (lApp->findObject<Load>()->canFull(&r)){
				fullA->setEnabled(true);
				break;
			}
		}
		saveA->setEnabled(!pool.isEmpty());
	});

	connect(danmV, &QTableView::customContextMenuRequested, [=](QPoint p){
		QMenu menu(this);
		auto selected = danmV->selectionModel()->selectedRows();
		if (!selected.isEmpty()){
			connect(menu.addAction(tr("Copy Danmaku")), &QAction::triggered, [=](){
				QStringList list;
				for (const QModelIndex &iter : selected) {
					QString string = iter.data(Danmaku::StringRole).toString();
					list.append(string);
				}
				qApp->clipboard()->setText(list.join('\n'));
			});
			
			QStringList ruleList;
			for (const QModelIndex &iter : selected) {
				QString sender = iter.data(Danmaku::SenderRole).toString();
				if (sender.isEmpty()) {
					continue;
				}
				ruleList.append("u=" + sender);
			}
			auto shield = lApp->findObject<Shield>();
			if (!ruleList.isEmpty()){
				connect(menu.addAction(tr("Eliminate The Sender")), &QAction::triggered, [=]() {
					for (const QString &iter : ruleList) {
						shield->insert(iter);
					}
					lApp->findObject<Danmaku>()->parse(Danmaku::Block);
				});
			}
			bool inList = false;
			for (const QString &iter : ruleList) {
				if (shield->contains(iter)) {
					inList = true;
					break;
				}
			}
			if (inList) {
				connect(menu.addAction(tr("Recover The Sender")), &QAction::triggered, [=]() {
					for (const QString &iter : ruleList) {
						shield->remove(iter);
					}
					lApp->findObject<Danmaku>()->parse(Danmaku::Block);
				});
			}
			menu.addSeparator();
		}
		menu.addAction(fullA);
		connect(menu.addAction(tr("Edit Blocking List")), &QAction::triggered, [this](){
			Prefer::exec(lApp->findObject<Interface>()->widget(), 3);
		});
		connect(menu.addAction(tr("Edit Danmaku Pool")), &QAction::triggered, [this](){
			Editor::exec(lApp->findObject<Interface>()->widget(), 2);
		});
		connect(menu.addAction(tr("Clear Danmaku Pool")), &QAction::triggered, lApp->findObject<Danmaku>(), &Danmaku::clear);
		menu.addAction(saveA);
		isStay = 1;
		menu.exec(danmV->viewport()->mapToGlobal(p));
		isStay = 0;
	});

	animation = new QPropertyAnimation(this, "pos", this);
	animation->setDuration(200);
	animation->setEasingCurve(QEasingCurve::OutCubic);
	connect(animation, &QPropertyAnimation::finished, [this](){
		if (!isPoped){
			hide();
			lApp->findObject<Interface>()->widget()->setFocus();
		}
	});

	connect(lApp->findObject<APlayer>(), &APlayer::timeChanged, this, &Info::setTime);
	connect(lApp->findObject<APlayer>(), &APlayer::volumeChanged, [this](int volume){
		updating = 1;
		volmS->setValue(volume);
		updating = 0;
	});
	connect(lApp->findObject<APlayer>(), &APlayer::begin, [this](){
		setDuration(lApp->findObject<APlayer>()->getDuration());
	});
	connect(lApp->findObject<APlayer>(), &APlayer::reach, [this](){
		setDuration(-1);
	});
	connect(lApp->findObject<APlayer>(), &APlayer::stateChanged, [this](int state){
		bool playing = state == APlayer::Play;
		playB->setIcon(playing ? pausI : playI);
		playA->setIcon(playing ? pausI : playI);
		playA->setText(playing ? tr("Pause") : tr("Play"));
	});
	hide();
}

void Info::pop()
{
	if (!isPoped&&animation->state() == QAbstractAnimation::Stopped){
		show();
		animation->setStartValue(pos());
		animation->setEndValue(pos() - QPoint(width(), 0));
		animation->start();
		isPoped = true;
	}
}

void Info::push(bool force)
{
	if (isPoped&&animation->state() == QAbstractAnimation::Stopped && (!preferStay() || force)){
		if (force){
			isStay = false;
		}
		animation->setStartValue(pos());
		animation->setEndValue(pos() + QPoint(width(), 0));
		animation->start();
		isPoped = false;
	}
}

void Info::terminate()
{
	if (animation->state() != QAbstractAnimation::Stopped){
		animation->setCurrentTime(animation->totalDuration());
	}
}

void Info::resizeEvent(QResizeEvent *e)
{
	int w = e->size().width(), h = e->size().height();
	double f = font().pointSizeF();
	int x = logicalDpiX()*f / 72, y = logicalDpiY()*f / 72;
	playB->setGeometry(QRect(0.83*x, 1.25*y, 2.08*x, 2.08*y));
	stopB->setGeometry(QRect(3.33*x, 1.25*y, 2.08*x, 2.08*y));
	duraT->setGeometry(QRect(5.83*x, 1.25*y, w - 6.67*x, 2.08*y));
	timeT->setGeometry(QRect(0.83*x, 4.17*y, w - 1.67*x, 2.08*y));
	timeS->setGeometry(QRect(0.83*x, 6.25*y, w - 1.67*x, 1.25*y));
	volmT->setGeometry(QRect(0.83*x, 8.33*y, w - 1.67*x, 2.08*y));
	volmS->setGeometry(QRect(0.83*x, 10.42*y, w - 1.67*x, 1.25*y));
	danmV->setGeometry(QRect(0.83*x, 14.17*y, w - 1.67*x, h - 15.00*y));
	QWidget::resizeEvent(e);
}

void Info::resizeHeader()
{
	Danmaku *d = lApp->findObject<Danmaku>();
	QStringList list;
	list.append(d->headerData(0, Qt::Horizontal, Qt::DisplayRole).toString());
	int c = d->rowCount() - 1, i;
	for (i = 0; i <= c; ++i){
		if (d->data(d->index(i, 0), Qt::ForegroundRole).value<QColor>() != Qt::red){
			list.append(d->data(d->index(i, 0), Qt::DisplayRole).toString());
			break;
		}
	}
	for (i = c; i >= 0; --i){
		if (d->data(d->index(i, 0), Qt::ForegroundRole).value<QColor>() != Qt::red){
			list.append(d->data(d->index(i, 0), Qt::DisplayRole).toString());
			break;
		}
	}
	int m = 0;
	for (QString item : list){
		m = qMax(m, danmV->fontMetrics().width(item) + 8);
	}
	danmV->horizontalHeader()->resizeSection(0, m);
}

void Info::setTime(qint64 _time)
{
	if (!timeS->isSliderDown()){
		updating = 1;
		timeS->setValue(_time * 400 / duration);
		updating = 0;
	}
	int c = _time / 1000;
	int s = duration / 1000;
	auto to = [](int num){
		QString res = QString::number(num);
		res.prepend(QString(2 - res.length(), '0'));
		return res;
	};
	QString t;
	t += to(c / 60) + ':' + to(c % 60);
	t += '/';
	t += to(s / 60) + ':' + to(s % 60);
	duraT->setText(t);
}

void Info::setDuration(qint64 _duration)
{
	if (_duration > 0){
		duration = _duration;
		timeS->setRange(0, 400);
	}
	else{
		duration = -1;
		timeS->setValue(0);
		timeS->setRange(0, 0);
		duraT->setText("00:00/00:00");
	}
}
