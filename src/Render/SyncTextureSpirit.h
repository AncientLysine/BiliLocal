#pragma once

#include "ISpirit.h"
#include "OpenGLRenderPrivate.h"

class SyncTextureSpirit :public ISpirit
{
public:
	GLuint texture;
	QImage *source;
	OpenGLRenderPrivate *const render;

	SyncTextureSpirit(const QImage &image, OpenGLRenderPrivate *render) :
		texture(0), render(render)
	{
		source = new QImage(image);
	}

	~SyncTextureSpirit()
	{
		if (texture)
			render->glDeleteTextures(1, &texture);
		delete source;
	}

	void draw(QPainter *painter, QRectF dest)
	{
		if (!texture){
			render->glGenTextures(1, &texture);
			render->loadTexture(texture, 4, source->width(), source->height(), source->bits());
			delete source;
			source = nullptr;
		}
		painter->beginNativePainting();
		QRect rect(QPoint(0, 0), ARender::instance()->getActualSize());
		render->glEnable(GL_BLEND);
		render->glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
		render->drawTexture(&texture, 4, dest, rect);
		painter->endNativePainting();
	}
};
