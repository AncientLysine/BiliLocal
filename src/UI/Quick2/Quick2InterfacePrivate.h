#pragma once

#include "../InterfacePrivate.h"
#include <QtCore>
#include <QtQml>

class Quick2InterfacePrivate : public InterfacePrivate
{
public:
	Quick2InterfacePrivate();

	virtual void percent(double degree) override;
	virtual void warning(QString title, QString text) override;

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
	QQmlApplicationEngine engine;
	const QString path;
	QPointer<QQuickWindow> home;
};
