/*=======================================================================
*
*   Copyright (C) 2013-2015 Lysine.
*
*   Filename:    Danmaku.cpp
*   Time:        2013/03/18
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
#include "Danmaku.h"
#include "../Config.h"
#include "../Local.h"
#include "../Utils.h"
#include "../Access/Load.h"
#include "../Player/APlayer.h"
#include "../Render/ARender.h"
#include "../Graphic/Graphic.h"
#include "../Model/Shield.h"
#include "../UI/Editor.h"
#include <algorithm>

class DanmakuPrivate
{
public:
	qint32 curr;
	qint64 time;
	qint64 dura;
	QList<Record> pool;
	QList<Comment *> danm;
	QList<Graphic *> draw;
	QAtomicInt wait;
	mutable QReadWriteLock lock;
};

Danmaku *Danmaku::ins = nullptr;

Danmaku *Danmaku::instance()
{
	return ins ? ins : new Danmaku(qApp);
}

Danmaku::Danmaku(QObject *parent) :
QAbstractItemModel(parent), d_ptr(new DanmakuPrivate)
{
	Q_D(Danmaku);
	ins = this;
	setObjectName("Danmaku");
	d->curr = d->time = 0;
	d->dura = -1;
	connect(APlayer::instance(), &APlayer::jumped, this, &Danmaku::jumpToTime);
	connect(APlayer::instance(), &APlayer::timeChanged, this, &Danmaku::setTime);
	connect(this, SIGNAL(layoutChanged()), ARender::instance(), SLOT(draw()));
	QMetaObject::invokeMethod(this, "alphaChanged", Qt::QueuedConnection, Q_ARG(int, Config::getValue("/Danmaku/Alpha", 100)));
}

Danmaku::~Danmaku()
{
	Q_D(Danmaku);
	qThreadPool->clear();
	qThreadPool->waitForDone();
	qDeleteAll(d->draw);
	delete d_ptr;
}

void Danmaku::draw(QPainter *painter, double move)
{
	Q_D(Danmaku);
	d->lock.lockForWrite();
	QVarLengthArray<Graphic *> dirty(d->draw.size());
	for (auto iter = d->draw.begin(); iter != d->draw.end();){
		Graphic *g = *iter;
		if (g->move(move)){
			dirty.append(g);
			++iter;
		}
		else{
			delete g;
			iter = d->draw.erase(iter);
		}
	}
	d->lock.unlock();
	for (Graphic *g : dirty){
		g->draw(painter);
	}
}

QList<Record> &Danmaku::getPool()
{
	Q_D(Danmaku);
	return d->pool;
}

QVariant Danmaku::data(const QModelIndex &index, int role) const
{
	Q_D(const Danmaku);
	if (index.isValid()){
		const Comment &comment = *d->danm[index.row()];
		switch (role){
		case Qt::DisplayRole:
			if (index.column() == 0){
				if (comment.blocked){
					return tr("Blocked");
				}
				else{
					QString time("%1:%2");
					qint64 sec = comment.time / 1000;
					if (sec < 0){
						time.prepend("-");
						sec = -sec;
					}
					time = time.arg(sec / 60, 2, 10, QChar('0'));
					time = time.arg(sec % 60, 2, 10, QChar('0'));
					return time;
				}
			}
			else{
				if (comment.mode == 7){
					QJsonDocument doc = QJsonDocument::fromJson(comment.string.toUtf8());
					if (doc.isArray()){
						QJsonArray data = doc.array();
						return data.size() >= 5 ? data.at(4).toString() : QString();
					}
					else{
						return doc.object()["n"].toString();
					}
				}
				else{
					return comment.string.left(50).remove('\n');
				}
			}
		case Qt::ForegroundRole:
			if (index.column() == 0){
				if (comment.blocked || comment.time > d->dura){
					return QColor(Qt::red);
				}
			}
			else{
				if (comment.blocked){
					return QColor(Qt::gray);
				}
			}
			break;
		case Qt::ToolTipRole:
			return Qt::convertFromPlainText(comment.string);
		case Qt::TextAlignmentRole:
			if (index.column() == 0){
				return Qt::AlignCenter;
			}
			break;
		case Qt::BackgroundRole:
			switch (comment.mode){
			case 7:
				return QColor(200, 255, 200);
			case 8:
				return QColor(255, 255, 160);
			default:
				break;
			}
		case Qt::UserRole:
			return (quintptr)&comment;
		}
	}
	return QVariant();
}

int Danmaku::rowCount(const QModelIndex &parent) const
{
	Q_D(const Danmaku);
	return parent.isValid() ? 0 : d->danm.size();
}

int Danmaku::columnCount(const QModelIndex &parent) const
{
	return parent.isValid() ? 0 : 2;
}

QModelIndex Danmaku::parent(const QModelIndex &) const
{
	return QModelIndex();
}

QModelIndex Danmaku::index(int row, int colum, const QModelIndex &parent) const
{
	if (!parent.isValid() && colum < 2){
		return createIndex(row, colum);
	}
	return QModelIndex();
}

QVariant Danmaku::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (role == Qt::DisplayRole&&orientation == Qt::Horizontal){
		if (section == 0){
			return tr("Time");
		}
		if (section == 1){
			return tr("Comment");
		}
	}
	return QVariant();
}

const Comment *Danmaku::commentAt(QPointF point) const
{
	Q_D(const Danmaku);
	d->lock.lockForRead();
	for (Graphic *g : d->draw){
		if (g->currentRect().contains(point)){
			d->lock.unlock();
			return g->getSource();
		}
	}
	d->lock.unlock();
	return nullptr;
}

void Danmaku::setAlpha(int alpha)
{
	Config::setValue("/Danmaku/Alpha", alpha);
	emit alphaChanged(alpha);
}

void Danmaku::resetTime()
{
	Q_D(Danmaku);
	d->curr = d->time = 0;
}

void Danmaku::clearPool()
{
	Q_D(Danmaku);
	if (!d->pool.isEmpty()){
		clearCurrent();
		d->pool.clear();
		d->danm.clear();
		parse(0x1 | 0x2);
	}
}

namespace
{
	class CommentPointer
	{
	public:
		const Comment *comment;

		CommentPointer(const Comment *comment) :
			comment(comment)
		{
		}

		inline bool operator == (const CommentPointer &o) const
		{
			return *comment == *o.comment;
		}
	};

	inline uint qHash(const CommentPointer &p, uint seed = 0)
	{
		return ::qHash(*p.comment, seed);
	}
}

void Danmaku::appendToPool(const Record *record)
{
	Q_D(Danmaku);
	Record *append = 0;
	for (Record &r : d->pool) {
		if (r.source == record->source) {
			append = &r;
			break;
		}
	}
	if (append == nullptr) {
		d->pool.append(*record);
		QSet<CommentPointer> s;
		auto &l = d->pool.last().danmaku;
		s.reserve(l.size());
		auto e = std::remove_if(l.begin(), l.end(), [&s](const Comment &c) {
			int n = s.size();
			s.insert(&c);
			return n == s.size();
		});
		l.erase(e, l.end());
	}
	else {
		QSet<CommentPointer> s;
		auto &l = append->danmaku;
		int c = l.size() + record->danmaku.size();
		s.reserve(c);
		l.reserve(c);
		for (const Comment &c : l) {
			s.insert(&c);
		}
		for (const Comment &i : record->danmaku) {
			l.append(i);
			Comment &c = l.last();
			c.time += append->delay - record->delay;
			int n = s.size();
			s.insert(&c);
			if (n == s.size()) {
				l.removeLast();
			}
		}
		if (record->full) {
			append->full = true;
		}
		append->limit = record->limit == 0 ? 0 : qMax(append->limit, record->limit);
	}
	parse(0x1 | 0x2);
	if (d->pool.size() >= 2 && !append) {
		QTimer::singleShot(0, []() {
			if (!Load::instance()->getHead()) {
				UI::Editor::exec(lApp->mainWidget());
			}
		});
	}
}

namespace
{
	class CommentComparer
	{
	public:
		inline bool operator ()(const Comment *f, const Comment *s)
		{
			return f->time < s->time;
		}

		//overloads for comparing with time
		inline bool operator ()(const Comment *c, qint64 time)
		{
			return c->time < time;
		}

		inline bool operator ()(qint64 time, const Comment *c)
		{
			return time < c->time;
		}
	};
}

void Danmaku::appendToPool(QString source, const Comment *comment)
{
	Q_D(Danmaku);
	Record *append = nullptr;
	for (Record &r : d->pool){
		if (r.source == source){
			append = &r;
			break;
		}
	}
	if (!append){
		Record r;
		r.source = source;
		d->pool.append(r);
		append = &d->pool.last();
	}
	append->danmaku.append(*comment);
	Comment *c = &append->danmaku.last();
	c->time += append->delay;
	d->danm.insert(std::upper_bound(d->danm.begin(), d->danm.end(), c, CommentComparer()), c);
	append->limit = append->limit == 0 ? 0 : qMax(append->limit, c->date);
	parse(0x2);
}

void Danmaku::clearCurrent(bool soft)
{
	Q_D(Danmaku);
	qThreadPool->clear();
	qThreadPool->waitForDone();
	d->lock.lockForWrite();
	for (auto iter = d->draw.begin(); iter != d->draw.end();){
		Graphic *g = *iter;
		if (soft&&g->stay()){
			++iter;
		}
		else{
			delete g;
			iter = d->draw.erase(iter);
		}
	}
	d->lock.unlock();
	ARender::instance()->draw();
}

void Danmaku::insertToCurrent(Graphic *graphic, int index)
{
	Q_D(Danmaku);
	d->lock.lockForWrite();
	graphic->setIndex();
	int size = d->draw.size(), next;
	if (size == 0 || index == 0){
		next = 0;
	}
	else{
		int ring = size + 1;
		next = index > 0 ? (index%ring) : (ring + index%ring);
		if (next == 0){
			next = size;
		}
	}
	d->draw.insert(next, graphic);
	d->lock.unlock();
}

void Danmaku::parse(int flag)
{
	Q_D(Danmaku);
	if ((flag & 0x1) > 0){
		beginResetModel();
		d->danm.clear();
		for (Record &record : d->pool){
			d->danm.reserve(d->danm.size() + record.danmaku.size());
			for (Comment &comment : record.danmaku){
				d->danm.append(&comment);
			}
		}
		std::stable_sort(d->danm.begin(), d->danm.end(), CommentComparer());
		d->dura = -1;
		for (Comment *c : d->danm) {
			if (c->time < 10000000 || c->time < d->dura * 2) {
				d->dura = c->time;
			}
			else {
				break;
			}
		}
		jumpToTime(d->time);
		endResetModel();
	}
	if ((flag & 0x2) > 0) {
		//MUST BE SORTED
		Q_ASSERT(std::is_sorted(d->danm.begin(), d->danm.end(), CommentComparer()));

		// Date Limit
		for (Record &r : d->pool) {
			for (Comment &c : r.danmaku) {
				c.blocked = r.limit != 0 && c.date > r.limit;
			}
		}
		// Repeat Limit
		int limit = Config::getValue("/Shield/Limit/Count", 5);
		int range = Config::getValue("/Shield/Limit/Range", 10000);
		if (limit != 0) {
			QVector<QString> clean;
			int size = d->danm.size();
			clean.reserve(size);
			for (const Comment *iter : d->danm) {
				const auto &raw = iter->string;

				int length = raw.length();
				const QChar *data = raw.data();

				QString clr;

				int passed = 0;
				const QChar *head = data;

				for (int i = 0; i < length; ++i) {
					const QChar &c = data[i];
					if (c.isLetterOrNumber() || c.isMark() || c == '_') {
						++passed;
					}
					else if (passed > 0) {
						clr.reserve(length);
						clr.append(head, passed);
						passed = 0;
						head = data + i + 1;
					}
				}
				if (passed == length) {
					clean.append(raw);
				}
				else {
					if (passed > 0) {
						clr.append(head, passed);
					}
					clean.append(clr);
				}
			}
			QHash<QString, int> count;
			int sta = 0, end = sta;
			for (; end < size; ++end) {
				Comment *e = d->danm[end];
				while (d->danm[sta]->time + range < e->time) {
					auto i = count.find(clean[sta]);
					if (i.value() == 1) {
						count.erase(i);
					}
					else if (i.value() > 1) {
						--(i.value());
					}
					++sta;
				}
				int &num = count[clean[end]];
				if (num >= 0 && ++num > limit && e->mode <= 6) {
					num = -1;
				}
			}
			for (; sta < size; ++sta) {
				auto i = count.find(clean[sta]);
				if (i.value() > 0) {
					count.erase(i);
				}
			}
			for (int i = 0; i < size; ++i) {
				Comment *c = d->danm[i];
				c->blocked = c->blocked || count.contains(clean[i]);
			}
		}
		// Regex Limit
		for (Comment *c : d->danm) {
			c->blocked = c->blocked || Shield::instance()->isBlocked(*c);
		}
		qThreadPool->clear();
		qThreadPool->waitForDone();
		d->lock.lockForWrite();
		for (auto iter = d->draw.begin(); iter != d->draw.end();) {
			const Comment *cur = (*iter)->getSource();
			if (cur&&cur->blocked) {
				delete *iter;
				iter = d->draw.erase(iter);
			}
			else {
				++iter;
			}
		}
		d->lock.unlock();
		emit layoutChanged();
	}
}

namespace
{
	class Process :public QRunnable
	{
	public:
		Process(DanmakuPrivate *d, const QList<const Comment *> &w) :
			danm(d), wait(w)
		{
			createTime = QDateTime::currentMSecsSinceEpoch();
		}

		~Process()
		{
			danm->wait -= wait.size();
		}

		virtual void run() override
		{
			//跳过500毫秒以上未处理的弹幕
			if (wait.isEmpty() || createTime < QDateTime::currentMSecsSinceEpoch() - 500){
				return;
			}
			//子线程优先级需要低于主线程
			QThread::currentThread()->setPriority(QThread::LowPriority);
			QList<Graphic *> ready;
			for (const Comment *comment : wait){
				Graphic *graphic = nullptr;
				try{
					graphic = Graphic::create(*comment);
				}
				catch (Graphic::format_unrecognized){
					//自带弹幕系统未识别，通知插件处理
					emit Danmaku::instance()->unrecognizedComment(comment);
				}
				catch (Graphic::args_prasing_error){}
				if (!graphic){
					continue;
				}
				QRectF &rect = graphic->currentRect();
				const QList<QRectF> &locate = graphic->locate();
				switch (locate.size()){
				case 1:
					//图元指定位置
					rect = locate.first();
				case 0:
					//弹幕自行定位
					ready.append(graphic);
					danm->lock.lockForWrite();
					break;
				default:
				{
					//弹幕自动定位
					QVarLengthArray<int> result(locate.size());
					memset(result.data(), 0, sizeof(int)*result.size());
					//计算每个位置的拥挤程度
					auto calculate = [&](const QList<Graphic *> &data){
						for (Graphic *iter : data){
							if (iter->getMode() != comment->mode){
								continue;
							}
							const QRectF &rect = iter->currentRect();
							const QRectF &from = locate[0];
							double stp = locate[1].top() - from.top();
							double len = from.height();
							int sta = qMax(0, qFloor((stp > 0 ? (rect.top() - from.top()) : (rect.bottom() - from.bottom())) / stp));
							int end = qMin(qCeil((rect.height() + len) / qAbs(stp) + sta), result.size());
							for (; sta < end; ++sta){
								graphic->currentRect() = locate[sta];
								result[sta] += graphic->intersects(iter);
							}
						}
					};
					//获取读锁，计算现有弹幕的拥挤程度
					danm->lock.lockForRead();
					quint64 last = danm->draw.isEmpty() ? 0 : danm->draw.last()->getIndex();
					calculate(danm->draw);
					danm->lock.unlock();
					ready.append(graphic);
					//获取写锁，计算两次锁之间的新弹幕
					danm->lock.lockForWrite();
					QList<Graphic *> addtion;
					QListIterator<Graphic *> iter(danm->draw);
					iter.toBack();
					while (iter.hasPrevious()){
						Graphic *p = iter.previous();
						if (p->getIndex() > last&&p->getMode() == comment->mode){
							addtion.prepend(p);
						}
						else break;
					}
					calculate(addtion);
					//挑选最空闲的位置
					int thin;
					thin = result[0];
					rect = locate[0];
					for (int i = 1; thin != 0 && i < result.size(); ++i){
						if (thin > result[i]){
							thin = result[i];
							rect = locate[i];
						}
					}
				}
				}
				//相同内容的弹幕需要同时启动，先将其冻结
				graphic->setEnabled(false);
				graphic->setIndex();
				danm->draw.append(graphic);
				danm->lock.unlock();
			}
			danm->lock.lockForWrite();
			for (Graphic *iter : ready){
				iter->setEnabled(true);
			}
			danm->lock.unlock();
		}

		Process &operator=(const Process &) = delete;

	private:
		DanmakuPrivate * const danm;
		qint64 createTime;
		QList<const Comment *> wait;
	};
}

void Danmaku::setTime(qint64 time)
{
	Q_D(Danmaku);
	d->time = time;
	int limit = Config::getValue("/Shield/Density", 0);
	QMap<qint64, QMap<QString, QList<const Comment *>>> buffer;
	for (; d->curr < d->danm.size() && d->danm[d->curr]->time < time; ++d->curr){
		const Comment *c = d->danm[d->curr];
		if (!c->blocked && (limit <= 0 || d->wait + d->draw.size() < limit)){
			++d->wait;
			buffer[c->time][c->string].append(c);
		}
	}
	for (const auto &sameTime : buffer){
		for (const auto &sameText : sameTime){
			qThreadPool->start(new Process(d, sameText));
		}
	}
}

void Danmaku::delayAll(qint64 time)
{
	Q_D(Danmaku);
	for (Record &r : d->pool){
		r.delay += time;
		for (Comment &c : r.danmaku){
			c.time += time;
		}
	}
	jumpToTime(d->time);
	emit layoutChanged();
}

void Danmaku::jumpToTime(qint64 time)
{
	Q_D(Danmaku);
	clearCurrent(true);
	d->time = time;
	d->curr = std::lower_bound(d->danm.begin(), d->danm.end(), time, CommentComparer()) - d->danm.begin();
}

void Danmaku::saveToFile(QString file) const
{
	Q_D(const Danmaku);
	QFile f(file);
	f.open(QIODevice::WriteOnly | QIODevice::Text);
	bool skip = Config::getValue("/Interface/Save/Skip", false);
	if (file.endsWith("xml", Qt::CaseInsensitive)){
		QXmlStreamWriter w(&f);
		w.setAutoFormatting(true);
		w.writeStartDocument();
		w.writeStartElement("i");
		w.writeStartElement("chatserver");
		w.writeCharacters("chat." + Utils::customUrl(Utils::Bilibili));
		w.writeEndElement();
		w.writeStartElement("mission");
		w.writeCharacters("0");
		w.writeEndElement();
		w.writeStartElement("source");
		w.writeCharacters("k-v");
		w.writeEndElement();
		for (const Comment *c : d->danm){
			if (c->blocked&&skip){
				continue;
			}
			w.writeStartElement("d");
			QStringList l;
			l << QString::number(c->time / 1000.0) <<
				QString::number(c->mode) <<
				QString::number(c->font) <<
				QString::number(c->color) <<
				QString::number(c->date) <<
				"0" <<
				c->sender <<
				"0";
			w.writeAttribute("p", l.join(','));
			w.writeCharacters(c->string);
			w.writeEndElement();
		}
		w.writeEndElement();
		w.writeEndDocument();
	}
	else{
		QJsonArray a;
		for (const Comment *c : d->danm){
			if (c->blocked&&skip){
				continue;
			}
			QJsonObject o;
			QStringList l;
			l << QString::number(c->time / 1000.0) <<
				QString::number(c->color) <<
				QString::number(c->mode) <<
				QString::number(c->font) <<
				c->sender <<
				QString::number(c->date);
			o["c"] = l.join(',');
			o["m"] = c->string;
			a.append(o);
		}
		f.write(QJsonDocument(a).toJson(QJsonDocument::Compact));
	}
	f.close();
}

qint64 Danmaku::getDuration() const
{
	Q_D(const Danmaku);
	return d->dura;
}
