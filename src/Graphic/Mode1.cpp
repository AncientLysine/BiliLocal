#include "Mode1.h"
#include "GraphicPrivate.h"
#include "../Config.h"

using namespace GraphicPrivate;

Mode1::Mode1(const Comment &comment) :
Plain(comment)
{
	Q_ASSERT(comment.mode == 1);
	speed = evaluate(Config::getValue<QString>("/Danmaku/Speed", "125+%{width}/5"));
}

QList<QRectF> Mode1::locate()
{
	QList<QRectF> results;
	if (rect.height() > 360){
		return results;
	}
	QSize size = ARender::instance()->getActualSize();
	QRectF init = rect;
	init.moveLeft(size.width());
	int end = size.height()*(Config::getValue("/Danmaku/Protect", false) ? 0.85 : 1) - rect.height();
	int stp = Config::getValue("/Danmaku/Grating", 10);
	for (int height = 0; height <= end; height += stp){
		init.moveTop(height);
		results.append(init);
	}
	return results;
}

bool Mode1::move(double time)
{
	if (enabled){
		rect.moveLeft(rect.left() - speed * time);
	}
	return rect.right() >= 0;
}

uint Mode1::intersects(Graphic *other)
{
	Q_ASSERT(other->getMode() == 1);
	const Mode1 &f = *static_cast<Mode1 *>(other);
	const Mode1 &s = *this;
	int h;
	if ((h = getOverlap(f.rect.top(), f.rect.bottom(), s.rect.top(), s.rect.bottom())) == 0){
		return 0;
	}
	int w = 0;
	if (f.rect.intersects(s.rect)){
		if (f.speed > s.speed){
			w = getOverlap(f.rect.left(), f.rect.right(), s.rect.left(), s.rect.right());
		}
		else{
			w = qMin(f.rect.width(), s.rect.width());
		}
	}
	else{
		double o = f.rect.right() - f.speed*s.rect.left() / s.speed;
		w = o > 0 ? qMin(qMin(f.rect.width(), s.rect.width()), o) : 0;
	}
	return h*w;
}
