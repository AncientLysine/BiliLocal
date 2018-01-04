#include "Common.h"
#include "WidgetInterfacePrivate.h"
#include "Home.h"

using namespace UI;

WidgetInterfacePrivate::WidgetInterfacePrivate()
{
	home = new Home();
}

WidgetInterfacePrivate::~WidgetInterfacePrivate()
{
	delete home;
}

void WidgetInterfacePrivate::percent(double degree)
{
	home->percent(degree);
}

void WidgetInterfacePrivate::warning(QString title, QString text)
{
	home->warning(title, text);
}

void WidgetInterfacePrivate::show()
{
	if (home->testAttribute(Qt::WA_WState_ExplicitShowHide) == false) {
		home->show();
	}
}

void WidgetInterfacePrivate::hide()
{
	home->hide();
}

QWidget * WidgetInterfacePrivate::widget()
{
	return home;
}

QWindow * WidgetInterfacePrivate::window()
{
	return home->backingStore()->window();
}
