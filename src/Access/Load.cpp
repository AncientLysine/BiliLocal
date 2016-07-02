/*=======================================================================
*
*   Copyright (C) 2013-2015 Lysine.
*
*   Filename:    Load.cpp
*   Time:        2014/04/22
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
#include "Load.h"
#include "AccessPrivate.h"
#include "Parse.h"
#include "../Config.h"
#include "../Local.h"
#include "../Utils.h"
#include "../Model/Danmaku.h"
#include "../Player/APlayer.h"
#include <algorithm>

class LoadPrivate : public AccessPrivate<Load, Load::Proc, Load::Task>
{
public:
	QStandardItemModel *model;

	explicit LoadPrivate(Load *load) :
		AccessPrivate<Load, Load::Proc, Load::Task>(load)
	{
		Q_Q(Load);
		model = new QStandardItemModel(q);
	}
};

namespace
{
	std::function<bool(QString &)> getRegular(QRegularExpression ex)
	{
		return [ex](QString &code){
			auto iter = ex.globalMatch(code);
			code.clear();
			while (iter.hasNext()){
				code = iter.next().captured();
			}
			return code.length() > 2;
		};
	}
}

Load::Load(QObject *parent)
	: QObject(parent), d_ptr(new LoadPrivate(this))
{
	Q_D(Load);
	setObjectName("Load");

	auto avProcess = [this](QNetworkReply *reply){
		Q_D(Load);
		Task &task = d->queue.head();
		int sharp = task.code.indexOf(QRegularExpression("[#_]"));
		switch (task.state){
		case None:
		{
			QString i = task.code.mid(2, sharp - 2);
			QString p = sharp == -1 ? QString() : task.code.mid(sharp + 1);
			QString url("http://www.%1/video/av%2/");
			url = url.arg(Utils::customUrl(Utils::Bilibili)).arg(i);
			if (!p.isEmpty()){
				url += QString("index_%1.html").arg(p);
			}
			forward(QNetworkRequest(url), Page);
			break;
		}
		case Page:
		{
			d->model->clear();
			QString api, id, video(reply->readAll());
			int part = video.indexOf("<select");
			if (part != -1 && sharp == -1){
				QRegularExpression r("(?<=>).*?(?=</option>)");
				QStringRef list(&video, part, video.indexOf("</select>", part) - part);
				QRegularExpressionMatchIterator i = r.globalMatch(list);
				api = "http://www.%1/video/%2/index_%3.html";
				api = api.arg(Utils::customUrl(Utils::Bilibili));
				while (i.hasNext()){
					int index = d->model->rowCount() + 1;
					QStandardItem *item = new QStandardItem;
					item->setData(QUrl(api.arg(task.code).arg(index)), UrlRole);
					item->setData((task.code + "#%1").arg(index), StrRole);
					item->setData(Page, NxtRole);
					item->setData(Utils::decodeXml(i.next().captured()), Qt::EditRole);
					d->model->appendRow(item);
				}
			}
			if (d->model->rowCount() > 0){
				emit stateChanged(task.state = Part);
			}
			else{
				QRegularExpression r = QRegularExpression("cid[=\":]*\\d+", QRegularExpression::CaseInsensitiveOption);
				QRegularExpressionMatchIterator i = r.globalMatch(video);
				while (i.hasNext()){
					QString m = i.next().captured();
					m = QRegularExpression("\\d+").match(m).captured();
					if (id.isEmpty()){
						id = m;
					}
					else if (id != m){
						id.clear();
						break;
					}
				}
				if (!id.isEmpty()){
					api = "http://comment.%1/%2.xml";
					api = api.arg(Utils::customUrl(Utils::Bilibili));
					forward(QNetworkRequest(api.arg(id)), File);
				}
				else{
					emit stateChanged(203);
					dequeue();
				}
			}
			break;
		}
		case File:
		{
			dumpDanmaku(reply->readAll(), Utils::Bilibili, false);
			emit stateChanged(task.state = None);
			dequeue();
			break;
		}
		}
	};
	auto avRegular = [](QString &code){
		code.remove(QRegularExpression("/index(?=_\\d+\\.html)"));
		QRegularExpression r("a(v(\\d+([#_])?(\\d+)?)?)?");
		r.setPatternOptions(QRegularExpression::CaseInsensitiveOption);
		return getRegular(r)(code);
	};
	d->pool.append({ avRegular, 0, avProcess });

	auto bbProcess = [this, avProcess](QNetworkReply *reply) {
		Q_D(Load);
		Task &task = d->queue.head();
		switch (task.state) {
		case None:
		{
			QString i = task.code.mid(2);
			QString u = "http://www.%1/bangumi/i/%2/";
			u = u.arg(Utils::customUrl(Utils::Bilibili)).arg(i);
			forward(QNetworkRequest(u), Page);
			break;
		}
		case Page:
		{
			d->model->clear();
			QString page(reply->readAll());
			QStringList list = page.split("<li data-index");

			if (list.size() < 2) {
				emit stateChanged(task.state = None);
				dequeue();
				break;
			}

			list.removeFirst();
			QListIterator<QString> iter(list);
			iter.toBack();
			while (iter.hasPrevious()) {
				QRegularExpression r;
				const QString &i = iter.previous();
				r.setPattern("(?<=href=\")[^\"]+");
				QString c = r.match(i).captured();
				fixCode(c);
				r.setPattern("(?<=<span>).+(?=</span>)");
				QString t = Utils::decodeXml(r.match(i).captured());

				QStandardItem *item = new QStandardItem;
				item->setData(c, StrRole);
				item->setData(None, NxtRole);
				item->setData(t, Qt::EditRole);
				d->model->appendRow(item);
			}
			emit stateChanged(task.state = Part);
		}
		}
	};

	auto bbRegular = [](QString &code) {
		code.replace(QRegularExpression("bangumi/i/(?=\\d+)"), "bb");
		QRegularExpression r("b(b(\\d+)?)?");
		r.setPatternOptions(QRegularExpression::CaseInsensitiveOption);
		return getRegular(r)(code);
	};
	d->pool.append({ bbRegular, 0, bbProcess });

	auto acProcess = [this](QNetworkReply *reply){
		Q_D(Load);
		Task &task = d->queue.head();
		int sharp = task.code.indexOf(QRegularExpression("[#_]"));
		switch (task.state){
		case None:
		{
			QString i = task.code.mid(2, sharp - 2);
			QString p = sharp == -1 ? QString() : task.code.mid(sharp + 1);
			QString url("http://www.%1/v/ac%2");
			url = url.arg(Utils::customUrl(Utils::AcFun)).arg(i);
			if (!p.isEmpty()){
				url += QString("_%1").arg(p);
			}
			forward(QNetworkRequest(url), Page);
			break;;
		}
		case Page:
		{
			d->model->clear();
			QRegularExpressionMatchIterator match = QRegularExpression("data-vid.*?</a>").globalMatch(reply->readAll());
			while (match.hasNext()){
				QStandardItem *item = new QStandardItem;
				QString part = match.next().captured();
				QRegularExpression r;
				r.setPattern("(?<=>)[^>]+?(?=</a>)");
				item->setData(Utils::decodeXml(r.match(part).captured()), Qt::EditRole);
				r.setPattern("(?<=data-vid=\").+?(?=\")");
				QString next("http://static.comment.%1/V2/%2?pageSize=1000&pageNo=1");
				next = next.arg(Utils::customUrl(Utils::AcFun)).arg(r.match(part).captured());
				item->setData(next, UrlRole);
				item->setData((task.code + "#%1").arg(d->model->rowCount() + 1), StrRole);
				item->setData(File, NxtRole);
				d->model->appendRow(item);
			}
			if (sharp == -1 && d->model->rowCount() >= 2){
				emit stateChanged(task.state = Part);
			}
			else{
				int i = sharp == -1 ? 0 : task.code.mid(sharp + 1).toInt() - 1;
				if (i >= 0 && i < d->model->rowCount()){
					forward(QNetworkRequest(d->model->item(i)->data(UrlRole).toUrl()), File);
				}
				else{
					emit stateChanged(203);
					dequeue();
				}
			}
			break;
		}
		case File:
		{
			QByteArray data = reply->readAll();
			if (data != "[[],[],[]]"){
				QNetworkRequest &request = task.request;
				QUrl url = request.url();
				int page = QUrlQuery(url).queryItemValue("pageNo").toInt();
				url.setQuery(QString());
				request.setUrl(url);
				dumpDanmaku(data, Utils::AcFun, false);
				QUrlQuery query;
				query.addQueryItem("pageSize", "1000");
				query.addQueryItem("pageNo", QString::number(page + 1));
				url.setQuery(query);
				request.setUrl(url);
				forward(request, File);
			}
			else{
				emit stateChanged(task.state = None);
				dequeue();
			}
			break;
		}
		}
	};
	auto acRegular = getRegular(QRegularExpression("a(c(\\d+([#_])?(\\d+)?)?)?", QRegularExpression::CaseInsensitiveOption));
	d->pool.append({ acRegular, 0, acProcess });

	auto abProcess = [this, acProcess](QNetworkReply *reply){
		Q_D(Load);
		Task &task = d->queue.head();
		int sharp = task.code.indexOf(QRegularExpression("[#_]"));
		switch (task.state){
		case None:
		{
			QString url("http://www.%1/bangumi/video/page?bangumiId=%2&pageSize=30&pageNo=%3&order=2");
			url = url.arg(Utils::customUrl(Utils::AcFun)).arg(task.code.mid(2, sharp - 2));
			url = url.arg(sharp == -1 ? 1 : (task.code.mid(sharp + 1).toInt() - 1) / 30 + 1);
			forward(QNetworkRequest(url), Page);
			break;
		}
		case Page:
		{
			if (sharp != -1){
				QJsonObject data = QJsonDocument::fromJson(reply->readAll()).object()["data"].toObject();
				int i = task.code.mid(sharp + 1).toInt();
				if (i > 0){
					i = (i - 1) % 30;
				}
				else{
					i = data["totalCount"].toInt();
					if (i > 30){
						task.code = task.code.left(sharp) + QString("#%1").arg(i);
						task.state = None;
						task.processer->process(nullptr);
						break;
					}
				}
				QJsonArray list = data["list"].toArray();
				if (i < 0 || i >= list.size()){
					emit stateChanged(203);
					dequeue();
					break;
				}
				QString head("http://static.comment.%1/V2/%2?pageSize=1000&pageNo=1");
				head = head.arg(Utils::customUrl(Utils::AcFun));
				head = head.arg(list[i].toObject()["danmakuId"].toString());
				forward(QNetworkRequest(head), File);
				break;
			}
			else{
				d->model->clear();
			}
		}
		case Part:
		{
			QJsonObject info = QJsonDocument::fromJson(reply->readAll()).object();
			if (!info["success"].toBool() && d->model->rowCount() == 0){
				emit stateChanged(info["status"].toInt());
				dequeue();
			}
			QJsonObject data = info["data"].toObject();
			for (const QJsonValue &value : data["list"].toArray()){
				QStandardItem *item = new QStandardItem;
				QJsonObject data = value.toObject();
				item->setData(data["title"].toString(), Qt::EditRole);
				QString head("http://static.comment.%1/V2/%2?pageSize=1000&pageNo=1");
				head = head.arg(Utils::customUrl(Utils::AcFun)).arg(data["danmakuId"].toString());
				item->setData(head, UrlRole);
				item->setData((task.code + "#%1").arg(d->model->rowCount() + 1), StrRole);
				item->setData(File, NxtRole);
				d->model->appendRow(item);
			}
			if (task.state != Part){
				emit stateChanged(task.state = Part);
			}
			if (data["pageNo"].toInt() < data["totalPage"].toInt()){
				QUrl url = reply->request().url();
				auto arg = QUrlQuery(url).queryItems();
				for (auto &p : arg){
					if (p.first == "pageNo"){
						p.second = QString::number(p.second.toInt() + 1);
						break;
					}
				}
				QUrlQuery query;
				query.setQueryItems(arg);
				url.setQuery(query);
				d->remain.insert(d->manager.get(QNetworkRequest(url)));
			}
			break;
		}
		case File:
		{
			acProcess(reply);
			break;
		}
		}
	};
	auto abRegular = getRegular(QRegularExpression("a(b(\\d+([#_])?(\\d+)?)?)?", QRegularExpression::CaseInsensitiveOption));
	d->pool.append({ abRegular, 0, abProcess });

	auto ccProcess = [this](QNetworkReply *reply){
		Q_D(Load);
		Task &task = d->queue.head();
		int sharp = task.code.indexOf(QRegularExpression("[#_]"));
		switch (task.state){
		case None:
		{
			QString i = task.code.mid(2, sharp - 2);
			QString p = sharp == -1 ? QString() : task.code.mid(sharp + 1);
			QString url("http://www.%1/play/h%2/");
			url = url.arg(Utils::customUrl(Utils::TuCao)).arg(i);
			if (!p.isEmpty()){
				url += QString("#%1").arg(p);
			}
			forward(QNetworkRequest(url), Page);
			break;
		}
		case Page:
		{
			QString page = reply->readAll();
			d->model->clear();
			QRegularExpressionMatch m;
			QRegularExpression r("(?<=<li>)[^<]*(?=</li>)");
			m = r.match(page, page.indexOf("<ul id=\"player_code\""));
			QStringList list = m.captured().split("**");
			m = r.match(page, m.capturedEnd());
			QString code = m.captured();
			for (const QString &iter : list){
				QStandardItem *item = new QStandardItem;
				item->setData(iter.mid(iter.indexOf('|') + 1), Qt::EditRole);
				QString api("http://www.%1/index.php?m=mukio&c=index&a=init&playerID=%2");
				api = api.arg(Utils::customUrl(Utils::TuCao)).arg((code + "-%1").arg(d->model->rowCount()));
				item->setData(api, UrlRole);
				item->setData((task.code + "#%1").arg(d->model->rowCount() + 1), StrRole);
				item->setData(File, NxtRole);
				d->model->appendRow(item);
			}
			if (sharp == -1 && d->model->rowCount() >= 2){
				emit stateChanged(task.state = Part);
			}
			else{
				int i = sharp == -1 ? 0 : task.code.mid(sharp + 1).toInt() - 1;
				if (i >= 0 && i < d->model->rowCount()){
					forward(QNetworkRequest(d->model->item(i)->data(UrlRole).toUrl()), File);
				}
				else{
					emit stateChanged(203);
					dequeue();
				}
			}
			break;
		}
		case File:
		{
			dumpDanmaku(reply->readAll(), Utils::TuCao, false);
			emit stateChanged(task.state = None);
			dequeue();
			break;
		}
		}
	};
	auto ccRegular = [](QString &code){
		code.replace(QRegularExpression("[Hh](?=\\d)"), "cc");
		QRegularExpression r("c(c(\\d+([#_])?(\\d+)?)?)?");
		r.setPatternOptions(QRegularExpression::CaseInsensitiveOption);
		return getRegular(r)(code);
	};
	d->pool.append({ ccRegular, 0, ccProcess });

	d->pool.append(Proc());
	Proc *directProc = &d->pool.last();
	directProc->process = [this](QNetworkReply *reply){
		Q_D(Load);
		Task &task = d->queue.head();
		switch (task.state){
		case None:
		{
			QUrl url = QUrl::fromUserInput(task.code);
			task.request.setUrl(url);
			task.state = File;
			forward();
			break;
		}
		case File:
		{
			Record load;
			QUrl url = reply->url();
			QByteArray data(reply->readAll());
			load.source = url.url();
			load.access = url.isLocalFile() ? url.toLocalFile() : load.source;
			load.string = QFileInfo(task.code).fileName();
			load.delay = task.delay;
			QString head = Utils::decodeTxt(data.left(512));
			if (head.startsWith("[Script Info]")){
				load.danmaku = Parse::parseComment(data, Utils::ASS);
			}
			else if (!head.startsWith("<?xml")){
				load.danmaku = Parse::parseComment(data, Utils::AcFun);
			}
			else if (head.indexOf("<packet>") != -1){
				load.danmaku = Parse::parseComment(data, Utils::Niconico);
			}
			else if (head.indexOf("<i>") != -1){
				load.danmaku = Parse::parseComment(data, Utils::Bilibili);
				QString i = QRegularExpression("(?<=<chatid>)\\d+(?=</chatid>)").match(head).captured();
				if (!i.isEmpty()){
					load.source = "http://comment.%1/%2.xml";
					load.source = load.source.arg(Utils::customUrl(Utils::Bilibili)).arg(i);
				}
			}
			else if (head.indexOf("<c>") != -1){
				load.danmaku = Parse::parseComment(data, Utils::AcfunLocalizer);
			}
			if (load.delay != 0){
				for (Comment &c : load.danmaku){
					c.time += load.delay;
				}
			}
			lApp->findObject<Danmaku>()->append(&load);
			emit stateChanged(task.state = None);
			dequeue();
			break;
		}
		}
	};
	directProc->priority = -100;
	directProc->regular = [this, directProc](QString &code){
		if (code.startsWith("full?") || code.startsWith("hist?")){
			code.clear();
			return false;
		}
		QUrl u = QUrl::fromUserInput(code);
		if (!u.host().isEmpty() && !u.path().isEmpty()){
			return true;
		}
		if (u.isLocalFile() && QFileInfo(u.toLocalFile()).exists()){
			return true;
		}
		code.clear();
		return false;
	};

	auto fullBiProcess = [this](QNetworkReply *reply){
		Q_D(Load);
		Task &task = d->queue.head();
		switch (task.state) {
		case None:
		{
			emit progressChanged(0);
			QString api("http://comment.%1/rolldate,%2");
			api = api.arg(Utils::customUrl(Utils::Bilibili));
			task.code = QUrlQuery(task.code.mid(5)).queryItemValue("source");
			forward(QNetworkRequest(api.arg(QFileInfo(task.code).baseName())), Page);
			break;
		}
		case Page:
		{
			QByteArray data = reply->readAll();
			QJsonArray date = QJsonDocument::fromJson(data).array();
			if (date.isEmpty()) {
				emit stateChanged(203);
				dequeue();
				break;
			}
			QJsonObject head = date.first().toObject();
			QString url("http://comment.%1/dmroll,%2,%3");
			url = url.arg(Utils::customUrl(Utils::Bilibili));
			url = url.arg(head["timestamp"].toVariant().toInt());
			url = url.arg(QFileInfo(task.code).baseName());
			QNetworkRequest request(url);
			request.setAttribute(QNetworkRequest::User, data);
			forward(request, Code);
			break;
		}
		case Code:
		{
			QByteArray data = task.request.attribute(QNetworkRequest::User).toByteArray();
			QJsonArray date = QJsonDocument::fromJson(data).array();
			QMap<int, int> count;
			for (auto iter : date) {
				QJsonObject item = iter.toObject();
				count[item["timestamp"].toVariant().toInt()] += item["new"].toVariant().toInt();
			}

			data = reply->readAll();
			if (count.size() >= 2) {
				int max = QRegularExpression("(?<=\\<max_count\\>).+(?=\\</max_count\\>)").match(data).captured().toInt();
				int now = 0;

				auto getHistory = [d, &count, &task](int date) {
					QString url("http://comment.%1/dmroll,%2,%3");
					url = url.arg(Utils::customUrl(Utils::Bilibili));
					url = url.arg(date);
					url = url.arg(QFileInfo(task.code).baseName());
					return d->manager.get(QNetworkRequest(url));
				};

				for (auto iter = count.begin() + 1;; ++iter) {
					now += iter.value();
					if (iter + 1 == count.end()) {
						d->remain += getHistory(iter.key());
						break;
					}
					else if (now + (iter + 1).value() > max) {
						d->remain += getHistory(iter.key());
						now = 0;
					}
				}

				auto pool = QSharedPointer<QVector<Parse::ResultDelegate>>::create();
				pool->append(Parse::parseComment(data, Utils::Bilibili));

				double total = d->remain.size() + 2;
				for (QNetworkReply *iter : d->remain) {
					connect(iter, &QNetworkReply::finished, [=, &task]() {
						QByteArray data = iter->readAll();
						pool->append(Parse::parseComment(data, Utils::Bilibili));
						switch (iter->error()) {
						case QNetworkReply::NoError:
							emit progressChanged((total - d->remain.size()) / total);
						case QNetworkReply::OperationCanceledError:
							if (d->remain.isEmpty() && !pool->empty()) {
								Record load;
								load.full = true;
								for (auto &iter : *pool) {
									load.danmaku.append(iter);
								}
								load.source = task.code;
								lApp->findObject<Danmaku>()->append(&load);
								emit stateChanged(task.state = None);
								dequeue();
							}
						default:
							break;
						}
					});
				}

				emit progressChanged(2 / total);
				emit stateChanged(task.state = File);
				break;
			}
			else {
				emit progressChanged(1);
				dumpDanmaku(data, Utils::Bilibili, true);
				emit stateChanged(task.state = None);
				dequeue();
				break;
			}
		}
		}
	};

	auto fullBiRegular = QRegularExpression("^full\\?source=http://comment\\.bilibili\\.com/\\d+\\.xml$");
	fullBiRegular.setPatternOptions(QRegularExpression::CaseInsensitiveOption);
	d->pool.append({ getRegular(fullBiRegular), 100, fullBiProcess });

	auto histBiProcess = [this](QNetworkReply *reply){
		Q_D(Load);
		Task &task = d->queue.head();
		switch (task.state){
		case None:
		{
			QUrlQuery query(task.code.mid(5));
			task.code = query.queryItemValue("source");
			QString cid = QFileInfo(task.code).baseName();
			QString dat = query.queryItemValue("date");
			QString url;
			QNetworkRequest request;
			if (dat != "0" && dat.toUInt() != QDateTime(QDate::currentDate()).toTime_t()){
				url = QString("http://comment.%1/dmroll,%2,%3");
				url = url.arg(Utils::customUrl(Utils::Bilibili));
				url = url.arg(dat).arg(cid);
				int limit = QDateTime(QDateTime::fromTime_t(dat.toInt()).date().addDays(1)).toTime_t();
				request.setAttribute(QNetworkRequest::User, limit);
			}
			else{
				url = QString("http://comment.%1/%2.xml").arg(Utils::customUrl(Utils::Bilibili));
				url = url.arg(cid);
			}
			request.setUrl(url);
			forward(request, File);
			break;
		}
		case File:
		{
			Record load;
			load.danmaku = Parse::parseComment(reply->readAll(), Utils::Bilibili);
			load.source = task.code;
			for (Record &iter : lApp->findObject<Danmaku>()->getPool()){
				if (iter.source == load.source){
					iter.full = false;
					iter.danmaku.clear();
					iter.limit = 1;
					break;
				}
			}
			load.limit = task.request.attribute(QNetworkRequest::User).toInt();
			lApp->findObject<Danmaku>()->append(&load);
			emit stateChanged(task.state = None);
			dequeue();
			break;
		}
		}
	};
	auto histBiRegular = QRegularExpression("^hist\\?source=http://comment\\.bilibili\\.com/\\d+\\.xml&date=\\d+$");
	histBiRegular.setPatternOptions(QRegularExpression::CaseInsensitiveOption);
	d->pool.append({ getRegular(histBiRegular), 100, histBiProcess });

	connect(this, &Load::stateChanged, [this](int code){
		switch (code){
		case None:
		case Page:
		case Part:
		case Code:
		case File:
			break;
		default:
		{
			Q_D(Load);
			if (!d->tryNext()){
				emit errorOccured(code);
			}
			break;
		}
		}
	});
}

Load::~Load()
{
	delete d_ptr;
}

Load::Task Load::codeToTask(QString code)
{
	Task task;
	task.processer = getProc(code);
	task.code = code;
	APlayer *aplayer = lApp->findObject<APlayer>();
	task.delay = aplayer->getState() != APlayer::Stop&&Config::getValue("/Load/Delay", false) ? aplayer->getTime() : 0;
	return task;
}

QStandardItemModel *Load::getModel()
{
	Q_D(Load);
	return d->model;
}

void Load::addProc(const Load::Proc *proc)
{
	Q_D(Load);
	d->addProc(proc);
}

const Load::Proc *Load::getProc(QString code)
{
	Q_D(Load);
	return d->getProc(code);
}

void Load::fixCode(QString &code)
{
	Q_D(Load);
	QString fixed;
	const Proc *p = nullptr;
	for (const Proc &i : d->pool){
		QString t(code);
		if (i.regular(t)){
			if (!p || i.priority > p->priority){
				fixed = t;
				p = &i;
			}
		}
		else if (!p&&t.length() > fixed.length()){
			fixed = t;
		}
	}
	code = fixed;
}

bool Load::canLoad(QString code)
{
	return getProc(code);
}

namespace
{
	QString toFull(QString source)
	{
		QUrlQuery query;
		query.addQueryItem("source", source);
		return "full?" + query.toString();
	}
}

bool Load::canFull(const Record *record)
{
	return !record->full&&getProc(toFull(record->source));
}

bool Load::canHist(const Record *record)
{
	QUrlQuery query;
	query.addQueryItem("source", record->source);
	query.addQueryItem("date", "0");
	return getProc("hist?" + query.toString());
}

void Load::loadDanmaku(QString code)
{
	const Task &task = codeToTask(code);
	if (enqueue(task) && Config::getValue("/Load/Clear", false)){
		lApp->findObject<Danmaku>()->clear();
	}
}

void Load::loadDanmaku(const QModelIndex &index)
{
	if (index.isValid()){
		QVariant s = index.data(StrRole);
		if (s.isValid()){
			Task task;
			task.code = s.toString();
			task.processer = getProc(task.code);
			task.request = QNetworkRequest(index.data(UrlRole).toUrl());
			task.state = index.data(NxtRole).toInt();
			APlayer *aplayer = lApp->findObject<APlayer>();
			task.delay = aplayer->getState() != APlayer::Stop&&Config::getValue("/Load/Delay", false) ? aplayer->getTime() : 0;
			enqueue(task);
		}
	}
	else{
		Q_D(Load);
		int c = d->model->rowCount();
		for (int i = 0; i < c; ++i){
			loadDanmaku(d->model->index(i, 0));
		}
	}
}

void Load::fullDanmaku(const Record *record)
{
	const Task &task = codeToTask(toFull(record->source));
	enqueue(task);
}

void Load::loadHistory(const Record *record, QDate date)
{
	QUrlQuery query;
	query.addQueryItem("source", record->source);
	query.addQueryItem("date", QString::number(date.isValid() ? QDateTime(date).toTime_t() : 0));
	const Task &task = codeToTask("hist?" + query.toString());
	enqueue(task);
}

void Load::dumpDanmaku(const QVector<Comment> *data, bool full)
{
	Q_D(Load);
	Task &task = d->queue.head();
	Record load;
	load.full = full;
	load.source = task.request.url().url();
	load.string = task.code;
	load.access = task.code;
	load.delay = task.delay;
	load.danmaku = *data;
	if (load.delay != 0){
		for (Comment &c : load.danmaku){
			c.time += load.delay;
		}
	}
	lApp->findObject<Danmaku>()->append(&load);
}

void Load::dumpDanmaku(const QByteArray &data, int site, bool full)
{
	QVector<Comment> list = Parse::parseComment(data, (Utils::Site)site);
	dumpDanmaku(&list, full);
}

void Load::forward()
{
	Q_D(Load);
	d->forward();
}

void Load::forward(QNetworkRequest request)
{
	Task &task = *getHead();
	task.request = request;
	forward();
}

void Load::forward(QNetworkRequest request, int state)
{
	Task &task = *getHead();
	task.request = request; task.state = state;
	forward();
}

void Load::dequeue()
{
	Q_D(Load);
	d->dequeue();
}

bool Load::enqueue(const Load::Task &task)
{
	Q_D(Load);
	return d->enqueue(task);
}

Load::Task *Load::getHead()
{
	Q_D(Load);
	return d->getHead();
}
