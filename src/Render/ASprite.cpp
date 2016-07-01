#include "Common.h"
#include "ASprite.h"
#include "ARender.h"
#include "../Graphic/GraphicPrivate.h"
#include "../Local.h"
#include "../Utils.h"

void ASprite::setAuto(const Comment & comment)
{
	color = QColor::fromRgb(comment.color);
	color.setAlphaF(Config::getValue("/Danmaku/Alpha", 100) / 100.0);
	effect = Config::getValue("/Danmaku/Effect", 5) / 2;
	const QSize & size = lApp->findObject<ARender>()->getActualSize();
	font = GraphicPrivate::getFont(comment.font * GraphicPrivate::getScale(comment.mode, comment.date, size));
	frame = comment.isLocal();
	text = comment.string;
}

void ASprite::setRect(QRectF rect)
{
	const QSize &size = getSize();
	transform.reset();
	transform.scale(rect.width() / size.width(), rect.height() / size.height());
	transform.translate(rect.x(), rect.y());
}
