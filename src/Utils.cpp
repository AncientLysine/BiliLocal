/*=======================================================================
*
*   Copyright (C) 2013-2015 Lysine.
*
*   Filename:    Utils.cpp
*   Time:        2013/05/10
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
#include "Utils.h"
#include "Config.h"
#include <algorithm>
#include <exception>

Utils::Site Utils::parseSite(QString url)
{
	url = url.toLower();
	if (-1 != url.indexOf("letv")){
		return Letv;
	}
	if (-1 != url.indexOf("tudou")){
		return Tudou;
	}
	if (-1 != url.indexOf("bilibili")){
		return Bilibili;
	}
	if (-1 != url.indexOf("acfun")){
		return AcFun;
	}
	if (-1 != url.indexOf("tucao")){
		return TuCao;
	}
	return Unknown;
}

void Utils::setCenter(QWidget *widget)
{
	QRect rect = widget->geometry();
	QWidget *parent = widget->parentWidget();
	if (!parent){
		rect.moveCenter(QApplication::desktop()->screenGeometry(widget).center());
	}
	else{
		if (widget->isWindow()){
			QPoint center = parent->geometry().center();
			if ((parent->windowFlags()&Qt::CustomizeWindowHint)){
				center.ry() += widget->style()->pixelMetric(QStyle::PM_TitleBarHeight) / 2;
			}
			rect.moveCenter(center);
		}
		else{
			rect.moveCenter(parent->rect().center());
		}
	}
	widget->setGeometry(rect);
}

void Utils::setGround(QWidget *widget, QColor color)
{
	widget->setAutoFillBackground(true);
	QPalette palette = widget->palette();
	palette.setColor(QPalette::Window, color);
	widget->setPalette(palette);
}

namespace{
	template<class T>
	class SStack
	{
	public:
		inline T &top()
		{
			if (isEmpty()){
				throw std::runtime_error("token mismatch");
			}
			return stk.top();
		}

		inline T pop()
		{
			if (isEmpty()){
				throw std::runtime_error("token mismatch");
			}
			return stk.pop();
		}

		inline void push(const T &i)
		{
			stk.push(i);
		}

		inline bool isEmpty()
		{
			return stk.isEmpty();
		}

	private:
		QStack<T> stk;
	};
}

double Utils::evaluate(QString exp)
{
	auto priority = [](QChar o){
		switch (o.unicode())
		{
		case '(':
			return 1;
		case '+':
		case '-':
			return 2;
		case '*':
		case '/':
			return 3;
		case '+' + 128:
		case '-' + 128:
			return 4;
		case ':':
		case ':' + 128:
			return 5;
		default:
			return 0;
		}
	};
	exp.remove(' ');
	QString pst;
	SStack<QChar> opt;
	int i = 0;
	opt.push('#');
	while (i < exp.length()){
		if (exp[i].isDigit() || exp[i] == '.'){
			pst.append(exp[i]);
		}
		else{
			auto tra = [&](){
				pst.append(' ');
				while (priority(exp[i]) <= priority(opt.top())){
					pst.append(opt.pop());
				}
				opt.push(exp[i]);
			};
			int colon = 0;
			switch (exp[i].unicode()){
			case '(':
				opt.push(exp[i]);
				break;
			case ')':
				while (opt.top() != '('){
					pst.append(opt.pop());
				}
				opt.pop();
				break;
			case '+':
			case '-':
			{
				if ((i == 0 || (!exp[i - 1].isDigit() && exp[i - 1] != ')')) && (i + 1) < exp.length() && (exp[i + 1].isDigit() || exp[i + 1] == '(')){
					exp[i].unicode() += 128;
				}
				tra();
				break;
			}
			case ':':
				switch (colon++){
				case 2:
					exp[i].unicode() += 128;
				case 1:
				case 0:
					break;
				default:
					throw std::runtime_error("colon overflow");
				}
				tra();
				break;
			case '*':
			case '/':
				tra();
				break;
			default:
				throw std::runtime_error("token unrecognized");
			}
		}
		++i;
	}
	while (!opt.isEmpty()){
		pst.append(opt.pop());
	}
	SStack<double> num;
	i = 0;
	while (pst[i] != '#'){
		if (pst[i].isDigit() || pst[i] == '.'){
			double n = 0;
			while (pst[i].isDigit()){
				n = n * 10 + pst[i++].toLatin1() - '0';
			}
			if (pst[i] == '.'){
				++i;
				double d = 1;
				while (pst[i].isDigit()){
					n += (d /= 10)*(pst[i++].toLatin1() - '0');
				}
			}
			num.push(n);
		}
		else{
			switch (pst[i].unicode()){
			case '+' + 128:
				num.push(+num.pop());
				break;
			case '-' + 128:
				num.push(-num.pop());
				break;
			case '+':
			{
				double r = num.pop(), l = num.pop();
				num.push(l + r);
				break;
			}
			case '-':
			{
				double r = num.pop(), l = num.pop();
				num.push(l - r);
				break;
			}
			case '*':
			{
				double r = num.pop(), l = num.pop();
				num.push(l*r);
				break;
			}
			case '/':
			{
				double r = num.pop(), l = num.pop();
				num.push(l / r);
				break;
			}
			case ':':
			{
				double r = num.pop(), l = num.pop();
				num.push(l * 60 + r);
				break;
			}
			case ':' + 128:
			{
				double r = num.pop(), l = num.pop();
				num.push(l * 24 + r);
				break;
			}
			}
			i++;
		}
	}
	return num.top();
}

QString Utils::defaultFont(bool monospace)
{
	if (monospace){
#ifdef Q_OS_LINUX
		return QStringLiteral("文泉驿等宽正黑");
#endif
#ifdef Q_OS_WIN32
		return QStringLiteral("黑体");
#endif
#ifdef Q_OS_MAC
		return QStringLiteral("华文黑体");
#endif
	}
	else{
#ifdef Q_OS_LINUX
		return QStringLiteral("文泉驿正黑");
#endif
#ifdef Q_OS_WIN32
		return QStringLiteral("微软雅黑");
#endif
#ifdef Q_OS_MAC
		return QStringLiteral("华文黑体");
#endif
	}
}

QString Utils::customUrl(Site site)
{
	QString name;
	switch (site){
	case AcFun:
		name = "acfun";
		break;
	case Bilibili:
		name = "bili";
		break;
	case Tudou:
		name = "tudou";
		break;
	case Niconico:
		name = "nico";
		break;
	case TuCao:
		name = "tucao";
		break;
	default:
		return QString();
	}
	QStringList urls, defs;
	defs << "acfun.tv" << "bilibili.com" << "tucao.cc";
	urls = Config::getValue("/Network/Url", defs.join(';')).split(';', QString::SkipEmptyParts);
	for (QString iter : urls + defs){
		if (iter.toLower().indexOf(name) != -1){
			return iter;
		}
	}
	return QString();
}

QString Utils::decodeTxt(const QByteArray &data)
{
	QTextCodec *codec = QTextCodec::codecForUtfText(data, nullptr);
	if (!codec) {
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
		codec = QTextCodec::codecForName(name);
	}
	if (!codec) {
		codec = QTextCodec::codecForLocale();
	}
	return codec->toUnicode(data);
}

QString Utils::decodeXml(QString string, bool fast)
{
	if (fast) {
		return decodeXml(QStringRef(&string), true);
	}

	QTextDocument text;
	text.setHtml(string);
	return text.toPlainText();
}

namespace
{
#ifdef _MSC_VER
#define FORCEINLINE __forceinline
#elif defined __GNUC__
#define FORCEINLINE __inline__ __attribute__((always_inline))
#else
#define FORCEINLINE inline
#endif

	template<char16_t... list>
	struct StaticString;

	template<char16_t tail>
	struct StaticString<tail>
	{
		FORCEINLINE static bool equalTo(const char16_t *string, int offset)
		{
			return string[offset] == tail;
		}
	};

	template<char16_t head, char16_t... rest>
	struct StaticString<head, rest...>
	{
		FORCEINLINE static bool equalTo(const char16_t *string, int offset)
		{
			return string[offset] == head && StaticString<rest...>::equalTo(string, offset + 1);
		}
	};

	template<char16_t... list>
	FORCEINLINE bool equal(const char16_t *string, int length, int offset)
	{
		return offset + (int)sizeof...(list) < length && StaticString<list...>::equalTo(string, offset);
	}

	int decodeHtmlEscape(const char16_t *data, int length, int i, char16_t &c)
	{
		if (i + 1 >= length) {
			return 0;
		}

		switch (data[i]) {
		case 'n':
			// &nbsp;
			if (equal<'b', 's', 'p', ';'>(data, length, i + 1)) {
				c = ' ';
				return 5;
			}
			break;
		case 'l':
			// &lt;
			if (equal<'t', ';'>(data, length, i + 1)) {
				c = '<';
				return 3;
			}
			break;
		case 'g':
			// &gt;
			if (equal<'t', ';'>(data, length, i + 1)) {
				c = '>';
				return 3;
			}
			break;
		case 'a':
			// &amp;
			if (equal<'m', 'p', ';'>(data, length, i + 1)) {
				c = '&';
				return 4;
			}
			break;
		case 'q':
			// &quot;
			if (equal<'u', 'o', 't', ';'>(data, length, i + 1)) {
				c = '\"';
				return 5;
			}
			break;
		case 'c':
			// &copy;
			if (equal<'o', 'p', 'y', ';'>(data, length, i + 1)) {
				c = u'©';
				return 5;
			}
			break;
		case 'r':
			// &reg;
			if (equal<'e', 'g', ';'>(data, length, i + 1)) {
				c = u'®';
				return 4;
			}
			break;
		case 't':
			// &times;
			if (equal<'i', 'm', 'e', 's', ';'>(data, length, i + 1)) {
				c = u'×';
				return 6;
			}
			break;
		case 'd':
			// &divide;
			if (equal<'i', 'v', 'i', 'd', 'e', ';'>(data, length, i + 1)) {
				c = u'÷';
				return 7;
			}
			break;
		}

		return 0;
	}

	int decodeCharEscape(const char16_t *data, int length, int i, char16_t &c)
	{
		if (i + 1 >= length) {
			return 0;
		}

		switch (data[i]) {
		case 'n':
			c = '\n';
			return 1;
		case 't':
			c = '\t';
			return 1;
		case '\"':
			c = '\"';
			return 1;
		}

		return 0;
	}
}

QString Utils::decodeXml(QStringRef ref, bool fast)
{
	if (!fast) {
		return decodeXml(ref.toString(), false);
	}

	int length = ref.length();
	const char16_t *data = (const char16_t *)ref.data();

	QString fixed;
	fixed.reserve(length);

	int passed = 0;
	const char16_t *head = data;

	for (int i = 0; i < length; ++i) {
		char16_t c = data[i];
		if (c < ' ' && c != '\n') {
			continue;
		}

		bool plain = true;
		switch (c) {
		case '&':
		{
			int p = decodeHtmlEscape(data, length, i + 1, c);
			plain = p == 0;
			i += p;
			break;
		}
		case '/':
		case '\\':
		{
			int p = decodeCharEscape(data, length, i + 1, c);
			plain = p == 0;
			i += p;
			break;
		}
		}
		if (plain) {
			++passed;
		}
		else {
			if (passed > 0) {
				fixed.append((QChar *)head, passed);
				passed = 0;
				head = data + i + 1;
			}
			fixed.append(QChar(c));
		}
	}
	if (passed > 0) {
		fixed.append((QChar *)head, passed);
	}
	return fixed;
}

QStringList Utils::getSuffix(int type, QString format)
{
	QStringList set;
	if (type&Video){
		set << "3g2" << "3gp" << "3gp2" << "3gpp" << "amv" << "asf" << "avi" << "divx" << "drc" << "dv" <<
			"f4v" << "flv" << "gvi" << "gxf" << "hlv" << "iso" << "letv" <<
			"m1v" << "m2t" << "m2ts" << "m2v" << "m4v" << "mkv" << "mov" <<
			"mp2" << "mp2v" << "mp4" << "mp4v" << "mpe" << "mpeg" << "mpeg1" <<
			"mpeg2" << "mpeg4" << "mpg" << "mpv2" << "mts" << "mtv" << "mxf" << "mxg" << "nsv" << "nuv" <<
			"ogg" << "ogm" << "ogv" << "ogx" << "ps" <<
			"rec" << "rm" << "rmvb" << "tod" << "ts" << "tts" << "vob" << "vro" <<
			"webm" << "wm" << "wmv" << "wtv" << "xesc";
	}
	if (type&Audio){
		int size = set.size();
		set << "3ga" << "669" << "a52" << "aac" << "ac3" << "adt" << "adts" << "aif" << "aifc" << "aiff" <<
			"amr" << "aob" << "ape" << "awb" << "caf" << "dts" << "flac" << "it" << "kar" <<
			"m4a" << "m4p" << "m5p" << "mka" << "mlp" << "mod" << "mp1" << "mp2" << "mp3" << "mpa" << "mpc" << "mpga" <<
			"oga" << "ogg" << "oma" << "opus" << "qcp" << "ra" << "rmi" << "s3m" << "spx" << "thd" << "tta" <<
			"voc" << "vqf" << "w64" << "wav" << "wma" << "wv" << "xa" << "xm";
		std::inplace_merge(set.begin(), set.begin() + size, set.end());
	}
	if (type&Subtitle){
		int size = set.size();
		set << "aqt" << "ass" << "cdg" << "dks" << "idx" << "jss" << "mks" << "mpl2" << "pjs" << "psb" << "rt" <<
			"smi" << "smil" << "srt" << "ssa" << "stl" << "sub" << "txt" << "usf" << "utf";
		std::inplace_merge(set.begin(), set.begin() + size, set.end());
	}
	if (type&Danmaku){
		int size = set.size();
		set << "json" << "xml";
		std::inplace_merge(set.begin(), set.begin() + size, set.end());
	}
	if (!format.isEmpty()){
		for (QString &iter : set){
			iter = format.arg(iter);
		}
	}
	return set;
}
