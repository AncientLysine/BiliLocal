#include "Common.h"
#include "Quick2Private.h"
#include "../../Local.h"
#include "../../UI/Interface.h"
#include <functional>

OpenGLQuick2RenderPrivate::OpenGLQuick2RenderPrivate()
{
	window = qobject_cast<QQuickWindow *>(lApp->findObject<Interface>()->window());
	window->setClearBeforeRendering(false);
	initialize();
	auto setSize = [this]() {
		device.setSize(window->size());
	};
	setSize();
	QObject::connect(window, &QQuickWindow::widthChanged,  setSize);
	QObject::connect(window, &QQuickWindow::heightChanged, setSize);
	QObject::connect(window, &QQuickWindow::beforeRendering, [this] {
		paint(&device);
	});
	QObject::connect(window, &QQuickWindow::frameSwapped, std::bind(&ElapsedTimer::swap, &timer));
}
