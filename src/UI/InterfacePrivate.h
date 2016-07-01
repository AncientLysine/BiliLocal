#pragma once

#include <QtCore>

class QWidget;
class QWindow;

class InterfacePrivate
{
public:
	virtual ~InterfacePrivate() = default;
	virtual void percent(double degree) = 0;
	virtual void warning(QString title, QString text) = 0;
	virtual void show() = 0;
	virtual void hide() = 0;
	virtual QWidget *widget() = 0;
	virtual QWindow *window() = 0;
};
