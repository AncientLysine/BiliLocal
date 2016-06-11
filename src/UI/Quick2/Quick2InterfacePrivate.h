#pragma once

#include "../Interface.h"
#include "../InterfacePrivate.h"
#include "Export.h"
#include <QtCore>
#include <QtQml>

using namespace UI;

class Quick2InterfacePrivate : public InterfacePrivate, public QQmlApplicationEngine
{
public:
	Quick2InterfacePrivate()
	{
		qmlRegisterSingletonType<Local>("BiliLocal", 1, 0, "LocalApp", [](QQmlEngine *, QJSEngine *)->QObject * {
			return new Export(lApp);
		});
		qmlRegisterUncreatableType<ARender>("BiliLocal", 1, 0, "ARender", "acess from app");
		qmlRegisterUncreatableType<APlayer>("BiliLocal", 1, 0, "APlayer", "acess from app");
		qmlRegisterUncreatableType<Running>("BiliLocal", 1, 0, "Running", "acess from app");
		qmlRegisterUncreatableType<Danmaku>("BiliLocal", 1, 0, "Danmaku", "acess from app");
		qmlRegisterUncreatableType<List>("BiliLocal", 1, 0, "List", "acess from app");
		qmlRegisterUncreatableType<Load>("BiliLocal", 1, 0, "Load", "acess from app");
		qmlRegisterUncreatableType<Post>("BiliLocal", 1, 0, "Post", "acess from app");
		qmlRegisterUncreatableType<Seek>("BiliLocal", 1, 0, "Seek", "acess from app");
		qmlRegisterUncreatableType<Sign>("BiliLocal", 1, 0, "Sign", "acess from app");
		load("./sctipts/quick2/home.qml");
		for (QObject *iter : rootObjects()) {
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

	virtual void percent(double degree) override
	{
	}

	virtual void warning(QString title, QString text) override
	{
	}

	virtual void show() override
	{
	}

	virtual void hide() override
	{
	}

	virtual QWidget *widget() override
	{
		return nullptr;
	}

	virtual QWindow *window() override
	{
		return home;
	}

private:
	QPointer<QQuickWindow> home;
};
