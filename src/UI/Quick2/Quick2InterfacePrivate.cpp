#include "Common.h"
#include "Quick2InterfacePrivate.h"
#include "Export.h"

using namespace UI;

Quick2InterfacePrivate::Quick2InterfacePrivate()
	: path(Utils::localPath(Utils::Script) + "Quick2/")
{
	qmlRegisterSingletonType<Local>("BiliLocal", 1, 0, "LocalApp", [](QQmlEngine *, QJSEngine *)->QObject * {
		return new Export(lApp);
	});
	const QString &message = QStringLiteral("Local: can't create module out of app.");
	qmlRegisterUncreatableType<Config>("BiliLocal", 1, 0, "Config", message);
	qmlRegisterUncreatableType<Shield>("BiliLocal", 1, 0, "Shield", message);
	qmlRegisterUncreatableType<ARender>("BiliLocal", 1, 0, "ARender", message);
	qmlRegisterUncreatableType<APlayer>("BiliLocal", 1, 0, "APlayer", message);
	qmlRegisterUncreatableType<Running>("BiliLocal", 1, 0, "Running", message);
	qmlRegisterUncreatableType<Danmaku>("BiliLocal", 1, 0, "Danmaku", message);
	qmlRegisterUncreatableType<List>("BiliLocal", 1, 0, "List", message);
	qmlRegisterUncreatableType<Load>("BiliLocal", 1, 0, "Load", message);
	qmlRegisterUncreatableType<Post>("BiliLocal", 1, 0, "Post", message);
	qmlRegisterUncreatableType<Seek>("BiliLocal", 1, 0, "Seek", message);
	qmlRegisterUncreatableType<Sign>("BiliLocal", 1, 0, "Sign", message);
	engine.addImportPath(path);
	engine.load(QUrl::fromUserInput(path + "Interface.qml"));
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
