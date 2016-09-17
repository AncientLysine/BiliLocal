#include "Common.h"
#include "ASprite.h"
#include "ARender.h"
#include "../Define/Comment.h"
#include "../Graphic/GraphicPrivate.h"
#include "../Local.h"
#include "../Utility/Sample.h"
#include "../Utility/Text.h"

void ASprite::setAuto(const Comment & comment)
{
	color = QColor::fromRgb(comment.color);
	color.setAlphaF(Config::getValue("/Danmaku/Alpha", 100) / 100.0);
	effect = Config::getValue("/Danmaku/Effect", 5) / 2;
	const QSize & size = lApp->findObject<ARender>()->getActualSize();
	font = GraphicPrivate::getFont(comment.font * GraphicPrivate::getScale(comment.mode, comment.date, size));
	frame = comment.isLocal();
	text = Utils::decodeTxt(QString(comment.string));
}

void ASprite::setPosition(QPointF position)
{
	Sample s("ASprite::setPosition");

	transform.reset();
	transform.translate(position.x(), position.y());
}
