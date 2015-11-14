/*=======================================================================
*
*   Copyright (C) 2013-2015 Lysine.
*
*   Filename:    Post.cpp
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

#include "Post.h"
#include "AccessPrivate.h"
#include "../Model/Danmaku.h"

Post *Post::ins = nullptr;

Post *Post::instance()
{
	return ins ? ins : new Post(qApp);
}

class PostPrivate : public AccessPrivate<Post ,Post::Proc, Post::Task>
{
public:
	explicit PostPrivate(Post *post):
		AccessPrivate<Post, Post::Proc, Post::Task>(post)
	{
	}

	virtual void onShift() override
	{
		Q_Q(Post);
		Post::Task &task = queue.head();
		emit q->stateChanged(task.state);
		remain.insert(task.data.isEmpty() ? manager.get(task.request) : manager.post(task.request, task.data));
	}
};

Post::Post(QObject *parent) : QObject(parent), d_ptr(new PostPrivate(this))
{
	Q_D(Post);
	ins = this;
	setObjectName("Post");

	auto avProcess = [this](QNetworkReply *reply){
		Q_D(Post);
		Task &task = d->queue.head();
		switch (task.state){
		case None:{
			QString api("http://interface.%1/dmpost");
			api = api.arg(Utils::customUrl(Utils::Bilibili));
			task.request.setUrl(api);
			task.request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
			const Comment &c = task.comment;
			QUrlQuery params;
			params.addQueryItem("cid", QFileInfo(task.target->source).baseName());
			params.addQueryItem("date", QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));
			params.addQueryItem("pool", "0");
			params.addQueryItem("playTime", QString::number(c.time / 1000.0, 'f', 4));
			params.addQueryItem("color", QString::number(c.color));
			params.addQueryItem("fontsize", QString::number(c.font));
			params.addQueryItem("message", c.string);
			params.addQueryItem("rnd", QString::number(qrand()));
			params.addQueryItem("mode", QString::number(c.mode));
			task.data = QUrl::toPercentEncoding(params.query(QUrl::FullyEncoded), "%=&", "-.~_");
			task.state = Code;
			forward();
			break;
		}
		case Code:{
			int code = QString(reply->readAll()).toInt();
			emit stateChanged(task.state = code > 0 ? None : code);
			dequeue();
			break;
		}
		}
	};
	auto avRegular = [](QString code){
		static QRegularExpression r("a(v(\\d+([#_])?(\\d+)?)?)?");
		r.setPatternOptions(QRegularExpression::CaseInsensitiveOption);
		return r.match(code).capturedLength() == code.length();
	};
	d->pool.append({ avRegular, 0, avProcess });

	connect(this, &Post::stateChanged, [this](int code){
		switch (code){
		case None:
		case Code:
			break;
		default:
		{
			Q_D(Post);
			if (!d->tryNext()){
				emit errorOccured(code);
			}
			break;
		}
		}
	});
}

Post::~Post()
{
	delete d_ptr;
}

void Post::addProc(const Post::Proc *proc)
{
	Q_D(Post);
	d->addProc(proc);
}

const Post::Proc *Post::getProc(QString code)
{
	Q_D(Post);
	return d->getProc(code);
}

bool Post::canPost(QString code)
{
	return getProc(code) != nullptr;
}

void Post::postComment(const Record *r, const Comment *c)
{
	Task task;
	task.code = r->access;
	task.comment = *c;
	task.comment.time -= r->delay;
	task.target = r;
	task.processer = getProc(task.code);
	enqueue(task);
}

void Post::forward()
{
	Q_D(Post);
	d->forward();
}

void Post::dequeue()
{
	Q_D(Post);
	d->dequeue();
}

bool Post::enqueue(const Post::Task &task)
{
	Q_D(Post);
	return d->enqueue(task);
}

Post::Task *Post::getHead()
{
	Q_D(Post);
	return d->getHead();
}
