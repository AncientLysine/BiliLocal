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

QString Utils::localPath(Path path)
{
#ifdef Q_OS_WIN32
	QString base = qApp->applicationDirPath() + '/';
	switch (path){
	case Cache:
		return base + "cache/";
	case Config:
		return base;
	case Locale:
		return base + "locale/";
	case Plugin:
		return base + "plugins/";
	case Script:
		return base + "scripts/";
	default:
		return base;
	}
#else
	switch (path){
	case Cache:
		return QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + '/';
	case Config:
		return QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + '/';
	case Locale:
		return QStandardPaths::writableLocation(QStandardPaths::DataLocation) + "/Locale/";
	case Plugin:
		return QStandardPaths::writableLocation(QStandardPaths::DataLocation) + "/Plugin/";
	case Script:
		return QStandardPaths::writableLocation(QStandardPaths::DataLocation) + "/Script/";
	default:
		return QStandardPaths::writableLocation(QStandardPaths::DataLocation) + '/';
	}
#endif
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
	defs << "acfun.tv" << "bilibili.com" << "tucao.tv";
	urls = Config::getValue("/Network/Url", defs.join(';')).split(';', QString::SkipEmptyParts);
	for (QString iter : urls + defs){
		if (iter.toLower().indexOf(name) != -1){
			return iter;
		}
	}
	return QString();
}

namespace
{
	template<char16_t... str>
	struct SString;

	template<char16_t lst>
	struct SString<lst>
	{
		inline static bool equal(char16_t const *src)
		{
			return lst == *src;
		}

		inline static bool equal(char16_t const *src, char16_t const *end)
		{
			return src < end && SString<lst>::equal(src);
		}
	};

	template<char16_t top, char16_t... rst>
	struct SString<top, rst...>
	{
		inline static bool equal(char16_t const *src)
		{
			return top == *src && SString<rst...>::equal(src + 1);
		}

		inline static bool equal(char16_t const *src, char16_t const *end)
		{
			return src + (int)sizeof...(rst) - 1 < end && SString<top, rst...>::equal(src);
		}
	};

	void htmlEscape(char16_t * &dst, char16_t const * &src, char16_t const *end)
	{
		Q_ASSERT(src[0] == u'&');

		if (src + 1 >= end) {
			return;
		}

		switch (src[1]) {
		case  u'a':
			if (SString<'m', 'p', ';'>::equal(src + 2, end)) {
				dst[0] = u'&';
				dst += 1;
				src += 5;
			}
			break;
		case  u'g':
			if (SString<'t', ';'>::equal(src + 2, end)) {
				dst[0] = u'>';
				dst += 1;
				src += 4;
			}
			break;
		case  u'l':
			if (SString<'t', ';'>::equal(src + 2, end)) {
				dst[0] = u'<';
				dst += 1;
				src += 4;
			}
			break;
		case  u'n':
			if (SString<'b', 's', 'p', ';'>::equal(src + 2, end)) {
				dst[0] = u' ';
				dst += 1;
				src += 6;
			}
			break;
		case  u'q':
			if (SString<'u', 'o', 't', ';'>::equal(src + 2, end)) {
				dst[0] = u'"';
				dst += 1;
				src += 6;
			}
			break;
		default:
			break;
		}
	}

	void charEscape(char16_t * &dst, char16_t const * &src, char16_t const *end)
	{
		Q_ASSERT(src[0] == u'/' || src[0] == u'\\');

		if (src + 1 >= end) {
			return;
		}

		switch (src[1]) {
		case u'r':
			dst[0] = u'\r';
			dst += 1;
			src += 2;
			break;
		case u'n':
			dst[0] = u'\n';
			dst += 1;
			src += 2;
			break;
		case u't':
			dst[0] = u'\t';
			dst += 1;
			src += 2;
			break;
		case u'\"':
			dst[0] = u'\"';
			dst += 1;
			src += 2;
			break;
		default:
			break;
		}
	}

	void lineEscape(char16_t * &dst, char16_t const * &src, char16_t const *end)
	{
		Q_ASSERT(src[0] == u'\r');

		if (src + 1 >= end) {
			return;
		}

		switch (src[1]) {
		case u'\n':
			dst[0] = u'\n';
			dst += 1;
			src += 2;
			break;
		default:
			break;
		}
	}
}

QString Utils::decodeTxt(QString &&txt)
{
	char16_t *dst = (char16_t *)txt.data();
	const char16_t *end = dst + txt.length();
	while (dst < end) {
		if (u'/' == *dst || u'\\' == *dst) {
			break;
		}
		++dst;
	}
	const char16_t *src = dst;
	while (src < end) {
		const char16_t *cur = src;
		switch (*src) {
		case u'/':
		case u'\\':
			charEscape(dst, src, end);
			break;
		default:
			break;
		}
		if (cur == src) {
			dst[0] = src[0];
			++dst;
			++src;
		}
	}
	txt.resize(dst - (char16_t *)txt.data());
	return std::move(txt);
}

QString Utils::decodeXml(QString &&xml, bool fast)
{
	if (fast == false) {
		QTextDocument text;
		text.setHtml(xml);
		return text.toPlainText();
	}

	char16_t *dst = (char16_t *)xml.data();
	const char16_t *end = dst + xml.length();
	while (dst < end) {
		if (u'&' == *dst || u'\r' == *dst) {
			break;
		}
		++dst;
	}
	const char16_t *src = dst;
	while (src < end) {
		const char16_t *cur = src;
		switch (*src) {
		case u'&':
			htmlEscape(dst, src, end);
			break;
		case u'\r':
			lineEscape(dst, src, end);
			break;
		default:
			break;
		}
		if (cur == src) {
			dst[0] = src[0];
			++dst;
			++src;
		}
	}
	xml.resize(dst - (char16_t *)xml.data());
	return std::move(xml);
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
