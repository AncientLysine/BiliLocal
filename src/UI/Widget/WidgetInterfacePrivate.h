#pragma once

#include "../InterfacePrivate.h"
#include <QString>

class QWidget;
class QWindow;

namespace UI
{
	class Home;
}

class WidgetInterfacePrivate : public InterfacePrivate
{
public:
	WidgetInterfacePrivate();
	virtual ~WidgetInterfacePrivate();

	virtual void percent(double degree) override;
	virtual void warning(QString title, QString text) override;

	virtual void show() override;
	virtual void hide() override;

	virtual QWidget *widget() override;
	virtual QWindow *window() override;

private:
	UI::Home *home;
};
