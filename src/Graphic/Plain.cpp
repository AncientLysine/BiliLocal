#include "Plain.h"
#include "GraphicPrivate.h"

using namespace GraphicPrivate;

Plain::Plain(const Comment &comment)
{
	QSize size = ARender::instance()->getActualSize();
	source = &comment;
	QFont font = getFont(comment.font*getScale(comment.mode, comment.date, size));
	QSize need = getSize(comment.string, font);
	rect.setSize(need);
	const QImage &image = getCache(comment.string, comment.color, font, need, comment.isLocal());
	cache = ARender::instance()->getCache(image);
}

void Plain::draw(QPainter *painter)
{
	if (enabled){
		cache->draw(painter, rect);
	}
}