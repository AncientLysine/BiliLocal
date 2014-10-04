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
#include "Utils.h"
#include "Config.h"
#include "Shield.h"

QHash<QString,QObject *> Local::objects;

static void setDefaultFont()
{
	QString def=Utils::defaultFont();
	QFont f=qApp->font();
	if(!QFontDatabase().families().contains(def)){
		def=QFontInfo(f).family();
	}
	f.setFamily(Config::getValue("/Interface/Font/Family",def));
	f.setPointSizeF(Config::getValue("/Interface/Font/Size",f.pointSizeF()));
	qApp->setFont(f);
}

#ifdef EMBEDDED
#include <jni.h>

Local::Local(int &argc,char **argv):
	QGuiApplication(argc,argv)
{
	QDir::setCurrent(QStandardPaths::writableLocation(QStandardPaths::DataLocation));
	Config::load();
	Shield::load();
	setDefaultFont();
	connect(this,&Local::aboutToQuit,[](){
		Shield::save();
		Config::save();
	});
	qsrand(QTime::currentTime().msec());
}

extern "C" JNIEXPORT void JNICALL Java_tv_danmaku_local_jni_LocalJNI_init
  (JNIEnv *, jobject)
{
}

extern "C" JNIEXPORT void JNICALL Java_tv_danmaku_local_jni_LocalJNI_free
  (JNIEnv *, jobject)
{
}
#else
#include "Plugin.h"
#include "Interface.h"

Local::Local(int &argc,char **argv):
	QApplication(argc,argv)
{
}

static void loadTranslator()
{
	QString locale=QLocale::system().name();
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

static void setToolTipBase()
{
	QPalette tip=qApp->palette();
	tip.setColor(QPalette::Inactive,QPalette::ToolTipBase,Qt::white);
	qApp->setPalette(tip);
	QToolTip::setPalette(tip);
}

int main(int argc,char *argv[])
{
	QDir::setCurrent(QFileInfo(QString::fromLocal8Bit(argv[0])).absolutePath());
	Local::addLibraryPath("./plugins");
	Local::setStyle("Fusion");
	Local a(argc,argv);
	Config::load();
	int single;
	if((single=Config::getValue("/Interface/Single",1))){
		QLocalSocket socket;
		socket.connectToServer("BiliLocalInstance");
		if(socket.waitForConnected()){
			QDataStream s(&socket);
			s<<a.arguments().mid(1);
			socket.waitForBytesWritten();
			return 0;
		}
	}
	Shield::load();
	loadTranslator();
	setDefaultFont();
	setToolTipBase();
	a.connect(&a,&Local::aboutToQuit,[](){
		Shield::save();
		Config::save();
	});
	qsrand(QTime::currentTime().msec());
	Interface w;
	Plugin::loadPlugins();
	w.show();
	w.tryLocal(a.arguments().mid(1));
	QLocalServer *server=NULL;
	if(single){
		server=new QLocalServer(qApp);
		server->listen("BiliLocalInstance");
		QObject::connect(server,&QLocalServer::newConnection,[&](){
			QLocalSocket *r=server->nextPendingConnection();
			r->waitForReadyRead();
			QDataStream s(r);
			QStringList args;
			s>>args;
			r->deleteLater();
			w.tryLocal(args);
		});
	}
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
#endif
