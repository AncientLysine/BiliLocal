#include "Common.h"
#include "Mode7.h"
#include "GraphicPrivate.h"
#include "../Config.h"
#include "../Render/ARender.h"

using namespace GraphicPrivate;

Mode7::Mode7(const Comment &comment)
{
	Q_ASSERT(comment.mode == 7);
	QJsonArray data = QJsonDocument::fromJson(comment.string.toUtf8()).array();
	int l = data.size();
	if (l < 5){
		throw format_unrecognized();
	}
	QSize size = ARender::instance()->getActualSize();
	auto getDouble = [&data](int i){return data.at(i).toVariant().toDouble(); };
	double scale = getScale(comment.mode, comment.date, size);
	bPos = QPointF(getDouble(0), getDouble(1));
	ePos = l < 8 ? bPos : QPointF(getDouble(7), getDouble(8));
	int w = size.width(), h = size.height();
	if (bPos.x() < 1 && bPos.y() < 1 && ePos.x() < 1 && ePos.y() < 1){
		bPos.rx() *= w;
		ePos.rx() *= w;
		bPos.ry() *= h;
		ePos.ry() *= h;
		scale = 1;
	}
	else if (scale == 0){
		scale = 1;
	}
	else{
		QSizeF player = getPlayer(comment.date);
		QPoint offset = QPoint((w - player.width()*scale) / 2, (h - player.height()*scale) / 2);
		bPos = bPos*scale + offset;
		ePos = ePos*scale + offset;
	}
	QStringList alpha = data[2].toString().split('-');
	bAlpha = alpha[0].toDouble();
	eAlpha = alpha[1].toDouble();
	life = getDouble(3);
	QJsonValue v = l < 12 ? QJsonValue(true) : data[11];
	int effect = (v.isString() ? v.toString() == "true" : v.toVariant().toBool()) ? Config::getValue("/Danmaku/Effect", 5) / 2 : -1;
	QFont font = getFont(scale ? comment.font*scale : comment.font, l < 13 ? Utils::defaultFont(true) : data[12].toString());
	QString string = data[4].toString();
	//TODO: using ARender::getSprite
	cache = getCache(string, comment.color, font, getSize(string, font), comment.isLocal(), effect, 100);
	zRotate = l < 6 ? 0 : getDouble(5);
	yRotate = l < 7 ? 0 : getDouble(6);
	wait = l < 11 ? 0 : getDouble(10) / 1000;
	stay = l < 10 ? 0 : life - wait - getDouble(9) / 1000;
	source = &comment;
	time = 0;
}

bool Mode7::move(double time)
{
	if (enabled){
		this->time += time;
	}
	return (this->time) <= life;
}

void Mode7::draw(QPainter *painter)
{
	if (enabled){
		QPointF cPos = bPos + (ePos - bPos)*qBound<double>(0, (time - wait) / (life - stay), 1);
		QTransform rotate;
		rotate.translate(+cPos.x(), +cPos.y());
		rotate.rotate(yRotate, Qt::YAxis);
		rotate.rotate(zRotate, Qt::ZAxis);
		rotate.translate(-cPos.x(), -cPos.y());
		painter->save();
		painter->setTransform(rotate);
		painter->setOpacity(bAlpha + (eAlpha - bAlpha)*time / life);
		painter->drawImage(cPos, cache);
		painter->restore();
	}
}
