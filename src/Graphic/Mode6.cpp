#include "Mode6.h"
#include "GraphicPrivate.h"
#include "../Config.h"

using namespace GraphicPrivate;

Mode6::Mode6(const Comment &comment) :
Plain(comment)
{
	Q_ASSERT(comment.mode == 6);
	QString expression = Config::getValue<QString>("/Danmaku/Speed", "125+%{width}/5");
	expression.replace("%{width}", QString::number(rect.width()), Qt::CaseInsensitive);
	speed = Utils::evaluate(expression);
}

QList<QRectF> Mode6::locate()
{
	QList<QRectF> results;
	if (rect.height() > 360){
		return results;
	}
	QSize size = ARender::instance()->getActualSize();
	QRectF init = rect;
	init.moveRight(0);
	int end = size.height()*(Config::getValue("/Danmaku/Protect", false) ? 0.85 : 1) - rect.height();
	int stp = Config::getValue("/Danmaku/Grating", 10);
	for (int height = 0; height <= end; height += stp){
		init.moveTop(height);
		results.append(init);
	}
	return results;
}

bool Mode6::move(qint64 time)
{
	QSize size = ARender::instance()->getActualSize();
	if (enabled){
		rect.moveLeft(rect.left() + speed*time / 1000.0);
	}
	return rect.left() <= size.width();
}

uint Mode6::intersects(Graphic *other)
{
	Q_ASSERT(other->getMode() == 6);
	const Mode6 &f = *static_cast<Mode6 *>(other);
	const Mode6 &s = *this;
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
		double o = f.rect.left() - f.speed*s.rect.right() / s.speed;
		w = o > 0 ? qMin(qMin(f.rect.width(), s.rect.width()), o) : 0;
	}
	return h*w;
}
