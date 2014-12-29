/*=======================================================================
*
*   Copyright (C) 2013 Lysine.
*
*   Filename:    Local.cpp
*   Time:        2013/03/18
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

#include "Local.h"
#include "APlayer.h"
#include "Config.h"
#include "Danmaku.h"
#include "Interface.h"
#include "List.h"
#include "Load.h"
#include "Plugin.h"
#include "Render.h"
#include "Shield.h"
#include "Utils.h"

QHash<QString,QObject *> Local::objects;

Local::Local(int &argc,char **argv):
	QApplication(argc,argv)
{
	QDir::setCurrent(applicationDirPath());
	setPalette(setStyle("Fusion")->standardPalette());
	setAttribute(Qt::AA_UseOpenGLES);
	Config::load();
	Shield::load();
	qsrand(QTime::currentTime().msec());
}

void Local::exit(int code)
{
	delete List::instance();
	delete Load::instance();
	Shield::save();
	Config::save();
	delete APlayer::instance();
	delete Danmaku::instance();
	QApplication::exit(code);
}

QString Local::suggestion(int code)
{
	switch (code){
	case 3:
	case 4:
		return tr("check your network connection");
	case 203:
	case 403:
	case -8:
		return tr("access denied, try login");
	default:
		return QString();
	}
}

namespace
{
void setDefaultFont()
{
	QString def=Utils::defaultFont();
	QFontInfo i(qApp->font());
	if(!QFontDatabase().families().contains(def)){
		def=i.family();
	}
	double p=i.pointSizeF();
	QFont f;
	f.setFamily(Config::getValue("/Interface/Font/Family",def));
	f.setPointSizeF(Config::getValue("/Interface/Font/Size",p));
	qApp->setFont(f);
}

void loadTranslator()
{
	QString locale=Config::getValue("/Interface/Locale",QLocale::system().name());
	QFileInfoList list;
	list+=QDir("./locale/"+locale).entryInfoList();
	list+=QFileInfo("./locale/"+locale+".qm");
	locale.resize(2);
	list+=QDir("./locale/"+locale).entryInfoList();
	list+=QFileInfo("./locale/"+locale+".qm");
	for(QFileInfo info:list){
		if(!info.isFile()){
			continue;
		}
		QTranslator *trans=new QTranslator(qApp);
		if(trans->load(info.absoluteFilePath())){
			qApp->installTranslator(trans);
		}
		else{
			delete trans;
		}
	}
}

void setToolTipBase()
{
	QPalette tip=qApp->palette();
	tip.setColor(QPalette::Inactive,QPalette::ToolTipBase,Qt::white);
	qApp->setPalette(tip);
	QToolTip::setPalette(tip);
}
}

int main(int argc,char *argv[])
{
	Local a(argc,argv);
	int single;
	if((single=Config::getValue("/Interface/Single",1))){
		QLocalSocket socket;
		socket.connectToServer("BiliLocalInstance");
		if (socket.waitForConnected()){
			QDataStream s(&socket);
			s<<a.arguments().mid(1);
			socket.waitForBytesWritten();
			return 0;
		}
	}
	loadTranslator();
	setDefaultFont();
	setToolTipBase();
	Interface w;
	Plugin::loadPlugins();
	w.show();
	w.tryLocal(a.arguments().mid(1));
	QLocalServer *server=nullptr;
	if(single){
		server=new QLocalServer(lApp);
		server->listen("BiliLocalInstance");
		QObject::connect(server,&QLocalServer::newConnection,[&](){
			QLocalSocket *r=server->nextPendingConnection();
			r->waitForReadyRead();
			QDataStream s(r);
			QStringList args;
			s>>args;
			delete r;
			w.tryLocal(args);
		});
	}
	QThread::currentThread()->setPriority(QThread::TimeCriticalPriority);
	int r;
	if((r=a.exec())==12450){
		if(server){
			delete server;
		}
		QProcess::startDetached(a.applicationFilePath(),QStringList());
		return 0;
	}
	else{
		return r;
	}
}
