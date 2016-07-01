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
#include "../Model/Shield.h"
#include <algorithm>

class DanmakuPrivate
{
public:
	qint64 dura;
	QList<Record> pool;
	QList<Comment *> danm;
};

Danmaku::Danmaku(QObject *parent)
	:QAbstractItemModel(parent), d_ptr(new DanmakuPrivate)
{
	Q_D(Danmaku);
	setObjectName("Danmaku");
	d->dura = -1;
}

Danmaku::~Danmaku()
{
	delete d_ptr;
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

QList<Record> &Danmaku::getPool()
{
	Q_D(Danmaku);
	return d->pool;
}

QList<Comment*>::iterator Danmaku::begin()
{
	Q_D(Danmaku);
	return d->danm.begin();
}

QList<Comment*>::iterator Danmaku::end()
{
	Q_D(Danmaku);
	return d->danm.end();
}

Comment * Danmaku::at(int index)
{
	Q_D(Danmaku);
	return d->danm.at(index);
}

void Danmaku::clear()
{
	Q_D(Danmaku);
	if (!d->pool.isEmpty()){
		//clearCurrent();
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

void Danmaku::append(const Record *record)
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
	if (append) {
		emit modelInsert();
	}
	else {
		emit modelAppend();
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
	};
}

void Danmaku::append(QString source, const Comment *comment)
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
	emit modelInsert();
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
			c->blocked = c->blocked || lApp->findObject<Shield>()->isBlocked(*c);
		}
		emit layoutChanged();
	}
}

void Danmaku::delayAll(qint64 time)
{
	Q_D(Danmaku);
	emit beginResetModel();
	for (Record &r : d->pool){
		r.delay += time;
		for (Comment &c : r.danmaku){
			c.time += time;
		}
	}
	emit endResetModel();
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

int Danmaku::size() const
{
	Q_D(const Danmaku);
	return d->danm.size();
}
