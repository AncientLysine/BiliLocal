#include "Common.h"
#include "WindowPrivate.h"

OpenGLWindowRenderPrivate::OpenGLWindowRenderPrivate()
{
	middle = new FParent(lApp->mainWidget());
	middle->lower();
	window = new OWindow(this);
	widget = QWidget::createWindowContainer(window, middle);
	middle->setAcceptDrops(false);
	widget->setAcceptDrops(false);
}
