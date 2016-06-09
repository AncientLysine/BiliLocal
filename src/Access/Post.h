/*=======================================================================
*
*   Copyright (C) 2013-2015 Lysine.
*
*   Filename:    Post.h
*   Time:        2013/05/23
*   Author:      zhengdanwei
*   Contributor: Lysine
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

#include "../Utils.h"
#include <QtCore>
#include <QtNetwork>
#include <functional>

class PostPrivate;

class Post : public QObject
{
	Q_OBJECT
public:
	enum State
	{
		None,
		Code
	};

	struct Proc
	{
		std::function<bool(QString)> regular;
		int priority;
		std::function<void(QNetworkReply *)> process;
	};

	struct Task
	{
		QString code;
		Comment comment;
		QNetworkRequest request;
		QByteArray data;
		int state;
		const Record *target;
		const Proc *processer;
		Task() :state(0), processer(nullptr){}
	};

	explicit Post(QObject *parent = nullptr);
	virtual ~Post();

private:
	PostPrivate *const d_ptr;
	Q_DECLARE_PRIVATE(Post);

signals:
	void stateChanged(int code);
	void errorOccured(int code);

public slots:
	void addProc(const Post::Proc *proc);
	const Post::Proc *getProc(QString code);

	bool canPost(QString code);
	void postComment(const Record *, const Comment *);

	void forward();
	void dequeue();
	bool enqueue(const Post::Task &);
	Post::Task *getHead();
};
