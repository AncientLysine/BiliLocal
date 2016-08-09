#pragma once

#include "../ASprite.h"
#include "OpenGLAtlas.h"

class OpenGLRenderPrivate;

class SyncTextureSprite : public ASprite
{
public:
	SyncTextureSprite(OpenGLRenderPrivate *render);
	virtual void prepare() override;
	virtual void draw(QPainter *painter) override;
	virtual QSize getSize() override;

private:
	QList<OpenGLSprite> sprites;
	QSize size;
	OpenGLRenderPrivate *const render;
};
