/*=======================================================================
*
*   Copyright (C) 2013 Lysine.
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

QJsonObject Utils::config;

Utils::Site Utils::getSite(QString url)
{
	if(url.startsWith("http://www.bilibili")||url.startsWith("http://bilibili.kankanews")){
		return Bilibili;
	}
	else if(url.startsWith("http://www.acfun")||url.startsWith("http://api.acfun")){
		return AcFun;
	}
	else if(url.startsWith("http://comic.letv.com")){
		return Letv;
	}
	else if(url.startsWith("http://api.acplay.net")){
		return AcPlay;
	}
	else{
		return Unknown;
	}
}

void Utils::setCenter(QWidget *widget)
{
	QPoint center;
	QWidget *parent=widget->parentWidget();
	if(parent==NULL){
		center=QApplication::desktop()->screenGeometry(widget).center();
	}
	else if(widget->isWindow()){
		center=parent->geometry().center();
	}
	else{
		center=parent->rect().center();
	}
	QRect rect=widget->geometry();
	rect.moveCenter(center);
	widget->setGeometry(rect);
}

void Utils::setGround(QWidget *widget,QColor color)
{
	widget->setAutoFillBackground(true);
	QPalette palette=widget->palette();
	palette.setColor(QPalette::Window,color);
	widget->setPalette(palette);
}

void Utils::setSelection(QAbstractItemView *view)
{
	QObject::connect(view->selectionModel(),
					 &QItemSelectionModel::selectionChanged,
					 [view](QItemSelection selected){
		if(selected.isEmpty()){
			view->setCurrentIndex(QModelIndex());
		}
	});
}

namespace{
template<class T>
class SStack
{
public:
	inline T &top()
	{
		if(isEmpty()){
			QT_THROW("Empty");
		}
		return stk.top();
	}

	inline T pop()
	{
		if(isEmpty()){
			QT_THROW("Empty");
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
	auto priority=[](QChar o){
		switch(o.unicode())
		{
		case '(':
			return 1;
		case '+':
		case '-':
			return 2;
		case '*':
		case '/':
			return 3;
		case '+'+128:
		case '-'+128:
			return 4;
		default:
			return 0;
		}
	};

	QT_TRY{
		QString pst;
		SStack<QChar> opt;
		int i=0;
		opt.push('#');
		while(i<exp.length()){
			if(exp[i].isDigit()||exp[i]=='.'){
				pst.append(exp[i]);
			}
			else{
				switch(exp[i].unicode()){
				case '(':
					opt.push(exp[i]);
					break;
				case ')':
					while(opt.top()!='('){
						pst.append(opt.pop());
					}
					opt.pop();
					break;
				case '+':
				case '-':
					if((i==0||exp[i-1]=='(')&&(i+1)<exp.length()&&(exp[i+1].isDigit()||exp[i+1]=='(')){
						exp[i].unicode()+=128;
					}
				case '*':
				case '/':
					pst.append(' ');
					while(priority(exp[i])<=priority(opt.top())){
						pst.append(opt.pop());
					}
					opt.push(exp[i]);
					break;
				case ' ':
					break;
				default:
					QT_THROW("Invalid");
				}
			}
			++i;
		}
		while(!opt.isEmpty()){
			pst.append(opt.pop());
		}
		SStack<double> num;
		i=0;
		while(pst[i]!='#'){
			if(pst[i].isDigit()||pst[i]=='.'){
				double n=0;
				while(pst[i].isDigit()){
					n=n*10+pst[i++].toLatin1()-'0';
				}
				if(pst[i]=='.'){
					++i;
					double d=1;
					while(pst[i].isDigit()){
						n+=(d/=10)*(pst[i++].toLatin1()-'0');
					}
				}
				num.push(n);
			}
			else{
				switch(pst[i].unicode()){
				case '+'+128:
					num.push(+num.pop());
					break;
				case '-'+128:
					num.push(-num.pop());
					break;
				case '+':
				{
					double r=num.pop(),l=num.pop();
					num.push(l+r);
					break;
				}
				case '-':
				{
					double r=num.pop(),l=num.pop();
					num.push(l-r);
					break;
				}
				case '*':
				{
					double r=num.pop(),l=num.pop();
					num.push(l*r);
					break;
				}
				case '/':
				{
					double r=num.pop(),l=num.pop();
					num.push(l/r);
					break;
				}
				}
				i++;
			}
		}
		return num.top();
	}
	QT_CATCH(...){
		return 0;
	}
}

QString Utils::defaultPath()
{
	QStringList paths=QStandardPaths::standardLocations(QStandardPaths::MoviesLocation);
	return getConfig("/Playing/Path",paths.isEmpty()?QDir::homePath():paths.last());
}

QString Utils::defaultFont(bool monospace)
{
	if(monospace){
#ifdef Q_OS_LINUX
		return "文泉驿等宽正黑";
#endif
#ifdef Q_OS_WIN32
#ifdef Q_CC_MSVC
		return QString::fromLocal8Bit("黑体");
#else
		return "黑体";
#endif
#endif
#ifdef Q_OS_MAC
		return "";
#endif
	}
	else{
#ifdef Q_OS_LINUX
		return "文泉驿正黑";
#endif
#ifdef Q_OS_WIN32
#ifdef Q_CC_MSVC
		return QString::fromLocal8Bit("微软雅黑");
#else
		return "微软雅黑";
#endif
#endif
#ifdef Q_OS_MAC
		return "华文黑体";
#endif
	}
}

static QString decodeXml(QString string)
{
	string.replace("&lt;","<");
	string.replace("&gt;",">");
	string.replace("&amp;","&");
	string.replace("&quot;","\"");
	QString fixed;
	for(QChar c:string){
		if(c>=' '){
			fixed+=c;
		}
	}
	return fixed;
}

QList<Comment> Utils::parseComment(QByteArray data,Site site)
{
	QList<Comment> list;
	switch(site){
	case Bilibili:
	{
		QStringList l=QString(data).split("<d p=\"");
		l.removeFirst();
		for(QString &item:l){
			Comment comment;
			int sta=0;
			int len=item.indexOf("\"");
			QStringList args=item.mid(sta,len).split(',');
			sta=item.indexOf(">")+1;
			len=item.indexOf("<",sta)-sta;
			comment.time=args[0].toDouble()*1000+0.5;
			comment.date=args[4].toInt();
			comment.mode=args[1].toInt();
			comment.font=args[2].toInt();
			comment.color=args[3].toInt();
			comment.sender=args[6];
			comment.string=decodeXml(item.mid(sta,len));
			list.append(comment);
		}
		break;
	}
	case AcFun:
	{
		QJsonArray a=QJsonDocument::fromJson(data).array();
		for(QJsonValue i:a){
			Comment comment;
			QJsonObject item=i.toObject();
			QStringList args=item["c"].toString().split(',');
			comment.time=args[0].toDouble()*1000+0.5;
			comment.date=args[5].toInt();
			comment.mode=args[2].toInt();
			comment.font=args[3].toInt();
			comment.color=args[1].toInt();
			comment.sender=args[4];
			comment.string=item["m"].toString();
			list.append(comment);
		}
		break;
	}
	case AcPlay:
	{
		QJsonArray a=QJsonDocument::fromJson(data).object()["Comments"].toArray();
		for(QJsonValue i:a){
			Comment comment;
			QJsonObject item=i.toObject();
			comment.time=item["Time"].toDouble()*1000+0.5;
			comment.date=item["Timestamp"].toInt();
			comment.mode=item["Mode"].toInt();
			comment.font=25;
			comment.color=item["Color"].toInt();
			comment.sender=QString::number(item["UId"].toInt());
			comment.string=item["Message"].toString();
			list.append(comment);
		}
		break;
	}
	case AcfunLocalizer:
	{
		QStringList l=QString(data).split("<l i=\"");
		l.removeFirst();
		for(QString &item:l){
			Comment comment;
			int sta=0;
			int len=item.indexOf("\"");
			QStringList args=item.mid(sta,len).split(',');
			sta=item.indexOf("<![CDATA[")+9;
			len=item.indexOf("]]>",sta)-sta;
			comment.time=args[0].toDouble()*1000+0.5;
			comment.date=args[5].toInt();
			comment.mode=args[3].toInt();
			comment.font=args[1].toInt();
			comment.color=args[2].toInt();
			comment.sender=args[4];
			comment.string=decodeXml(item.mid(sta,len));
			list.append(comment);
		}
		break;
	}
	default:
		break;
	}
	return list;
}

void Utils::loadConfig()
{
	QFile conf("./Config.txt");
	conf.open(QIODevice::ReadOnly|QIODevice::Text);
	config=QJsonDocument::fromJson(conf.readAll()).object();
	conf.close();
}

void Utils::saveConfig()
{
	QFile conf("./Config.txt");
	conf.open(QIODevice::WriteOnly|QIODevice::Text);
	conf.write(QJsonDocument(config).toJson());
	conf.close();
}

