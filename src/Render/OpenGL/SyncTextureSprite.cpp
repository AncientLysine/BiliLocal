#include "Common.h"
#include "SyncTextureSprite.h"
#include "OpenGLRenderPrivate.h"
#include "../ARender.h"
#include "../../Sample.h"
#include "../../Graphic/GraphicPrivate.h"

SyncTextureSprite::SyncTextureSprite(OpenGLRenderPrivate *render)
	: render(render)
{
}

void SyncTextureSprite::prepare()
{
}

void SyncTextureSprite::draw(QPainter *)
{
	Sample s("SyncTextureSprite::draw");

	if (!size.isValid()) {
		QFont f = font;
		f.setPixelSize(f.pixelSize() * qApp->devicePixelRatio());
		render->resource->manager.insert({ text, f, effect }, sprites, size);
	}

	if (sprites.isEmpty()) {
		return;
	}

	for(int y = 0; y < size.height(); ++y){
		for (int x = 0; x < size.width(); ++x) {
			OpenGLSprite &s = sprites[y * size.width() + x];
			QRectF draw(QPointF(x, y) * OpenGLAtlas::MaxSize, s.rect.size());
			draw = render->scaleRect(draw, 1 / qApp->devicePixelRatio());
			draw = transform.mapRect(draw);
			draw = render->scaleRect(draw, 1 * qApp->devicePixelRatio());
			render->appendDrawCall(draw, s.rect, s.getTexture(), color);
		}
	}
}

QSize SyncTextureSprite::getSize()
{
	if (size.isNull()) {
		return QSize(0, 0);
	}
	else if (size.isValid()) {
		int w = 0, h = 0, sw = size.width(), sh = size.height();
		for (int x = 0; x < sw; ++x) {
			w += sprites[x].rect.width();
		}
		for (int y = 0; y < sh; ++y) {
			h += sprites[y * sw].rect.height();
		}
		QSize s(w, h);
		s /= qApp->devicePixelRatio();
		return s;
	}
	else {
		QFont f = font;
		f.setPixelSize(f.pixelSize() * qApp->devicePixelRatio());
		const int pad = OpenGLAtlas::Padding;
		QSize s = GraphicPrivate::getSize(text, f);
		s += QSize(pad, pad) * 2;
		s /= qApp->devicePixelRatio();
		return s;
	}
}
