#include "Common.h"
#include "Parse.h"
#include <QtConcurrent>
#include <functional>

namespace
{
	class FutureRecord : public Parse::ResultDelegate::Record
	{
	public:
		QFuture<QVector<Comment>> data;

		explicit FutureRecord(const QFuture<QVector<Comment>> &data) :
			data(data)
		{
		}

		virtual QVector<Comment> get() override
		{
			return data.result();
		}
	};

	class VectorRecord : public Parse::ResultDelegate::Record
	{
	public:
		QVector<Comment> data;

		explicit VectorRecord(const QVector<Comment> &data) :
			data(data)
		{
		}

		virtual QVector<Comment> get() override
		{
			return data;
		}
	};
}

Parse::ResultDelegate Parse::parseComment(const QByteArray &data, Utils::Site site)
{
	switch (site) {
	case Utils::Bilibili:
	case Utils::TuCao:
	{
		auto xml = QSharedPointer<QString>::create(Utils::decodeTxt(data));
		QVector<QStringRef> items = xml->splitRef("<d p=");
		if (items.isEmpty()) {
			return ResultDelegate();
		}
		items.removeFirst();
		auto future = QtConcurrent::mappedReduced<
			QVector<Comment>,
			QVector<QStringRef>,
			std::function<Comment(const QStringRef &)>,
			std::function<void(QVector<Comment> &, const Comment &)>
		>(
			items,
			[xml](const QStringRef &item) {
			const QVector<QStringRef> &args = item.mid(1, item.indexOf(item.at(0), 1) - 1).split(',');
			Comment comment;
			if (args.size() > 4) {
				comment.time = args[0].toDouble() * 1000 + 0.5;
				comment.date = args[4].toInt();
				comment.mode = args[1].toInt();
				comment.font = args[2].toInt();
				comment.color = args[3].toInt();
				comment.sender = args.size() <= 6 ? QString() : args[6].toString();
				const QStringRef &last = args[args.size() <= 6 ? 4 : 6];
				int sta = item.indexOf('>', last.position() + last.size() - item.position()) + 1;
				int len = item.indexOf('<', sta) - sta;
				comment.string = Utils::decodeXml(item.mid(sta, len), true);
			}
			return comment;
		},
			[](QVector<Comment> &list, const Comment &comment) {
			if (!comment.isEmpty()) {
				list.append(comment);
			}
		}
		);
		return ResultDelegate(new FutureRecord(future));
	}
	case Utils::AcFun:
	{
		QQueue <QJsonArray> queue;
		queue.append(QJsonDocument::fromJson(data).array());
		QVector<QJsonValue> items;
		while (!queue.isEmpty()) {
			const QJsonArray &head = queue.takeFirst();
			items.reserve(items.size() + items.size());
			for (const QJsonValue &item : head) {
				if (item.isArray()) {
					queue.append(item.toArray());
				}
				else {
					items.append(item);
				}
			}
		}
		auto future = QtConcurrent::mappedReduced<
			QVector<Comment>,
			QVector<QJsonValue>,
			std::function<Comment(const QJsonValue &)>,
			std::function<void(QVector<Comment> &, const Comment &)>
		>(
			items,
			[items](const QJsonValue &item) {
			QJsonObject o = item.toObject();
			const QString &c = o["c"].toString();
			const QString &m = o["m"].toString();
			const QVector<QStringRef> &args = c.splitRef(',');
			Comment comment;
			if (args.size() > 5) {
				comment.time = args[0].toDouble() * 1000 + 0.5;
				comment.date = args[5].toInt();
				comment.mode = args[2].toInt();
				comment.font = args[3].toInt();
				comment.color = args[1].toInt();
				comment.sender = args[4].toString();
				comment.string = m;
			}
			return comment;
		},
			[](QVector<Comment> &list, const Comment &comment) {
			if (!comment.isEmpty()) {
				list.append(comment);
			}
		}
		);
		return ResultDelegate(new FutureRecord(future));
	}
	case Utils::AcfunLocalizer:
	{
		QString xml = Utils::decodeTxt(data);
		QVector<QStringRef> items = xml.splitRef("<l i=\"");
		if (items.isEmpty()) {
			return ResultDelegate();
		}
		items.removeFirst();
		QVector<Comment> list;
		list.reserve(items.size());
		for (const QStringRef &item : items){
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
			comment.string = Utils::decodeXml(item.mid(sta, len), true);
			list.append(comment);
		}
		return ResultDelegate(new VectorRecord(list));
	}
	case Utils::Niconico:
	{
		QVector<QStringRef> items = Utils::decodeTxt(data).splitRef("<chat ");
		if (items.isEmpty()) {
			return ResultDelegate();
		}
		items.removeFirst();
		QVector<Comment> list;
		list.reserve(items.size());
		for (const QStringRef &item : items){
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
		return ResultDelegate(new VectorRecord(list));
	}
	case Utils::ASS:
	{
		QString xml = Utils::decodeTxt(data);
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
			return ResultDelegate();
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
			return ResultDelegate();
		}
		qint64 dat = QDateTime::currentDateTime().toTime_t();
		pos += len;
		ref = xml.midRef(pos).split("Dialogue:",QString::SkipEmptyParts);
		QVector<Comment> list;
		list.reserve(ref.size());
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
		return ResultDelegate(new VectorRecord(list));
	}
	default:
		return ResultDelegate();
	}
}
