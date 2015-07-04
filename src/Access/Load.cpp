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

#include "Load.h"
#include "AccessPrivate.h"
#include "../Config.h"
#include "../Utils.h"
#include "../Model/Danmaku.h"
#include "../Player/APlayer.h"

Load *Load::ins = nullptr;

Load *Load::instance()
{
	return ins ? ins : new Load(qApp);
}

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
	QString decodeAsText(const QByteArray &data)
	{
		QTextCodec *codec = QTextCodec::codecForUtfText(data, nullptr);
		if (!codec){
			QByteArray name;
			QByteArray head = data.left(512).toLower();
			if (head.startsWith("<?xml")){
				int pos = head.indexOf("encoding=");
				if (pos >= 0){
					pos += 9;
					if (pos < head.size()){
						auto c = head.at(pos);
						if ('\"' == c || '\'' == c){
							++pos;
							name = head.mid(pos, head.indexOf(c, pos) - pos);
						}
					}
				}
			}
			else{
				int pos = head.indexOf("charset=", head.indexOf("meta "));
				if (pos >= 0){
					pos += 8;
					int end = pos;
					while (++end < head.size()){
						auto c = head.at(end);
						if (c == '\"' || c == '\'' || c == '>'){
							name = head.mid(pos, end - pos);
							break;
						}
					}
				}
			}
			codec = QTextCodec::codecForName(name);
		}
		if (!codec){
			codec = QTextCodec::codecForLocale();
		}
		return codec->toUnicode(data);
	}

	QList<Comment> parseComment(QByteArray data, Utils::Site site)
	{
		QList<Comment> list;
		switch (site){
		case Utils::Bilibili:
		case Utils::TuCao:
		{
			QString xml = decodeAsText(data);
			QVector<QStringRef> l = xml.splitRef("<d p=");
			l.removeFirst();
			for (const QStringRef &item : l){
				const QVector<QStringRef> &args = item.mid(1, item.indexOf(item.at(0), 1) - 1).split(',');
				if (args.size() <= 4){
					continue;
				}
				Comment comment;
				comment.time = args[0].toDouble() * 1000 + 0.5;
				comment.date = args[4].toInt();
				comment.mode = args[1].toInt();
				comment.font = args[2].toInt();
				comment.color = args[3].toInt();
				comment.sender = args.size() <= 6 ? QString() : args[6].toString();
				const QStringRef &last = args[args.size() <= 6 ? 4 : 6];
				int sta = item.indexOf('>', last.position() + last.size() - item.position()) + 1;
				int len = item.lastIndexOf('<') - sta;
				comment.string = Utils::decodeXml(item.mid(sta, len).toString(), true);
				list.append(comment);
			}
			break;
		}
		case Utils::AcFun:
		{
			QQueue<QJsonArray> queue;
			queue.append(QJsonDocument::fromJson(data).array());
			while (!queue.isEmpty()){
				for (const QJsonValue &i : queue.head()){
					if (i.isArray()){
						queue.append(i.toArray());
					}
					else{
						QJsonObject item = i.toObject();
						QString c(item["c"].toString()), m(item["m"].toString());
						const QVector<QStringRef> &args = c.splitRef(',');
						if (args.size() < 6){
							continue;
						}
						Comment comment;
						comment.time = args[0].toDouble() * 1000 + 0.5;
						comment.date = args[5].toInt();
						comment.mode = args[2].toInt();
						comment.font = args[3].toInt();
						comment.color = args[1].toInt();
						comment.sender = args[4].toString();
						comment.string = m;
						list.append(comment);
					}
				}
				queue.dequeue();
			}
			break;
		}
		case Utils::AcfunLocalizer:
		{
			QString xml = decodeAsText(data);
			QVector<QStringRef> l = xml.splitRef("<l i=\"");
			l.removeFirst();
			for (const QStringRef &item : l){
				const QVector<QStringRef> &args = item.left(item.indexOf("\"")).split(',');
				if (args.size() < 6){
					continue;
				}
				Comment comment;
				comment.time = args[0].toDouble() * 1000 + 0.5;
				comment.date = args[5].toInt();
				comment.mode = 1;
				comment.font = 25;
				comment.color = args[2].toInt();
				comment.sender = args[4].toString();
				int sta = item.indexOf("<![CDATA[") + 9;
				int len = item.indexOf("]]>", sta) - sta;
				comment.string = Utils::decodeXml(item.mid(sta, len).toString(), true);
				list.append(comment);
			}
			break;
		}
		case Utils::Niconico:
		{
			QStringList l = decodeAsText(data).split("<chat ");
			l.removeFirst();
			for (const QString &item : l){
				Comment comment;
				QString key, val;
				/* 0 wait for key
				 * 1 wait for left quot
				 * 2 wait for value
				 * 3 wait for comment
				 * 4 finsihed */
				int state = 0;
				QMap<QString, QString> args;
				for (const QChar &c : item){
					switch (state){
					case 0:
						if (c == '='){
							state = 1;
						}
						else if (c == '>'){
							state = 3;
						}
						else if (c != ' '){
							key.append(c);
						}
						break;
					case 1:
						if (c == '\"'){
							state = 2;
						}
						break;
					case 2:
						if (c == '\"'){
							state = 0;
							args.insert(key, val);
							key = val = QString();
						}
						else{
							val.append(c);
						}
						break;
					case 3:
						if (c == '<'){
							state = 4;
						}
						else{
							comment.string.append(c);
						}
						break;
					}
				}
				if (state != 4){
					continue;
				}
				comment.time = args["vpos"].toLongLong() * 10;
				comment.date = args["date"].toLongLong();
				QStringList ctrl = args["mail"].split(' ', QString::SkipEmptyParts);
				comment.mode = ctrl.contains("shita") ? 4 : (ctrl.contains("ue") ? 5 : 1);
				comment.font = ctrl.contains("small") ? 15 : (ctrl.contains("big") ? 36 : 25);
				comment.color = 0xFFFFFF;
				for (const QString &name : ctrl){
					QColor color(name);
					if (color.isValid()){
						comment.color = color.rgb();
						break;
					}
				}
				comment.sender = args["user_id"];
				list.append(comment);
			}
			break;
		}
		case Utils::ASS:
		{
			QString xml = decodeAsText(data);
			int pos = 0, len;
			pos = xml.indexOf("PlayResY:") + 9;
			len = xml.indexOf('\n', pos) + 1 - pos;
			int vertical = xml.midRef(pos, len).trimmed().toInt();
			QVector<QStringRef> ref;
			pos = xml.indexOf("Format:", pos) + 7;
			len = xml.indexOf('\n', pos) + 1 - pos;
			ref = xml.midRef(pos, len).split(',');
			int name = -1, size = -1;
			for (int i = 0; i < ref.size(); ++i){
				const QStringRef &item = ref[i].trimmed();
				if (item == "Name"){
					name = i;
				}
				else if (item == "Fontsize"){
					size = i;
				}
			}
			if (name < 0 || size < 0){
				break;
			}
			pos += len;
			len = xml.indexOf("Format:", pos) - pos;
			ref = xml.midRef(pos, len).split("Style:", QString::SkipEmptyParts);
			QMap<QString, int> style;
			for (const QStringRef &item : ref){
				const auto &args = item.split(',');
				style[args[name].trimmed().toString()] = args[size].toInt();
			}
			pos += len;
			pos = xml.indexOf("Format:", pos) + 7;
			len = xml.indexOf("\n", pos) + 1 - pos;
			ref = xml.midRef(pos, len).split(',');
			int text = -1, font = -1, time = -1;
			for (int i = 0; i < ref.size(); ++i){
				const QStringRef &item = ref[i].trimmed();
				if (item == "Text"){
					text = i;
				}
				else if (item == "Start"){
					time = i;
				}
				else if (item == "Style"){
					font = i;
				}
			}
			if (text < 0 || font < 0 || time < 0){
				break;
			}
			qint64 dat = QDateTime::currentDateTime().toTime_t();
			pos += len;
			ref = xml.midRef(pos).split("Dialogue:",QString::SkipEmptyParts);
			for (const QStringRef &item : ref){
				const auto &args = item.split(',');
				Comment comment;
				comment.date = dat;
				QString t;
				t = args[time].trimmed().toString();
				comment.time = 1000 * Utils::evaluate(t);
				t = args[font].trimmed().toString();
				comment.font = style[t];
				t = item.mid(args[text].position()-item.position()).trimmed().toString();
				int split = t.indexOf("}") + 1;
				comment.string = t.midRef(split).trimmed().toString();
				const auto &m = t.midRef(1, split - 2).split('\\', QString::SkipEmptyParts);
				for (const QStringRef &i : m){
					if (i.startsWith("fs")){
						comment.font = i.mid(2).toInt();
					}
					else if (i.startsWith("c&H", Qt::CaseSensitive)){
						comment.color = i.mid(3).toInt(nullptr, 16);
					}
					else if (i.startsWith("c")){
						comment.color = i.mid(1).toInt();
					}
					else if (i.startsWith("move")){
						const auto &p = i.mid(5, i.length() - 6).split(',');
						if (p.size() == 4){
							comment.mode = p[0].toInt() > p[2].toInt() ? 1 : 6;
						}
					}
					else if (i.startsWith("pos")){
						const auto &p = i.mid(4, i.length() - 5).split(',');
						if (p.size() == 2){
							comment.mode = p[1].toInt() > vertical / 2 ? 4 : 5;
						}
					}
					else{
						comment.mode = 0;
						break;
					}
				}
				if (comment.mode != 0 && comment.color != 0){
					list.append(comment);
				}
			}
		}
		default:
			break;
		}
		return list;
	}

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

