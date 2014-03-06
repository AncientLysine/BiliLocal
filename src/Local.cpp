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

#include "Utils.h"
#include "Shield.h"
#include "Cookie.h"
#include "Interface.h"
#include <QtCore>

static void loadTranslator()
{
	QString path="./locale/"+QLocale::system().name();
	QFileInfoList list=QDir(path).entryInfoList();
	list.append(QFileInfo(path+".qm"));
	for(QFileInfo info:list){
		if(info.isFile()){
			QTranslator *trans=new QTranslator(qApp);
			if(trans->load(info.absoluteFilePath())){
				qApp->installTranslator(trans);
			}
			else{
				delete trans;
			}
		}
	}
}

static void setDefaultFont()
{
	QString def=Utils::defaultFont();
	QFont f=qApp->font();
	if(!QFontDatabase().families().contains(def)){
		def=QFontInfo(f).family();
	}
	f.setFamily(Utils::getConfig("/Interface/Font",def));
	qApp->setFont(f);
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
	QApplication::setStyle("Fusion");
	QApplication a(argc,argv);
	QLocalSocket socket;
	socket.connectToServer("BiliLocalInstance");
	if(socket.waitForConnected()){
		QDataStream s(&socket);
		s<<a.arguments();
		socket.waitForBytesWritten();
		return 0;
	}
	QDir::setCurrent(a.applicationDirPath());
	Utils::loadConfig();
	Shield::load();
	Cookie::load();
	loadTranslator();
	setDefaultFont();
	setToolTipBase();
	a.connect(&a,&QApplication::aboutToQuit,[](){
		Cookie::save();
		Shield::save();
		Utils::saveConfig();
	});
	qsrand(QTime::currentTime().msec());
	Interface w;
	w.show();
	w.parseArgs(a.arguments());
	QLocalServer single;
	single.listen("BiliLocalInstance");
	QObject::connect(&single,&QLocalServer::newConnection,[&](){
		QLocalSocket *r=single.nextPendingConnection();
		r->waitForReadyRead();
		QDataStream s(r);
		QStringList args;
		s>>args;
		r->deleteLater();
		w.parseArgs(args);
	});
	int r;
	if((r=a.exec())==12450){
		QProcess::startDetached(a.applicationFilePath(),QStringList());
		return 0;
	}
	else{
		return r;
	}
}
