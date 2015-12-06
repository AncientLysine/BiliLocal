#pragma once

#include "../ISprite.h"
#include <QOpenGLFunctions>

class OpenGLRenderPrivate;

class SyncTextureSprite :public ISprite
{
public:
	GLuint texture;
	QImage *source;
	OpenGLRenderPrivate *const render;

	SyncTextureSprite(const Comment &comment, OpenGLRenderPrivate *render);
	virtual ~SyncTextureSprite();
	void draw(QPainter *painter, QRectF dest);
	QSize getSize();
};
