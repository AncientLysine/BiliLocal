#include "Mode5.h"
#include "GraphicPrivate.h"
#include "../Config.h"

using namespace GraphicPrivate;

Mode5::Mode5(const Comment &comment) :
Plain(comment)
{
	Q_ASSERT(comment.mode == 5);
	life = evaluate(Config::getValue<QString>("/Danmaku/Life", "5"));
}

QList<QRectF> Mode5::locate()
{
	QList<QRectF> results;
	if (rect.height() > 360){
		return results;
	}
	QSize size = ARender::instance()->getActualSize();
	QRectF init = rect;
	init.moveCenter(QPointF(size.width() / 2.0, 0));
	int end = size.height()*(Config::getValue("/Danmaku/Protect", false) ? 0.85 : 1) - rect.height();
	int stp = Config::getValue("/Danmaku/Grating", 10);
	for (int height = 0; height <= end; height += stp){
		init.moveTop(height);
		results.append(init);
	}
	return results;
}

bool Mode5::move(double time)
{
	if (enabled){
		life -= time;
	}
	return life > 0;
}

uint Mode5::intersects(Graphic *other)
{
	Q_ASSERT(other->getMode() == 5);
	const Mode5 &f = *this;
	const Mode5 &s = *static_cast<Mode5 *>(other);
	return getOverlap(f.rect.top(), f.rect.bottom(), s.rect.top(), s.rect.bottom())*qMin(f.rect.width(), s.rect.width());
}