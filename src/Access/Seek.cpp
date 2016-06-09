/*=======================================================================
*
*   Copyright (C) 2013-2015 Lysine.
*
*   Filename:    Seek.cpp
*   Time:        2015/06/30
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
#include "Seek.h"
#include "AccessPrivate.h"
#include "../Utils.h"

namespace
{
	QStandardItem *praseItem(QNetworkRequest requset)
	{
		auto cell = requset.attribute(QNetworkRequest::User).value<quintptr>();
		return reinterpret_cast<QStandardItem *>(cell);
	}
}

class SeekPrivate : public AccessPrivate<Seek ,Seek::Proc, Seek::Task>
{
public:
	explicit SeekPrivate(Seek *seek):
		AccessPrivate<Seek, Seek::Proc, Seek::Task>(seek)
	{
	}

	virtual bool onError(QNetworkReply *reply) override
	{
		Q_Q(Seek);
		const Seek::Task &task = *q->getHead();
		switch(task.state){
		case Seek::More:
		case Seek::Data:
			return true;
		default:
			return AccessPrivate<Seek, Seek::Proc, Seek::Task>::onError(reply);
		}
	}

	void onThumbArrived(QNetworkReply *reply)
	{
		Q_Q(Seek);
		auto task = q->getHead();
		QPixmap icon;
		if (icon.loadFromData(reply->readAll())){
			icon = icon.scaled(task->cover, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
			praseItem(reply->request())->setIcon(icon);
		}
	}
};

namespace
{
	QStringList translate(const QList<const char *> &list)
	{
		QStringList t;
		for (const char *iter : list){
			t.append(Seek::tr(iter));
		}
		return t;
	}
	
	QHash<int, QString> getChannel(QString name)
	{
		static QHash<QString, QHash<int, QString>> m;
		if (!m.contains(name)){
			QFile file(":/Text/DATA");
			file.open(QIODevice::ReadOnly | QIODevice::Text);
			QJsonObject data = QJsonDocument::fromJson(file.readAll()).object()[name + "Channel"].toObject();
			for (auto iter = data.begin(); iter != data.end(); ++iter){
				m[name][iter.key().toInt()] = iter.value().toString();
			}
		}
		return m[name];
	}

	void disableEditing(const QList<QStandardItem *> &list)
	{
		for (QStandardItem *iter : list){
			iter->setEditable(false);
		}
	}
}

Seek::Seek(QObject *parent)
	: QObject(parent), d_ptr(new SeekPrivate(this))
{
	Q_D(Seek);
	setObjectName("Seek");

	QList<const char *> biOrder;
#define tr
	biOrder <<
		tr("totalrank") <<
		tr("click") <<
		tr("pubdate") <<
		tr("dm") <<
		tr("stow");
#undef tr

	auto biProcess = [=](QNetworkReply *reply){
		Q_D(Seek);
		Task &task = d->queue.head();
		static QHash<const Task *, QList<QNetworkRequest>> images;
		switch (task.state){
		case None:
		{
			if (task.text.isEmpty()){
				task.model->clear();
				dequeue();
				break;
			}
			QUrl url("http://search." + Utils::customUrl(Utils::Bilibili) + "/all");
			QUrlQuery query;
			query.addQueryItem("keyword", task.text);
			query.addQueryItem("page", QString::number(task.page.first));
			query.addQueryItem("order", biOrder[task.sort]);
			url.setQuery(query);
			task.request.setUrl(url);
			task.state = List;
			forward();
			break;
		}
		case List:
		{
			task.model->clear();
			task.model->setHorizontalHeaderLabels({ tr("Cover"), tr("Play"), tr("Danmaku"), tr("Title"), tr("Typename"), tr("Author") });
			task.model->horizontalHeaderItem(0)->setData(-1, Qt::UserRole);
			task.model->horizontalHeaderItem(1)->setData(45, Qt::UserRole);
			task.model->horizontalHeaderItem(2)->setData(45, Qt::UserRole);
			task.model->horizontalHeaderItem(4)->setData(75, Qt::UserRole);
			task.model->horizontalHeaderItem(5)->setData(75, Qt::UserRole);

			const QByteArray &data = reply->readAll();
			auto page = QTextCodec::codecForHtml(data, QTextCodec::codecForName("UTF-8"))->toUnicode(data);
			QStringList list = page.split("<li class=\"video", QString::SkipEmptyParts);

			if (list.isEmpty()) {
				emit stateChanged(task.state = None);
				dequeue();
				break;
			}

			QRegularExpression r;
			QRegularExpressionMatch m;
			r.setPatternOptions(QRegularExpression::DotMatchesEverythingOption);

			QStringList bang = list.takeFirst().split("<li class=\"synthetical");
			if (bang.size() > 0) {
				bang.removeFirst();
			}
			for (const QString &item : bang) {
				QList<QStandardItem *> line;
				for (int i = 0; i < 6; ++i) {
					line.append(new QStandardItem());
				}

				r.setPattern("(?<=src=\")[^\"']+");
				m = r.match(item);
				QNetworkRequest request(m.captured());
				request.setAttribute(QNetworkRequest::User, (quintptr)line[0]);
				d->remain += d->manager.get(request);

				r.setPattern("(?<=bangumi/i/)\\d+(?=/)");
				m = r.match(item, m.capturedEnd());
				line[0]->setData("bb" + m.captured(), Qt::UserRole);
				line[0]->setSizeHint(QSize(0, task.cover.height() + 3));

				r.setPattern("(?<=>).*?(?=</a>)");
				m = r.match(item, m.capturedEnd());
				line[3]->setText(Utils::decodeXml(m.captured().trimmed()));

				r.setPattern("(?<=>).*?(?=</div>)");
				m = r.match(item, item.indexOf("class=\"des", m.capturedEnd()));
				QString d = m.captured().trimmed();
				d.replace("<em class=\"keyword\">", "<font color=red>").replace("</em>", "</font>");
				d = "<p>" + d + "</p>";
				line[3]->setToolTip(d);

				disableEditing(line);
				task.model->appendRow(line);
			}

			if (list.size() > 0) {
				QString &last = list.last();
				last.truncate(last.indexOf("</li>") + 5);
			}
			for (const QString &item : list) {
				QList<QStandardItem *> line;
				for (int i = 0; i < 6; ++i) {
					line.append(new QStandardItem());
				}

				r.setPattern("av\\d+");
				m = r.match(item);
				line[0]->setData(m.captured(), Qt::UserRole);
				line[0]->setSizeHint(QSize(0, task.cover.height() + 3));

				r.setPattern("(?<=src=\")[^\"']+");
				m = r.match(item, m.capturedEnd());
				QNetworkRequest request(m.captured());
				request.setAttribute(QNetworkRequest::User, (quintptr)line[0]);
				d->remain += d->manager.get(request);

				r.setPattern("(?<=>).*?(?=</span>)");
				m = r.match(item, item.indexOf("class=\"type", m.capturedEnd()));
				line[4]->setText(Utils::decodeXml(m.captured()));

				r.setPattern("(?<=>).*?(?=</a>)");
				m = r.match(item, item.indexOf("class=\"title", m.capturedEnd()));
				line[3]->setText(Utils::decodeXml(m.captured().trimmed()));

				r.setPattern("(?<=>).*?(?=</div>)");
				m = r.match(item, item.indexOf("class=\"des", m.capturedEnd()));
				QString d = m.captured().trimmed();
				d.replace("<em class=\"keyword\">", "<font color=red>").replace("</em>", "</font>");
				d = "<p>" + d + "</p>";
				line[3]->setToolTip(d);

				r.setPattern("(?<=i>).*?(?=</span>)");
				m = r.match(item, item.indexOf("playtime", m.capturedEnd()));
				line[1]->setText(m.captured().simplified());

				m = r.match(item, item.indexOf("subtitle", m.capturedEnd()));
				line[2]->setText(m.captured().simplified());

				r.setPattern("(?<=>).*?(?=</a>)");
				m = r.match(item, item.indexOf("uper", m.capturedEnd()));
				line[5]->setText(Utils::decodeXml(m.captured()));

				disableEditing(line);
				task.model->appendRow(line);
			}

			auto num = QRegularExpression("(?<=\")\\d+").match(page, page.indexOf("data-num_pages"));
			task.page.second = num.captured().toInt();

			if (d->remain.isEmpty()){
				emit stateChanged(task.state = None);
				dequeue();
			}
			else{
				emit stateChanged(task.state = Data);
			}
			break;
		}
		case Data:
		{
			if (reply->error() == QNetworkReply::NoError){
				d->onThumbArrived(reply);
			}
			if (d->remain.isEmpty() && reply->error() != QNetworkReply::OperationCanceledError){
				emit stateChanged(task.state = None);
				dequeue();
			}
			break;
		}
		}
	};
	d->pool.append({ "Bilibili", translate(biOrder), 0, biProcess });

	QList<const char *> acOrder;
#define tr
	acOrder <<
		tr("rankLevel") <<
		tr("releaseDate") <<
		tr("views") <<
		tr("comments") <<
		tr("stows");
#undef tr

	auto acProcess = [=](QNetworkReply *reply){
		Q_D(Seek);
		Task &task = d->queue.head();
		switch (task.state){
		case None:
		{
			if (task.text.isEmpty()){
				task.model->clear();
				dequeue();
				break;
			}
			QUrl url("http://search." + Utils::customUrl(Utils::AcFun) + "/search");
			QUrlQuery query;
			query.addQueryItem("q", task.text);
			query.addQueryItem("sortType", "-1");
			query.addQueryItem("field", "title");
			query.addQueryItem("sortField", acOrder[task.sort]);
			query.addQueryItem("pageNo", QString::number(task.page.first));
			query.addQueryItem("pageSize", "20");
			url.setQuery(query);
			task.request.setUrl(url);
			task.state = List;
			forward();
			break;
		}
		case List:
		{
			task.model->clear();
			task.model->setHorizontalHeaderLabels({ tr("Cover"), tr("Play"), tr("Comment"), tr("Title"), tr("Typename"), tr("Author") });
			task.model->horizontalHeaderItem(0)->setData(-1, Qt::UserRole);
			task.model->horizontalHeaderItem(1)->setData(45, Qt::UserRole);
			task.model->horizontalHeaderItem(2)->setData(45, Qt::UserRole);
			task.model->horizontalHeaderItem(4)->setData(75, Qt::UserRole);
			task.model->horizontalHeaderItem(5)->setData(75, Qt::UserRole);

			QJsonObject page = QJsonDocument::fromJson(reply->readAll()).object()["data"].toObject()["page"].toObject();
			for (const QJsonValue &iter : page["list"].toArray()){
				QJsonObject item = iter.toObject();

				int channelId = item["channelId"].toDouble();
				if (channelId == 110 || channelId == 63 || (channelId > 72 && channelId < 77)) {
					continue;
				}

				QList<QStandardItem *> line;
				line << new QStandardItem();
				line << new QStandardItem(QString::number((int)item["views"].toDouble()));
				line << new QStandardItem(QString::number((int)item["comments"].toDouble()));
				line << new QStandardItem(Utils::decodeXml(item["title"].toString()));
				line << new QStandardItem(getChannel("AcFun")[channelId]);
				line << new QStandardItem(Utils::decodeXml(item["username"].toString()));
				line[0]->setData(item["contentId"].toString(), Qt::UserRole);
				line[0]->setSizeHint(QSize(0, task.cover.height() + 3));
				line[3]->setToolTip("<p>" + item["description"].toString() + "</p>");

				QNetworkRequest request(QUrl(item["titleImg"].toString()));
				request.setAttribute(QNetworkRequest::User, (quintptr)line[0]);
				d->remain += d->manager.get(request);

				disableEditing(line);
				task.model->appendRow(line);
			}

			task.page.second = qCeil(page["totalCount"].toDouble() / page["pageSize"].toDouble());

			if (d->remain.isEmpty()){
				emit stateChanged(task.state = None);
				dequeue();
			}
			else{
				emit stateChanged(task.state = Data);
			}
			break;
		}
		case Data:
		{
			if (reply->error() == QNetworkReply::NoError){
				d->onThumbArrived(reply);
			}
			if (d->remain.isEmpty() && reply->error() != QNetworkReply::OperationCanceledError){
				emit stateChanged(task.state = None);
				dequeue();
			}
			break;
		}
		}
	};
	d->pool.append({ "AcFun", translate(acOrder), 0, acProcess });

	connect(this, &Seek::stateChanged, [this](int code){
		switch (code){
		case None:
		case List:
		case More:
		case Data:
			break;
		default:
		{
			Q_D(Seek);
			if (!d->tryNext()){
				emit errorOccured(code);
			}
			break;
		}
		}
	});
}

Seek::~Seek()
{
	delete d_ptr;
}

void Seek::addProc(const Seek::Proc *proc)
{
	Q_D(Seek);
	d->addProc(proc);
}

const Seek::Proc *Seek::getProc(QString name)
{
	Q_D(Seek);
	return d->getProc(name);
}

QStringList Seek::modules()
{
	Q_D(Seek);
	QStringList list;
	for (const Proc &iter : d->pool){
		if (list.contains(iter.name)){
			continue;
		}
		list << iter.name;
	}
	return list;
}

void Seek::dequeue()
{
	Q_D(Seek);
	d->dequeue();
}

bool Seek::enqueue(const Seek::Task &task)
{
	Q_D(Seek);
	if (d->queue.size()){
		d->dequeue();
	}
	return d->enqueue(task);
}

Seek::Task *Seek::getHead()
{
	Q_D(Seek);
	return d->getHead();
}

void Seek::forward()
{
	Q_D(Seek);
	d->forward();
}