Load::Load(QObject *parent) : QObject(parent), d_ptr(new LoadPrivate(this))
{
	Q_D(Load);
	ins = this;
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
			int sta;
			if ((sta = video.indexOf("<div class=\"alist\">")) != -1 && sharp == -1){
				int len = video.indexOf("</select>", sta) - sta + 1;
				len = len < 0 ? 0 : len;
				QString select = video.mid(sta, len);
				QRegExp regex("value\\='[^']+");
				int cur = 0;
				api = "http://www." + Utils::customUrl(Utils::Bilibili);
				while ((cur = regex.indexIn(select, cur)) != -1){
					sta = select.indexOf('>', cur) + 1;
					cur += regex.matchedLength();
					QStandardItem *item = new QStandardItem;
					item->setData(QUrl(api + regex.cap().mid(7)), UrlRole);
					item->setData((task.code + "#%1").arg(d->model->rowCount() + 1), StrRole);
					item->setData(Page, NxtRole);
					item->setData(Utils::decodeXml(select.mid(sta, select.indexOf('<', sta) - sta)), Qt::EditRole);
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
			QString head = decodeAsText(data.left(512));
			if (head.startsWith("[Script Info]")){
				load.danmaku = parseComment(data, Utils::ASS);
			}
			else if (!head.startsWith("<?xml")){
				load.danmaku = parseComment(data, Utils::AcFun);
			}
			else if (head.indexOf("<packet>") != -1){
				load.danmaku = parseComment(data, Utils::Niconico);
			}
			else if (head.indexOf("<i>") != -1){
				load.danmaku = parseComment(data, Utils::Bilibili);
				QString i = QRegularExpression("(?<=<chatid>)\\d+(?=</chatid>)").match(head).captured();
				if (!i.isEmpty()){
					load.source = "http://comment.%1/%2.xml";
					load.source = load.source.arg(Utils::customUrl(Utils::Bilibili)).arg(i);
				}
			}
			else if (head.indexOf("<c>") != -1){
				load.danmaku = parseComment(data, Utils::AcfunLocalizer);
			}
			if (load.delay != 0){
				for (Comment &c : load.danmaku){
					c.time += load.delay;
				}
			}
			Danmaku::instance()->appendToPool(&load);
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
		if (QFileInfo(code).exists()){
			return true;
		}
		code.clear();
		return false;
	};

	auto fullBiProcess = [this](QNetworkReply *reply){
		Q_D(Load);
		Task &task = d->queue.head();
		static QHash<const Task *, QSet<Comment>> progress;
		switch (task.state){
		case None:
		{
			QString api("http://comment.%1/rolldate,%2");
			api = api.arg(Utils::customUrl(Utils::Bilibili));
			task.code = QUrlQuery(task.code.mid(5)).queryItemValue("source");
			forward(QNetworkRequest(api.arg(QFileInfo(task.code).baseName())), Code);
			break;
		}
		case Code:
		{
			QMap<QDate, int> count;
			for (QJsonValue iter : QJsonDocument::fromJson(reply->readAll()).array()){
				QJsonObject obj = iter.toObject();
				QJsonValue time = obj["timestamp"], size = obj["new"];
				count[QDateTime::fromTime_t(time.toVariant().toUInt()).date()] += size.toVariant().toInt();
			}
			if (count.isEmpty()){
				emit stateChanged(203);
				dequeue();
				return;
			}
			QString url("http://comment.%1/dmroll,%3,%2");
			url = url.arg(Utils::customUrl(Utils::Bilibili)).arg(QFileInfo(task.code).baseName());
			auto getHistory = [=](QDate date){
				return d->manager.get(QNetworkRequest(url.arg(QDateTime(date).toTime_t())));
			};
			emit stateChanged(task.state = File);
			QNetworkReply *header = getHistory(count.firstKey());
			connect(header, &QNetworkReply::finished, [=, &task](){
				QByteArray data = header->readAll();
				auto &record = progress[&task];
				for (const Comment &c : parseComment(data, Utils::Bilibili)){
					record.insert(c);
				}
				double total = 0;
				if (count.size() >= 2){
					int max = QRegularExpression("(?<=\\<max_count\\>).+(?=\\</max_count\\>)").match(data).captured().toInt();
					int now = 0;
					for (auto iter = count.begin() + 1;; ++iter){
						now += iter.value();
						if (iter + 1 == count.end()){
							d->remain += getHistory(iter.key());
							break;
						}
						else if (now + (iter + 1).value() > max){
							d->remain += getHistory(iter.key());
							now = 0;
						}
					}
					total = d->remain.size() + 2;
					emit progressChanged(2 / total);
				}
				else{
					progress.remove(&task);
					emit stateChanged(task.state = None);
					dequeue();
				}
				for (QNetworkReply *it : d->remain){
					connect(it, &QNetworkReply::finished, [=, &record, &task](){
						QByteArray data = it->readAll();
						for (const Comment &c : parseComment(data, Utils::Bilibili)){
							record.insert(c);
						}
						switch (it->error()){
						case QNetworkReply::NoError:
							emit progressChanged((total - d->remain.size()) / total);
						case QNetworkReply::OperationCanceledError:
							if (d->remain.isEmpty()){
								Record load;
								load.full = true;
								load.danmaku = record.toList();
								load.source = task.code;
								Danmaku::instance()->appendToPool(&load);
								progress.remove(&task);
								emit stateChanged(task.state = None);
								dequeue();
							}
						default:
							break;
						}
					});
				}
			});
			break;
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
			QString cid = QFileInfo(task.code).baseName(), url;
			QString dat = query.queryItemValue("date");
			if (dat != "0"){
				url = QString("http://comment.%1/dmroll,%2,%3");
				url = url.arg(Utils::customUrl(Utils::Bilibili));
				url = url.arg(dat).arg(cid);
			}
			else{
				url = QString("http://comment.%1/%2.xml").arg(Utils::customUrl(Utils::Bilibili));
				url = url.arg(cid);
			}
			forward(QNetworkRequest(url), File);
			break;
		}
		case File:
		{
			Record load;
			load.danmaku = parseComment(reply->readAll(), Utils::Bilibili);
			load.source = task.code;
			for (Record &iter : Danmaku::instance()->getPool()){
				if (iter.source == load.source){
					iter.full = false;
					iter.danmaku.clear();
					break;
				}
			}
			Danmaku::instance()->appendToPool(&load);
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
	APlayer *aplayer = APlayer::instance();
	task.delay = aplayer->getState() != APlayer::Stop&&Config::getValue("/Playing/Delay", false) ? aplayer->getTime() : 0;
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
	if (enqueue(task) && Config::getValue("/Playing/Clear", false)){
		Danmaku::instance()->clearPool();
	}
}

void Load::loadDanmaku(const QModelIndex &index)
{
	if (index.isValid()){
		QVariant u = index.data(UrlRole), s = index.data(StrRole);
		if (u.isValid() && s.isValid()){
			Task task;
			task.code = s.toString();
			task.processer = getProc(task.code);
			task.request = QNetworkRequest(u.toUrl());
			task.state = index.data(NxtRole).toInt();
			APlayer *aplayer = APlayer::instance();
			task.delay = aplayer->getState() != APlayer::Stop&&Config::getValue("/Playing/Delay", false) ? aplayer->getTime() : 0;
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

void Load::dumpDanmaku(const QList<Comment> *data, bool full)
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
	Danmaku::instance()->appendToPool(&load);
}

void Load::dumpDanmaku(const QByteArray &data, int site, bool full)
{
	auto list = parseComment(data, (Utils::Site)site);
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
