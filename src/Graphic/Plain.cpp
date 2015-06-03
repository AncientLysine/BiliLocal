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
	spirit = ARender::instance()->getSpirit(image);
}

Plain::~Plain()
{
	delete spirit;
}

void Plain::draw(QPainter *painter)
{
	if (enabled){
		spirit->draw(painter, rect);
	}
}

double Plain::evaluate(QString expression)
{
	expression.replace("%{width}", QString::number(rect.width()), Qt::CaseInsensitive);
	try{
		return Utils::evaluate(expression);
	}
	catch (std::runtime_error){
		throw args_prasing_error();
	}
}
