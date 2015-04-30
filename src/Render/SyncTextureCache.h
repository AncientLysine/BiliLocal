#pragma once

#include "ARender.h"
#include "OpenGLRenderPrivateBase.h"

class SyncTextureTCache :public ARender::ICache
{
public:
	GLuint texture;
	QImage source;
	OpenGLRenderPrivateBase *render;

	SyncTextureTCache(const QImage &image, OpenGLRenderPrivateBase *render) :
		texture(0), source(image), render(render)
	{
	}

	~SyncTextureTCache()
	{
		if (texture)
			render->glDeleteTextures(1, &texture);
	}

	void draw(QPainter *painter, QRectF dest)
	{
		if (!texture){
			render->glGenTextures(1, &texture);
			render->uploadTexture(texture, 4, source.width(), source.height(), (quint8 *)source.bits());
			source = QImage();
		}
		painter->beginNativePainting();
		QRect rect(QPoint(0, 0), ARender::instance()->getActualSize());
		render->glEnable(GL_BLEND);
		render->glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
		render->drawTexture(&texture, 4, dest, rect);
		painter->endNativePainting();
	}
};
