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
#include "Config.h"
#include "History.h"

Utils::Site Utils::getSite(QString url)
{
	if(url.indexOf("letv.com")!=-1){
		return Letv;
	}
	else if(url.indexOf("bilibili")!=-1){
		return Bilibili;
	}
	else if(url.indexOf("acfun")!=-1){
		return AcFun;
	}
	else if(url.indexOf("acplay")!=-1){
		return AcPlay;
	}
	else{
		return Unknown;
	}
}

#ifndef EMBEDDED
void Utils::setCenter(QWidget *widget)
{
	QRect rect=widget->geometry();
	QWidget *parent=widget->parentWidget();
	if(parent==NULL){
		rect.moveCenter(QApplication::desktop()->screenGeometry(widget).center());
	}
	else{
		if(widget->isWindow()){
			QPoint center=parent->geometry().center();
			if((parent->windowFlags()&Qt::CustomizeWindowHint)){
				center.ry()+=widget->style()->pixelMetric(QStyle::PM_TitleBarHeight)/2;
			}
			rect.moveCenter(center);
		}
		else{
			rect.moveCenter(parent->rect().center());
		}
	}
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
#endif

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
	exp=exp.trimmed();
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
	QString path=History::instance()->lastPath();
	if(!path.isEmpty()){
		return QFileInfo(path).absolutePath();
	}
	else{
		QStringList paths=QStandardPaths::standardLocations(QStandardPaths::MoviesLocation);
		paths.append(QDir::homePath());
		return paths.front();
	}
}

