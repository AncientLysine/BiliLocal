#include "Common.h"
#include "SyncTextureSprite.h"
#include "../ARender.h"
#include "OpenGLRenderPrivate.h"
#include "../../Graphic/GraphicPrivate.h"
#include "../../Utils.h"

SyncTextureSprite::SyncTextureSprite(const Comment &comment, OpenGLRenderPrivate *render) :
texture(0), render(render)
{
	QSize size = ARender::instance()->getActualSize();
	QFont font = GraphicPrivate::getFont(comment.font*GraphicPrivate::getScale(comment.mode, comment.date, size));
	QSize need = GraphicPrivate::getSize(comment.string, font);
	source = new QImage(GraphicPrivate::getCache(comment.string, comment.color, font, need, comment.isLocal()));
}

SyncTextureSprite::~SyncTextureSprite()
{
	if (texture)
		render->glDeleteTextures(1, &texture);
	delete source;
}

void SyncTextureSprite::draw(QPainter *painter, QRectF dest)
{
	if (!texture){
		render->glGenTextures(1, &texture);
		render->loadTexture(texture, 4, source->width(), source->height(), source->bits(), 4);
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

QSize SyncTextureSprite::getSize()
{
	return source ? source->size() : QSize();
}
