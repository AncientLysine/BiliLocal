#include "Common.h"
#include "Parse.h"
#include "../Local.h"
#include <QtConcurrent>
#include <cstdlib>

namespace
{
	QTextCodec *codeForData(const QByteArray &data)
	{
		QTextCodec *code = QTextCodec::codecForUtfText(data, nullptr);
		if (code) {
			return code;
		}
		QByteArray name;
		QByteArray head = data.left(512).toLower();
		if (head.startsWith("<?xml")) {
			int pos = head.indexOf("encoding=");
			if (pos >= 0) {
				pos += 9;
				if (pos < head.size()) {
					auto c = head.at(pos);
					if ('\"' == c || '\'' == c) {
						++pos;
						name = head.mid(pos, head.indexOf(c, pos) - pos);
					}
				}
			}
		}
		else {
			int pos = head.indexOf("charset=", head.indexOf("meta "));
			if (pos >= 0) {
				pos += 8;
				int end = pos;
				while (++end < head.size()) {
					auto c = head.at(end);
					if (c == '\"' || c == '\'' || c == '>') {
						name = head.mid(pos, end - pos);
						break;
					}
				}
			}
		}
		code = QTextCodec::codecForName(name);
		if (code) {
			return code;
		}
		return QTextCodec::codecForLocale();
	}

	QString decodeBytes(const QByteArray &data)
	{
		return codeForData(data)->toUnicode(data);
	}

	using namespace Parse;

	typedef ResultDelegate::Finish Finish;
	typedef ResultDelegate::Record Record;
	typedef ResultDelegate::Result Result;

	class FutureRecord : public Record
	{
	public:
		explicit FutureRecord(const QFuture<Result> &data)
			: data(data)
		{
		}

		virtual void onFinish(Finish cb) override
		{
			typedef QFutureWatcher<Result> Watcher;
			auto watcher = new Watcher(qApp);
			QObject::connect(watcher, &Watcher::finished, [=]() {
				Result r = watcher->future();
				delete watcher;
				cb(std::move(r));
			});
			watcher->setFuture(data);
		}

		virtual Result get() override
		{
			return data.result();
		}

	private:
		QFuture<Result> data;
	};

	class VectorRecord : public Record
	{
	public:
		Result data;

		explicit VectorRecord(const Result &data)
			: data(data)
		{
		}

		virtual void onFinish(Finish cb) override
		{
			cb(Result(data));
		}

		virtual Result get() override
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
		auto future = QtConcurrent::run([data]() {
			typedef QPair<const char *, const char *> RawChar;
			QVector<RawChar> raws;
			const char *dat = data.constData();
			QByteArrayMatcher match("<d p=");
			int sta = match.indexIn(data);
			if (sta == -1) {
				return QVector<Comment>();
			}
			for (;;) {
				auto sub = dat + sta;
				sta = match.indexIn(data, sta + 5);
				if (sta == -1) {
					raws.append(qMakePair(sub, dat + data.size()));
					break;
				}
				else {
					raws.append(qMakePair(sub, dat + sta));
				}
			}
			auto codec = codeForData(data);
			auto map = [codec](const RawChar &raw) {
				Comment comment;
				//strtof/strtoi need char * ?!
				char *arg = const_cast<char *>(raw.first) + 6;
				int time = std::strtod(arg, &arg) * 1000 + 0.5;
				if (*(arg++) != ',') return comment;
				int mode = std::strtol(arg, &arg, 10);
				if (*(arg++) != ',') return comment;
				int font = std::strtol(arg, &arg, 10);
				if (*(arg++) != ',') return comment;
				int colo = std::strtol(arg, &arg, 10);
				if (*(arg++) != ',') return comment;
				int date = std::strtol(arg, &arg, 10);
				comment.mode = mode;
				comment.font = font;
				comment.color = colo;
				comment.time = time;
				comment.date = date;
				auto num = 4;
				auto end = *(raw.first + 5);
				auto lst = arg;
				for (; arg < raw.second && *arg != end; ++arg) {
					if (',' == *arg) {
						if (7 == (++num)) {
							comment.sender = QLatin1String(lst, arg - lst);
						}
						lst = arg + 1;
					}
				}
				for (; arg < raw.second && *arg != '>'; ++arg);
				lst = arg + 1;
				for (; arg < raw.second && *arg != '<'; ++arg);
				comment.string = Utils::decodeXml(codec->toUnicode(lst, arg - lst), true);
				return comment;
			};
			auto reduce = [](QVector<Comment> &list, const Comment &comment) {
				if (comment.isEmpty() == false) {
					list.append(comment);
				}
			};
			qThreadPool->reserveThread();
			const auto &result = QtConcurrent::blockingMappedReduced<
				QVector<Comment>,
				QVector<RawChar>,
				std::function<Comment(const RawChar &)>,
				std::function<void(QVector<Comment> &, const Comment &)>>
				(raws, map, reduce, QtConcurrent::OrderedReduce | QtConcurrent::SequentialReduce);
			qThreadPool->releaseThread();
			return result;
		});
		return ResultDelegate(new FutureRecord(future));

	}
	case Utils::AcFun:
	{
		auto future = QtConcurrent::run([data]() {
			QQueue <QJsonArray> wait;
			wait.append(QJsonDocument::fromJson(data).array());
			QVector<QJsonValue> raws;
			while (!wait.isEmpty()) {
				const QJsonArray &head = wait.takeFirst();
				raws.reserve(raws.size() + raws.size());
				for (const QJsonValue &item : head) {
					if (item.isArray()) {
						wait.append(item.toArray());
					}
					else {
						raws.append(item);
					}
				}
			}
			auto map = [](const QJsonValue &item) {
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
			};
			auto reduce = [](QVector<Comment> &list, const Comment &comment) {
				if (!comment.isEmpty()) {
					list.append(comment);
				}
			};
			qThreadPool->reserveThread();
			const auto &result = QtConcurrent::blockingMappedReduced<
				QVector<Comment>,
				QVector<QJsonValue>,
				std::function<Comment(const QJsonValue &)>,
				std::function<void(QVector<Comment> &, const Comment &)>>
				(raws, map, reduce, QtConcurrent::OrderedReduce | QtConcurrent::SequentialReduce);
			qThreadPool->releaseThread();
			return result;
		});
		return ResultDelegate(new FutureRecord(future));
	}
	case Utils::AcfunLocalizer:
	{
		QVector<QStringRef> items = decodeBytes(data).splitRef("<l i=\"");
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
		QVector<QStringRef> items = decodeBytes(data).splitRef("<chat ");
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
		QString xml = decodeBytes(data);
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