QString Utils::defaultFont(bool monospace)
{
	if(monospace){
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
	switch(site){
	case AcFun:
		return Config::getValue("/Network/Url/AcFun",
								QString("acfun.tv"));
	case Bilibili:
		return Config::getValue("/Network/Url/Bilibili",
								QString("bilibili.com"));
	case AcPlay:
		return Config::getValue("/Network/Url/AcPlay",
								QString("acplay.net"));
	default:
		return QString();
	}
}

QString Utils::decodeXml(QString string,bool fast)
{
#ifndef EMBEDDED
	if(!fast){
		QTextDocument text;
		text.setHtml(string);
		return text.toPlainText();
	}
#else
	Q_UNUSED(fast)
#endif
	QString fixed;
	fixed.reserve(string.length());
	int i=0,l=string.length();
	for(i=0;i<l;++i){
		QChar c=string[i];
		if(c>=' '||c=='\n'){
			bool f=true;
			switch(c.unicode()){
			case '&':
				if(l-i>=4){
					switch(string[i+1].unicode()){
					case 'l':
						if(string[i+2]=='t'&&string[i+3]==';'){
							fixed+='<';
							f=false;
							i+=3;
						}
						break;
					case 'g':
						if(string[i+2]=='t'&&string[i+3]==';'){
							fixed+='>';
							f=false;
							i+=3;
						}
						break;
					case 'a':
						if(l-i>=5&&string[i+2]=='m'&&string[i+3]=='p'&&string[i+4]==';'){
							fixed+='&';
							f=false;
							i+=4;
						}
						break;
					case 'q':
						if(l-i>=6&&string[i+2]=='u'&&string[i+3]=='o'&&string[i+4]=='t'&&string[i+5]==';'){
							fixed+='\"';
							f=false;
							i+=5;
						}
						break;
					}
				}
				break;
			case '/' :
			case '\\':
				if(l-i>=2){
					switch(string[i+1].unicode()){
					case 'n':
						fixed+='\n';
						f=false;
						i+=1;
						break;
					case 't':
						fixed+='\t';
						f=false;
						i+=1;
						break;
					case '\"':
						fixed+='\"';
						f=false;
						i+=1;
						break;
					}
				}
				break;
			}
			if(f){
				fixed+=c;
			}
		}
	}
	return fixed;
}

QStringList Utils::getRenderModules()
{
	QStringList modules;
#ifdef RENDER_RASTER
	modules<<"Raster";
#endif
#ifdef RENDER_OPENGL
	modules<<"OpenGL";
#endif
#ifdef RENDER_DETACH
	modules<<"Detach";
#endif
	return modules;
}

QStringList Utils::getDecodeModules()
{
	QStringList modules;
#ifdef BACKEND_VLC
	modules<<"VLC";
#endif
#ifdef BACKEND_QMM
	modules<<"QMM";
#endif
#ifdef BACKEND_NIL
	modules<<"NIL";
#endif
	return modules;
}

QStringList Utils::getSuffix(int type,QString format)
{
	QStringList set;
	if(type&Video){
		set<<"3g2"<<"3gp"<<"3gp2"<<"3gpp"<<"amv"<<"asf"<<"avi"<<"divx"<<"drc"<<"dv"<<
			 "f4v"<<"flv"<<"gvi"<<"gxf"<<"hlv"<<"iso"<<"letv"<<
			 "m1v"<<"m2t"<<"m2ts"<<"m2v"<<"m4v"<<"mkv"<<"mov"<<
			 "mp2"<<"mp2v"<<"mp4"<<"mp4v"<<"mpe"<<"mpeg"<<"mpeg1"<<
			 "mpeg2"<<"mpeg4"<<"mpg"<<"mpv2"<<"mts"<<"mtv"<<"mxf"<<"mxg"<<"nsv"<<"nuv"<<
			 "ogg"<<"ogm"<<"ogv"<<"ogx"<<"ps"<<
			 "rec"<<"rm"<<"rmvb"<<"tod"<<"ts"<<"tts"<<"vob"<<"vro"<<
			 "webm"<<"wm"<<"wmv"<<"wtv"<<"xesc";
	}
	if(type&Audio){
		int size=set.size();
		set<<"3ga"<<"669"<<"a52"<<"aac"<<"ac3"<<"adt"<<"adts"<<"aif"<<"aifc"<<"aiff"<<
			 "amr"<<"aob"<<"ape"<<"awb"<<"caf"<<"dts"<<"flac"<<"it"<<"kar"<<
			 "m4a"<<"m4p"<<"m5p"<<"mka"<<"mlp"<<"mod"<<"mp1"<<"mp2"<<"mp3"<<"mpa"<<"mpc"<<"mpga"<<
			 "oga"<<"ogg"<<"oma"<<"opus"<<"qcp"<<"ra"<<"rmi"<<"s3m"<<"spx"<<"thd"<<"tta"<<
			 "voc"<<"vqf"<<"w64"<<"wav"<<"wma"<<"wv"<<"xa"<<"xm";
		std::inplace_merge(set.begin(),set.begin()+size,set.end());
	}
	if(type&Subtitle){
		int size=set.size();
		set<<"aqt"<<"ass"<<"cdg"<<"dks"<<"idx"<<"jss"<<"mks"<<"mpl2"<<"pjs"<<"psb"<<"rt"<<
			 "smi"<<"smil"<<"srt"<<"ssa"<<"stl"<<"sub"<<"txt"<<"usf"<<"utf";
		std::inplace_merge(set.begin(),set.begin()+size,set.end());
	}
	if(type&Danmaku){
		int size=set.size();
		set<<"json"<<"xml";
		std::inplace_merge(set.begin(),set.begin()+size,set.end());
	}
	if(!format.isEmpty()){
		for(QString &iter:set){
			iter=format.arg(iter);
		}
	}
	return set;
}

QList<Comment> Utils::parseComment(QByteArray data,Site site)
{
	QList<Comment> list;
	switch(site){
	case Bilibili:
	{
		QStringList l=QString(data).split("<d p=\"");
		l.removeFirst();
		for(const QString &item:l){
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
			comment.string=decodeXml(item.mid(sta,len),true);
			list.append(comment);
		}
		break;
	}
	case AcFun:
	{
		QQueue<QJsonArray> queue;
		queue.append(QJsonDocument::fromJson(data).array());
		while(!queue.isEmpty()){
			for(const QJsonValue &i:queue.front()){
				if(i.isArray()){
					queue.append(i.toArray());
				}
				else{
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
			}
			queue.dequeue();
		}
		break;
	}
	case AcPlay:
	{
		QJsonArray a=QJsonDocument::fromJson(data).object()["Comments"].toArray();
		for(const QJsonValue &i:a){
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
		for(const QString &item:l){
			Comment comment;
			int sta=0;
			int len=item.indexOf("\"");
			QStringList args=item.mid(sta,len).split(',');
			sta=item.indexOf("<![CDATA[")+9;
			len=item.indexOf("]]>",sta)-sta;
			comment.time=args[0].toDouble()*1000+0.5;
			comment.date=args[5].toInt();
			comment.mode=1;
			comment.font=25;
			comment.color=args[2].toInt();
			comment.sender=args[4];
			comment.string=decodeXml(item.mid(sta,len),true);
			list.append(comment);
		}
		break;
	}
	case Niconico:
	{
		QStringList l=QString(data).split("<chat ");
		l.removeFirst();
		for(const QString &item:l){
			Comment comment;
			QString key,val;
			/* 0 wait for key
			 * 1 wait for left quot
			 * 2 wait for value
			 * 3 wait for comment
			 * 4 finsihed */
			int state=0;
			QMap<QString,QString> args;
			for(const QChar &c:item){
				switch(state){
				case 0:
					if(c=='='){
						state=1;
					}
					else if(c=='>'){
						state=3;
					}
					else if(c!=' '){
						key.append(c);
					}
					break;
				case 1:
					if(c=='\"'){
						state=2;
					}
					break;
				case 2:
					if(c=='\"'){
						state=0;
						args.insert(key,val);
						key=val=QString();
					}
					else{
						val.append(c);
					}
					break;
				case 3:
					if(c=='<'){
						state=4;
					}
					else{
						comment.string.append(c);
					}
					break;
				}
			}
			if(state!=4){
				continue;
			}
			comment.time=args["vpos"].toLongLong()*10;
			comment.date=args["date"].toLongLong();
			QStringList ctrl=args["mail"].split(' ',QString::SkipEmptyParts);
			comment.mode=ctrl.contains("shita")?4 :(ctrl.contains("ue") ?5 :1 );
			comment.font=ctrl.contains("small")?15:(ctrl.contains("big")?36:25);
			comment.color=0xFFFFFF;
			for(const QString &name:ctrl){
				QColor color(name);
				if(color.isValid()){
					comment.color=color.rgb();
					break;
				}
			}
			comment.sender=args["user_id"];
			list.append(comment);
		}
		break;
	}
	default:
		break;
	}
	return list;
}
