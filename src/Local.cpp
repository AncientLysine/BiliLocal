/*=======================================================================
*
*   Copyright (C) 2013-2015 Lysine.
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

#include "Common.h"
#include "Local.h"
#include "Config.h"
#include "Plugin.h"
#include "Utils.h"
#include "Access/Load.h"
#include "Access/Post.h"
#include "Access/Seek.h"
#include "Access/Sign.h"
#include "Model/Danmaku.h"
#include "Model/Running.h"
#include "Model/List.h"
#include "Model/Shield.h"
#include "Player/APlayer.h"
#include "Render/ARender.h"
#include "UI/Interface.h"
#include <type_traits>

Local *Local::ins = nullptr;

Local *Local::instance()
{
	return ins ? ins : new Local(qApp);
}

Local::Local(QObject *parent)
	: QObject(parent)
{
	ins = this;
	setObjectName("Local");
#define lIns(ModuleType) (this->objects[#ModuleType] = new ModuleType(this))
	lIns(Config);
	lIns(Shield);
	lIns(Danmaku);
	lIns(Running);
	lIns(Interface);
	objects["APlayer"] = APlayer::create(this);
	objects["ARender"] = ARender::create(this);
	lIns(Load);
	lIns(Post);
	lIns(Seek);
	lIns(Sign);
	lIns(List);
#define lSet(ModuleType) static_cast<ModuleType *>(this->objects[#ModuleType])->setup()
	lSet(Interface);
	lSet(ARender);
	lSet(Running);
#undef lIns
#undef lSet
	connect(qApp, &QCoreApplication::aboutToQuit, this, [this]() {
		delete this;
	});
}

Local::~Local()
{
	Config::save();
	delete findObject<APlayer>();
	delete findObject<Running>();
	delete findObject<ARender>();
}

void Local::tryLocal(QString path)
{
	QFileInfo info(path);
	QString suffix = info.suffix().toLower();
	if (info.exists() == false) {
		return;
	}
	else if (Utils::getSuffix(Utils::Danmaku).contains(suffix)) {
		findObject<Load>()->loadDanmaku(path);
	}
	else if (findObject<APlayer>()->getState() != APlayer::Stop
		&& Utils::getSuffix(Utils::Subtitle).contains(suffix)) {
		findObject<APlayer>()->addSubtitle(path);
	}
	else {
		switch (Config::getValue("/Interface/Single", 1)) {
		case 0:
		case 1:
			findObject<APlayer>()->stop();
			findObject<APlayer>()->setMedia(path);
			break;
		case 2:
			findObject<List>()->appendMedia(path);
			break;
		}
	}
}

namespace
{
	void setDefaultFont()
	{
		QString def = Utils::defaultFont();
		QFontInfo i(qApp->font());
		if (!QFontDatabase().families().contains(def)){
			def = i.family();
		}
		double p = i.pointSizeF();
		QFont f;
		f.setFamily(Config::getValue("/Interface/Font/Family", def));
		f.setPointSizeF(Config::getValue("/Interface/Font/Size", p));
		qApp->setFont(f);
	}

	void loadTranslator()
	{
		QString locale = Config::getValue("/Interface/Locale", QLocale::system().name());
		QFileInfoList list;
		list += QDir("./locale/" + locale).entryInfoList();
		list += QFileInfo("./locale/" + locale + ".qm");
		locale.resize(2);
		list += QDir("./locale/" + locale).entryInfoList();
		list += QFileInfo("./locale/" + locale + ".qm");
		for (QFileInfo info : list){
			if (!info.isFile()){
				continue;
			}
			QTranslator *trans = new QTranslator(qApp);
			if (trans->load(info.absoluteFilePath())){
				qApp->installTranslator(trans);
			}
			else{
				delete trans;
			}
		}
	}
}

int main(int argc, char *argv[])
{
	std::remove_pointer<decltype(qApp)>::type a(argc, argv);
	QDir::setCurrent(a.applicationDirPath());
	Config::load();
	int single = Config::getValue("/Interface/Single", 1);
	if (single){
		QLocalSocket socket;
		socket.connectToServer("BiliLocalInstance");
		if (socket.waitForConnected()){
			QDataStream s(&socket);
			s << a.arguments().mid(1);
			socket.waitForBytesWritten();
			return 0;
		}
	}
	a.setAttribute(Qt::AA_UseOpenGLES);
	qThreadPool->setMaxThreadCount(Config::getValue("/Danmaku/Thread", QThread::idealThreadCount()));
	qsrand(QTime::currentTime().msec());
	loadTranslator();
	setDefaultFont();
	new Local(&a);
	Plugin::load();
	lApp->findObject<Interface>()->show();
	for (const QString iter : a.arguments().mid(1)) {
		lApp->tryLocal(iter);
	}
	QLocalServer *server = nullptr;
	if (single){
		server = new QLocalServer(&a);
		server->listen("BiliLocalInstance");
		QObject::connect(server, &QLocalServer::newConnection, [=](){
			QLocalSocket *r = server->nextPendingConnection();
			r->waitForReadyRead();
			QDataStream s(r);
			QStringList args;
			s >> args;
			delete r;
			for (const QString iter : args) {
				lApp->tryLocal(iter);
			}
		});
	}
	int r = a.exec();
	if (r == 12450){
		if (server){
			delete server;
		}
		QProcess::startDetached(a.applicationFilePath(), QStringList());
		return 0;
	}
	else{
		return r;
	}
}
