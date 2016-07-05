#include "Common.h"
#include "Quick2Private.h"
#include "../../Local.h"
#include "../../UI/Interface.h"
#include <functional>

OpenGLQuick2RenderPrivate::OpenGLQuick2RenderPrivate()
	: uninitialized(true)
{
	window = qobject_cast<QQuickWindow *>(lApp->findObject<Interface>()->window());
	window->setClearBeforeRendering(false);
	auto setSize = [this]() {
		device->setSize(window->size());
	};
	QObject::connect(window, &QQuickWindow::widthChanged,  setSize);
	QObject::connect(window, &QQuickWindow::heightChanged, setSize);
	QObject::connect(window, &QQuickWindow::beforeRendering, [=] {
		if (uninitialized){
			initialize();
			device.reset(new QOpenGLPaintDevice(window->size()));
			uninitialized = false;
		}
		paint(device.data());
	});
	QObject::connect(window, &QQuickWindow::frameSwapped, std::bind(&OpenGLRenderPrivate::onSwapped, this));
}
