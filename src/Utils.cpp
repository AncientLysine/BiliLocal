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
	if (-1 != url.indexOf("acplay")){
		return AcPlay;
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
	case AcPlay:
		name = "acplay";
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
	defs << "acfun.tv" << "bilibili.com" << "acplay.net" << "tucao.cc";
	urls = Config::getValue("/Network/Url", defs.join(';')).split(';', QString::SkipEmptyParts);
	for (QString iter : urls + defs){
		if (iter.toLower().indexOf(name) != -1){
			return iter;
		}
	}
	return QString();
}

QString Utils::decodeXml(QString string, bool fast)
{
	if (!fast){
		QTextDocument text;
		text.setHtml(string);
		return text.toPlainText();
	}
	QString fixed;
	fixed.reserve(string.length());
	int i = 0, l = string.length();
	for (i = 0; i < l; ++i){
		QChar c = string[i];
		if (c >= ' ' || c == '\n'){
			bool f = true;
			switch (c.unicode()){
			case '&':
				if (l - i >= 4){
					switch (string[i + 1].unicode()){
					case 'l':
						if (string[i + 2] == 't'&&string[i + 3] == ';'){
							fixed += '<';
							f = false;
							i += 3;
						}
						break;
					case 'g':
						if (string[i + 2] == 't'&&string[i + 3] == ';'){
							fixed += '>';
							f = false;
							i += 3;
						}
						break;
					case 'a':
						if (l - i >= 5 && string[i + 2] == 'm'&&string[i + 3] == 'p'&&string[i + 4] == ';'){
							fixed += '&';
							f = false;
							i += 4;
						}
						break;
					case 'q':
						if (l - i >= 6 && string[i + 2] == 'u'&&string[i + 3] == 'o'&&string[i + 4] == 't'&&string[i + 5] == ';'){
							fixed += '\"';
							f = false;
							i += 5;
						}
						break;
					}
				}
				break;
			case '/':
			case '\\':
				if (l - i >= 2){
					switch (string[i + 1].unicode()){
					case 'n':
						fixed += '\n';
						f = false;
						i += 1;
						break;
					case 't':
						fixed += '\t';
						f = false;
						i += 1;
						break;
					case '\"':
						fixed += '\"';
						f = false;
						i += 1;
						break;
					}
				}
				break;
			}
			if (f){
				fixed += c;
			}
		}
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
