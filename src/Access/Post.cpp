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

class PostPrivate :public AccessPrivate < Post ,Post::Proc, Post::Task>
{
public:
	explicit PostPrivate(Post *post):
		AccessPrivate <Post, Post::Proc, Post::Task>(post)
	{
	}

	void forward()
	{
		Q_Q(Post);
		Post::Task &task = queue.head();
		if (!task.processer){
			task.state = QNetworkReply::ProtocolUnknownError;
			emit q->stateChanged(task.state);
			q->dequeue();
			return;
		}
		if (task.state == 0){
			Danmaku::instance()->appendToPool(task.target->source, &task.comment);
			task.processer->process(nullptr);
		}
		else{
			emit q->stateChanged(task.state);
			QNetworkAccessManager &m = manager;
			remain.insert(task.data.isEmpty() ? m.get(task.request) : m.post(task.request, task.data));
		}
	}
};

Post::Post(QObject *parent) :
QObject(parent), d_ptr(new PostPrivate(this))
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
			emit stateChanged(task.state = Code);
			forward();
			break;
		}
		case Code:{
			int code = QString(reply->readAll()).toInt();
			if (code > 0){
				emit stateChanged(task.state = None);
			}
			else{
				emit stateChanged(task.state = code);
			}
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

	auto ddProcess = [this](QNetworkReply *reply){
		Q_D(Post);
		Task &task = d->queue.head();
		switch (task.state){
		case None:{
			QString api("http://api.%1/api/v1/comment/%2");
			api = api.arg(Utils::customUrl(Utils::AcPlay));
			api = api.arg(QFileInfo(task.target->source).baseName());
			task.request.setUrl(api);
			task.request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
			const Comment &c = task.comment;
			QJsonObject params;
			params["Token"] = 0;
			params["Time"] = c.time / 1000.0;
			params["Mode"] = c.mode;
			params["Color"] = c.color;
			params["TimeStamp"] = QDateTime::currentMSecsSinceEpoch();
			params["Pool"] = 0;
			params["UId"] = 0;
			params["CId"] = 0;
			params["Message"] = c.string;
			task.data= QJsonDocument(params).toJson();
			emit stateChanged(task.state = Code);
			forward();
			break;
		}
		case Code:{
			const QJsonObject &result = QJsonDocument::fromJson(reply->readAll()).object();
			if (result["Success"].toBool()){
				emit stateChanged(task.state = None);
			}
			else{
				emit stateChanged(task.state = QNetworkReply::UnknownNetworkError);
			}
			dequeue();
			break;
		}
		}

	};
	auto ddRegular = [](QString code){
		static QRegularExpression r("d(d\\d*)?");
		r.setPatternOptions(QRegularExpression::CaseInsensitiveOption);
		return r.match(code).capturedLength() == code.length();
	};
	d->pool.append({ ddRegular, 0, ddProcess });

	connect(this, &Post::stateChanged, [this](int code){
		switch (code){
		case None:
		case Code:
			break;
		default:{
			Q_D(Post);
			if (!d->tryNext()){
				emit errorOccured(code);
			}
			break;
		}
		}
	});
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

int Post::size()
{
	Q_D(Post);
	return d->size();
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

void Post::forward()
{
	Q_D(Post);
	d->forward();
}
