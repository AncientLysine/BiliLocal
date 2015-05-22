/*=======================================================================
*
*   Copyright (C) 2013-2015 Lysine.
*
*   Filename:    Editor.cpp
*   Time:        2013/06/30
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

#include "Editor.h"
#include "../Config.h"
#include "../Utils.h"
#include "../Access/Load.h"
#include "../Access/NetworkConfiguration.h"
#include "../Model/Danmaku.h"
#include "../Model/List.h"
#include "../Player/APlayer.h"

using namespace UI;

namespace{
	class ListEditor :public QListView
	{
	public:
		explicit ListEditor(QWidget *parent = 0) :
			QListView(parent)
		{
			setModel(List::instance());
			setMinimumWidth(200 * logicalDpiX() / 96);
			setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
			setSelectionMode(ExtendedSelection);
			setDragDropMode(InternalMove);
			setContextMenuPolicy(Qt::ActionsContextMenu);
			setIconSize(QSize(15 * logicalDpiX() / 96, 15 * logicalDpiY() / 96));
			setAlternatingRowColors(true);

			megA = new QAction(Editor::tr("Merge"), this);
			connect(megA, &QAction::triggered, [this](){
				List::instance()->merge(selectionModel()->selectedRows());
				setCurrentIndex(QModelIndex());
			});
			addAction(megA);

			grpA = new QAction(Editor::tr("Group"), this);
			connect(grpA, &QAction::triggered, [this](){
				List::instance()->group(selectionModel()->selectedRows());
				setCurrentIndex(QModelIndex());
			});
			addAction(grpA);

			splA = new QAction(Editor::tr("Split"), this);
			splA->setShortcut(QKeySequence("S"));
			connect(splA, &QAction::triggered, [this](){
				List::instance()->split(selectionModel()->selectedRows());
				setCurrentIndex(QModelIndex());
			});
			addAction(splA);

			delA = new QAction(Editor::tr("Delete"), this);
			delA->setShortcut(QKeySequence("Del"));
			connect(delA, &QAction::triggered, [this](){
				List::instance()->waste(selectionModel()->selectedRows());
				setCurrentIndex(QModelIndex());
			});
			addAction(delA);

			connect(this, SIGNAL(doubleClicked(QModelIndex)), List::instance(), SLOT(jumpToIndex(QModelIndex)));
		}

	private:
		QAction *megA;
		QAction *grpA;
		QAction *splA;
		QAction *delA;

		void dragEnterEvent(QDragEnterEvent *e)
		{
			if (e->mimeData()->hasFormat("text/uri-list")){
				e->acceptProposedAction();
			}
			QListView::dragEnterEvent(e);
		}

		void dropEvent(QDropEvent *e)
		{
			if (e->mimeData()->hasFormat("text/uri-list")){
				for (const QString &item : QString(e->mimeData()->data("text/uri-list")).split('\n', QString::SkipEmptyParts)){
					List::instance()->appendMedia(QUrl(item).toLocalFile().trimmed());
				}
			}
			QListView::dropEvent(e);
		}

		void currentChanged(const QModelIndex &c,
			const QModelIndex &p)
		{
			QListView::currentChanged(c, p);
			selectionModel()->setCurrentIndex(QModelIndex(), QItemSelectionModel::NoUpdate);
		}

		QSize sizeHint() const
		{
			return QSize(150 * logicalDpiX() / 96, QListView::sizeHint().height());
		}

	};

	class Calendar :public QDialog
	{
	public:
		explicit Calendar(QWidget *parent = 0) :
			QDialog(parent, Qt::Popup)
		{
			auto layout = new QGridLayout(this);
			date = new QLabel(this);
			date->setAlignment(Qt::AlignCenter);
			layout->addWidget(date, 0, 1);
			prev = new QToolButton(this);
			next = new QToolButton(this);
			prev->setIcon(QIcon(":/Picture/previous.png"));
			next->setIcon(QIcon(":/Picture/next.png"));
			connect(prev, &QToolButton::clicked, [this](){
				setCurrentPage(page.addMonths(-1));
			});
			connect(next, &QToolButton::clicked, [this](){
				setCurrentPage(page.addMonths(+1));
			});
			layout->addWidget(prev, 0, 0);
			layout->addWidget(next, 0, 2);
			table = new QTableWidget(7, 7, this);
			table->setShowGrid(false);
			table->setSelectionMode(QAbstractItemView::SingleSelection);
			table->verticalHeader()->hide();
			table->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);
			table->horizontalHeader()->hide();
			table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
			table->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
			table->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
			connect(table, &QTableWidget::itemDoubleClicked, this, &QDialog::accept);
			layout->addWidget(table, 1, 0, 1, 3);
			resize(280 * logicalDpiX() / 96, 250 * logicalDpiY() / 96);
			Utils::setCenter(this);
		}

		QDate selectedDate()
		{
			QTableWidgetItem *item = table->currentItem();
			if (item){
				QDate selected = item->data(Qt::UserRole).toDate();
				if (selected <= QDate::currentDate()){
					return selected;
				}
			}
			return QDate();
		}

		void setCurrentDate(QDate _d)
		{
			curr = _d;
		}

		void setCurrentPage(QDate _d)
		{
			page.setDate(_d.year(), _d.month(), 1);
			table->clear();
			for (int day = 1; day <= 7; ++day){
				QTableWidgetItem *item = new QTableWidgetItem(QDate::shortDayName(day));
				item->setFlags(0);
				QFont f = item->font();
				f.setBold(true);
				item->setFont(f);
				item->setData(Qt::TextColorRole, QColor(Qt::black));
				table->setItem(0, day - 1, item);
			}
			int row = 1;
			for (QDate iter = page; iter.month() == page.month(); iter = iter.addDays(1)){
				QTableWidgetItem *item = new QTableWidgetItem(QString::number(iter.day()));
				item->setData(Qt::UserRole, iter);
				item->setData(Qt::TextAlignmentRole, Qt::AlignCenter);
				if (!count.contains(iter)){
					item->setFlags(0);
				}
				else{
					item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
				}
				if (iter == curr){
					item->setData(Qt::BackgroundRole, QColor(Qt::gray));
				}
				table->setItem(row, iter.dayOfWeek() - 1, item);
				if (item->column() == 6){
					++row;
				}
			}
			for (int r = 0; r < 7; ++r){
				for (int c = 0; c<7; ++c){
					if (table->item(r, c) == 0){
						QTableWidgetItem *item = new QTableWidgetItem;
						item->setFlags(0);
						table->setItem(r, c, item);
					}
				}
			}
			QDate last(page.year(), page.month(), page.daysInMonth());
			prev->setEnabled(page>count.firstKey());
			next->setEnabled(last < count.lastKey());
			date->setText(page.toString("MMM yyyy"));
		}

		void setCount(const QMap<QDate, int> &_c)
		{
			count = _c;
		}

	private:
		QToolButton *prev;
		QToolButton *next;
		QTableWidget *table;
		QDate page;
		QDate curr;
		QLabel *date;
		QMap<QDate, int> count;
		QDate currentLimit();
	};

	class Track :public QWidget
	{
	public:
		explicit Track(QWidget *parent = 0) :
			QWidget(parent)
		{
			int x = 100 * logicalDpiX() / 96, y = 100 * logicalDpiY() / 96;
			resize(parent->width(), y);
			m_record = nullptr;
			m_wheel = 0;
			m_magnet << 0 << 0 << 0;
			m_label = new QLabel(this);
			m_label->setGeometry(0, 0, x - 2, y*0.8 - 2);
			m_label->setFixedWidth(m_label->width());
			m_label->setWordWrap(true);
			m_delay = new QLineEdit(this);
			m_delay->setGeometry(0, y*0.8 - 2, x - 2, y / 5);
			m_delay->setFrame(false);
			m_delay->setAlignment(Qt::AlignCenter);
			connect(m_delay, &QLineEdit::editingFinished, [this](){
				QRegularExpression r("[\\d\\.\\+\\-\\(][\\s\\d\\.\\:\\+\\-\\*\\/\\(\\)]*");
				QString expression = r.match(m_delay->text()).captured();
				if (!expression.isEmpty()){
					delayRecord(Utils::evaluate(expression) * 1000 - m_record->delay);
				}
				else{
					m_delay->setText(m_prefix.arg(m_record->delay / 1000));
				}
			});
		}

		Record &getRecord()
		{
			return *m_record;
		}

		void setRecord(Record *r)
		{
			m_record = r;
			m_label->setText(r->string);
			m_label->setAlignment((m_label->sizeHint().height() > m_label->height() ? Qt::AlignTop : Qt::AlignVCenter) | Qt::AlignHCenter);
			m_delay->setText(m_prefix.arg(r->delay / 1000));
		}

		void setPrefix(QString p){
			m_prefix = p;
		}

		void setCurrent(qint64 c)
		{
			m_current = c;
			m_magnet[1] = c;
		}

		void setDuration(qint64 d)
		{
			m_duration = d;
			m_magnet[2] = d;
		}

	private:
		QLabel *m_label;
		QLineEdit *m_delay;

		Record *m_record;
		QString m_prefix;
		qint64 m_current;
		qint64 m_duration;
		QPoint m_point;
		QList<int> m_magnet;
		int m_wheel;

		void paintEvent(QPaintEvent *e)
		{
			QPainter painter(this);
			double x = 100 * logicalDpiX() / 96, y = 100 * logicalDpiY() / 96;
			painter.fillRect(e->rect(), Qt::gray);
			painter.fillRect(0, 0, x - 2, y - 2, Qt::white);
			if (x >= width() - 5 || m_duration <= 0){
				return;
			}
			int w = width() - x, m = 0, d = m_duration / (w / 5) + 1, t = 0;
			QHash<int, int> c;
			for (const Comment &com : m_record->danmaku){
				if (com.blocked){
					continue;
				}
				if (com.time >= 0 && com.time <= m_duration){
					++t;
				}
				int k = (com.time - m_record->delay) / d, v = c.value(k, 0) + 1;
				c.insert(k, v);
				m = v > m ? v : m;
			}
			if (m != 0){
				int o = w*m_record->delay / m_duration;
				if (!m_point.isNull()){
					o += mapFromGlobal(QCursor::pos()).x() - m_point.x();
					for (qint64 p : m_magnet){
						p = p*w / m_duration;
						if (qAbs(o - p) < 5){
							o = p;
							break;
						}
					}
				}
				painter.setClipRect(x, 0, w, y);
				for (int j = 0; j < w; j += 5){
					int he = c[j / 5] * (y - 2) / m;
					painter.fillRect(o + j + x, y - 2 - he, 5, he, Qt::white);
				}
				painter.setPen(Qt::white);
				QString count = QString("%1/%2").arg(t).arg(m_record->danmaku.size());
				QRect rect = painter.fontMetrics().boundingRect(x, 0, w, y - 2, Qt::AlignRight, count);
				rect.adjust(-5, 0, 0, 0);
				painter.fillRect(rect, QColor(160, 160, 164, 100));
				painter.drawText(rect, Qt::AlignCenter, count);
				if (m_current > 0){
					painter.fillRect(m_current*w / m_duration + x, 0, 1, y, Qt::red);
				}
			}
		}

		void wheelEvent(QWheelEvent *e)
		{
			m_wheel += e->angleDelta().y();
			if (qAbs(m_wheel) >= 120){
				qint64 d = (m_record->delay / 1000) * 1000;
				d += m_wheel > 0 ? -1000 : 1000;
				d -= m_record->delay;
				delayRecord(d);
				m_wheel = 0;
			}
			e->accept();
		}

		void mouseMoveEvent(QMouseEvent *e)
		{
			if (!m_point.isNull()){
				update();
			}
			else{
				m_point = e->pos();
			}
		}

		void mouseReleaseEvent(QMouseEvent *e)
		{
			if (!m_point.isNull()){
				int w = width() - 100 * logicalDpiX() / 96;
				qint64 d = (e->x() - m_point.x())*m_duration / w;
				for (qint64 p : m_magnet){
					if (qAbs(d + m_record->delay - p) < m_duration / (w / 5) + 1){
						d = p - m_record->delay;
						break;
					}
				}
				delayRecord(d);
			}
			m_point = QPoint();
		}

		void delayRecord(qint64 delay)
		{
			m_record->delay += delay;
			for (Comment &c : m_record->danmaku){
				c.time += delay;
			}
			QMetaObject::invokeMethod(Danmaku::instance(), "parse", Qt::QueuedConnection, Q_ARG(int, 0x1 | 0x2));
		}
	};

	class MScroll :public QScrollBar
	{
	public:
		explicit MScroll(QWidget *parent = 0) :
			QScrollBar(parent)
		{
		}

	private:
		void paintEvent(QPaintEvent *e)
		{
			QScrollBar::paintEvent(e);
			QPainter painter(this);
			painter.setPen(Qt::gray);
			QPoint points[4] = { rect().topLeft(), rect().topRight(), rect().bottomRight(), rect().bottomLeft() };
			painter.drawPolyline(points, 4);
		}
	};

	class PoolEditor :public QWidget
	{
	public:
		explicit PoolEditor(QWidget *parent = 0) :
			QWidget(parent)
		{
			setMinimumWidth(100 * logicalDpiX() / 96);
			manager = new QNetworkAccessManager(this);
			NetworkConfiguration::instance()->setManager(manager);

			scroll = new MScroll(this);
			scroll->setSingleStep(20);
			connect(scroll, &QScrollBar::valueChanged, [this](int value){
				value = -value;
				for (Track *t : tracks){
					t->move(0, value);
					value += 100 * logicalDpiY() / 96;
				}
			});

			widget = new QWidget(this);
			widget->setContextMenuPolicy(Qt::CustomContextMenu);
			connect(widget, &QWidget::customContextMenuRequested, [this](QPoint point){
				Track *c = nullptr;
				for (Track *t : tracks){
					if (t->geometry().contains(point)){
						c = t;
						break;
					}
				}
				if (!c){
					return;
				}
				QMenu menu(this);
				Load *load = Load::instance();
				auto &p = Danmaku::instance()->getPool();
				auto &r = c->getRecord();
				QAction *fullA = menu.addAction(Editor::tr("Full"));
				fullA->setEnabled(load->canFull(&r));
				connect(fullA, &QAction::triggered, [=, &r](){
					load->fullDanmaku(&r);
				});
				menu.addSeparator();
				connect(menu.addAction(Editor::tr("History")), &QAction::triggered, [=, &r](){
					Calendar history(window());
					QMap<QDate, int> count;
					QDate c = r.limit == 0 ? QDate::currentDate().addDays(1) : QDateTime::fromTime_t(r.limit).date();
					history.setCurrentDate(c);
					if (!load->canHist(&r)){
						for (const Comment &c : r.danmaku){
							++count[QDateTime::fromTime_t(c.date).date()];
						}
						count[QDate::currentDate().addDays(1)] = 0;
						count.remove(count.firstKey());
						history.setCount(count);
						history.setCurrentPage(c);
					}
					else{
						QString cid = QFileInfo(r.source).baseName();
						QString api("http://comment.%1/rolldate,%2");
						api = api.arg(Utils::customUrl(Utils::Bilibili));
						QNetworkReply *reply = manager->get(QNetworkRequest(api.arg(cid)));
						connect(reply, &QNetworkReply::finished, [&](){
							QMap<QDate, int> count;
							for (QJsonValue iter : QJsonDocument::fromJson(reply->readAll()).array()){
								QJsonObject obj = iter.toObject();
								QJsonValue time = obj["timestamp"], size = obj["new"];
								count[QDateTime::fromTime_t(time.toVariant().toInt()).date()] += size.toVariant().toInt();
							}
							count[QDate::currentDate().addDays(1)] = 0;
							history.setCount(count);
							history.setCurrentPage(c);
						});
					}
					if (history.exec() == QDialog::Accepted){
						QDate selected = history.selectedDate();
						if (!load->canHist(&r)){
							r.limit = selected.isValid() ? QDateTime(selected).toTime_t() : 0;
							Danmaku::instance()->parse(0x2);
							widget->update();
						}
						else{
							load->loadHistory(&r, history.selectedDate());
						}
					}
				});
				connect(menu.addAction(Editor::tr("Delete")), &QAction::triggered, [&p, &r](){
					for (auto i = p.begin(); i != p.end(); ++i){
						if (&r == &(*i)){
							p.erase(i);
							Danmaku::instance()->parse(0x1 | 0x2);
							break;
						}
					}
				});
				menu.exec(mapToGlobal(point));
			});

			parseRecords();
			connect(Danmaku::instance(), &Danmaku::modelReset, this, &PoolEditor::parseRecords, Qt::QueuedConnection);
		}

	private:
		QList<Track *> tracks;
		QWidget *widget;
		QScrollBar *scroll;
		QNetworkAccessManager *manager;

		void paintEvent(QPaintEvent *)
		{
			if (!Danmaku::instance()->getPool().isEmpty()){
				return;
			}
			QPainter p(this);
			p.setPen(Qt::gray);
			QString t = QStringLiteral("_(:з」∠)_");
			QFont f = p.font();
			f.setPointSize(55);
			f.setBold(true);
			p.setFont(f);
			QSize s = p.fontMetrics().size(0, t);
			if (width() < s.width()){
				QTransform r;
				r.translate(width() / 2, height() / 2);
				r.rotate(qRadiansToDegrees(qAcos((width() - 100) / (double)(s.width() - 100))), Qt::YAxis);
				p.setTransform(r);
			}
			else{
				p.translate(width() / 2, height() / 2);
			}
			p.drawText(QRect(QPoint(-s.width() / 2, -s.height() / 2), s), t);
		}

		void parseRecords()
		{
			qDeleteAll(tracks);
			tracks.clear();
			QList<Record> &pool = Danmaku::instance()->getPool();
			if (!pool.isEmpty()){
				qint64 duration = -1;
				if (duration <= 0){
					duration = APlayer::instance()->getDuration();
				}
				if (duration <= 0){
					duration = Danmaku::instance()->getDuration();
				}
				int height = 0;
				for (Record &r : pool){
					Track *t = new Track(widget);
					t->setPrefix(Editor::tr("Delay: %1s"));
					t->move(0, height);
					height += 100 * logicalDpiY() / 96;
					t->setCurrent(APlayer::instance()->getTime());
					t->setDuration(duration);
					t->setRecord(&r);
					t->show();
					tracks.append(t);
				}
			}
			parseLayouts();
			update();
		}

		void parseLayouts()
		{
			double y = 100 * logicalDpiY() / 96;
			QRect r = rect();
			if (tracks.size()*y - 2 > r.height()){
				scroll->show();
				scroll->setGeometry(r.adjusted(r.width() - scroll->width(), 0, 0, 0));
				widget->setGeometry(r.adjusted(0, 0, -scroll->width(), 0));
			}
			else{
				scroll->hide();
				widget->setGeometry(r);
			}
			scroll->setRange(0, tracks.size()*y - r.height() - 2);
			scroll->setValue(tracks.size() ? -tracks.first()->y() : 0);
			scroll->setPageStep(r.height());
			for (Track *t : tracks){
				t->resize(widget->width(), y);
			}
		}

		void resizeEvent(QResizeEvent *)
		{
			parseLayouts();
		}
	};
}

void Editor::exec(QWidget *parent, int show)
{
	static Editor *executing;
	if (!executing){
		Editor editor(parent);
		if (!(show&List)){
			editor.list->hide();
		}
		if (!(show&Pool)){
			editor.pool->hide();
		}
		executing = &editor;
		editor.QDialog::exec();
		executing = nullptr;
	}
	else{
		executing->activateWindow();
	}
}

Editor::Editor(QWidget *parent) :
QDialog(parent)
{
	setMinimumSize(650 * logicalDpiX() / 96, 450 * logicalDpiY() / 96);
	setWindowTitle(tr("Editor"));

	QSplitter *splitter = new QSplitter(this);
	splitter->addWidget(list = new ListEditor(this));
	splitter->setStretchFactor(0, 0);
	splitter->addWidget(pool = new PoolEditor(this));
	splitter->setStretchFactor(1, 1);
	(new QGridLayout(this))->addWidget(splitter);
	setFocus();
	Utils::setCenter(this);
}
