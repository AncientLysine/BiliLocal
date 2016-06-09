#include "Common.h"
#include "Mode4.h"
#include "GraphicPrivate.h"
#include "../Config.h"
#include "../Local.h"

using namespace GraphicPrivate;

Mode4::Mode4(const Comment &comment) :
Plain(comment)
{
	Q_ASSERT(comment.mode == 4);
	life = evaluate(Config::getValue<QString>("/Danmaku/Life", "5"));
}

QList<QRectF> Mode4::locate()
{
	QList<QRectF> results;
	if (rect.height() > 360){
		return results;
	}
	QSize size = lApp->findObject<ARender>()->getActualSize();
	QRectF init = rect;
	init.moveCenter(QPointF(size.width() / 2.0, 0));
	init.moveBottom(size.height()*(Config::getValue("/Danmaku/Protect", false) ? 0.85 : 1));
	int stp = Config::getValue("/Danmaku/Grating", 10);
	for (int height = init.top(); height >= 0; height -= stp){
		init.moveTop(height);
		results.append(init);
	}
	return results;
}

bool Mode4::move(double time)
{
	if (enabled){
		life -= time;
	}
	return life > 0;
}

uint Mode4::intersects(Graphic *other)
{
	Q_ASSERT(other->getMode() == 4);
	const Mode4 &f = *this;
	const Mode4 &s = *static_cast<Mode4 *>(other);
	return getOverlap(f.rect.top(), f.rect.bottom(), s.rect.top(), s.rect.bottom())*qMin(f.rect.width(), s.rect.width());
}
