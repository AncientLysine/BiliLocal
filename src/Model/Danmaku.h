/*=======================================================================
*
*   Copyright (C) 2013-2015 Lysine.
*
*   Filename:    Danmaku.h
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

#pragma once

#include <QtGui>
#include <QtCore>

class Comment;
class Record;
class DanmakuPrivate;

class Danmaku : public QAbstractItemModel
{
	Q_OBJECT
public:
	enum Role
	{
		ModeRole = Qt::UserRole,
		FontRole,
		ColorRole,
		TimeRole,
		DateRole,
		SenderRole,
		StringRole,
		BlockRole
	};

	virtual QVariant data(const QModelIndex &index, int role) const override;
	virtual int rowCount(const QModelIndex &parent = QModelIndex()) const override;
	virtual int columnCount(const QModelIndex &parent = QModelIndex()) const override;
	virtual QModelIndex parent(const QModelIndex &) const override;
	virtual QModelIndex index(int row, int colum, const QModelIndex &parent = QModelIndex()) const override;
	virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
	virtual QHash<int, QByteArray> roleNames() const override;

	explicit Danmaku(QObject *parent = nullptr);
	virtual ~Danmaku();

	QList<Record> &getPool();
	QList<Comment *>::iterator begin();
	QList<Comment *>::iterator end();
	Comment *at(int index);
	void append(Record &&record);

private:
	DanmakuPrivate *const d_ptr;
	Q_DECLARE_PRIVATE(Danmaku);

signals:
	void modelAppend();
	void modelInsert();

public slots:
	void clear();
	void append(QString source, const Comment *comment);
	void parse(int flag = 0);
	void delayAll(qint64 time);
	void saveToFile(QString file) const;
	qint64 getDuration() const;
	int size() const;
};
