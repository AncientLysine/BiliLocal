#include "Common.h"
#include "Quick2InterfacePrivate.h"
#include "Export.h"

using namespace UI;

Quick2InterfacePrivate::Quick2InterfacePrivate()
{
	qmlRegisterSingletonType<Local>("BiliLocal", 1, 0, "LocalApp", [](QQmlEngine *, QJSEngine *)->QObject * {
		return new Export(lApp);
	});
	qmlRegisterUncreatableType<Config>("BiliLocal", 1, 0, "Config", "acess from app");
	qmlRegisterUncreatableType<Shield>("BiliLocal", 1, 0, "Shield", "acess from app");
	qmlRegisterUncreatableType<ARender>("BiliLocal", 1, 0, "ARender", "acess from app");
	qmlRegisterUncreatableType<APlayer>("BiliLocal", 1, 0, "APlayer", "acess from app");
	qmlRegisterUncreatableType<Running>("BiliLocal", 1, 0, "Running", "acess from app");
	qmlRegisterUncreatableType<Danmaku>("BiliLocal", 1, 0, "Danmaku", "acess from app");
	qmlRegisterUncreatableType<List>("BiliLocal", 1, 0, "List", "acess from app");
	qmlRegisterUncreatableType<Load>("BiliLocal", 1, 0, "Load", "acess from app");
	qmlRegisterUncreatableType<Post>("BiliLocal", 1, 0, "Post", "acess from app");
	qmlRegisterUncreatableType<Seek>("BiliLocal", 1, 0, "Seek", "acess from app");
	qmlRegisterUncreatableType<Sign>("BiliLocal", 1, 0, "Sign", "acess from app");
	QFile file(Utils::localPath(Utils::Script) + "Quick2/Interface.qml");
	if (file.open(QIODevice::ReadOnly)) {
		engine.loadData(file.readAll());
		for (QObject *iter : engine.rootObjects()) {
			home = qobject_cast<QQuickWindow *>(iter);
			if (home) {
				break;
			}
			home = iter->findChild<QQuickWindow *>();
			if (home) {
				break;
			}
		}
	}
}

void Quick2InterfacePrivate::percent(double degree)
{
	Q_UNUSED(degree);
	//TODO
}

void Quick2InterfacePrivate::warning(QString title, QString text)
{
	Q_UNUSED(title);
	Q_UNUSED(text);
	//TODO
}
