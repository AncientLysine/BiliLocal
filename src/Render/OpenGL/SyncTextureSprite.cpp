#include "Common.h"
#include "SyncTextureSprite.h"
#include "OpenGLRenderPrivate.h"
#include "../ARender.h"
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
	if (!size.isValid()) {
		render->manager->insert({ text, font, effect }, sprites, size);
	}

	if (sprites.isEmpty()) {
		return;
	}

	for(int y = 0; y < size.height(); ++y){
		for (int x = 0; x < size.width(); ++x) {
			Sprite &s = sprites[y * size.width() + x];
			QRectF draw(QPointF(x, y) * Atlas::MaxSize, s.rect.size());
			draw = transform.mapRect(draw);
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
		return QSize(w, h);
	}
	else {
		constexpr int pad = Atlas::Padding;
		return GraphicPrivate::getSize(text, font) + QSize(pad, pad) * 2;
	}
}
