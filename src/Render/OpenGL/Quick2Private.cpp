#include "Common.h"
#include "Quick2Private.h"
#include "../../Local.h"
#include "../../Sample.h"
#include "../../UI/Interface.h"
#include <functional>

OpenGLQuick2RenderPrivate::OpenGLQuick2RenderPrivate()
	: uninitialized(true)
{
	window = qobject_cast<QQuickWindow *>(lApp->findObject<Interface>()->window());
	window->setClearBeforeRendering(false);
	QObject::connect(window, &QQuickWindow::beforeRendering, [=] {
		Sample s("Quick2Render::paint");

		if (uninitialized){
			initialize();
			device.reset(new QOpenGLPaintDevice());
			uninitialized = false;
		}
		double ratio = window->devicePixelRatio();
		device->setDevicePixelRatio(ratio);
		device->setSize(window->size() * ratio);
		paint(device.data());
	});
	QObject::connect(window, &QQuickWindow::frameSwapped, std::bind(&OpenGLRenderPrivate::onSwapped, this));
}
