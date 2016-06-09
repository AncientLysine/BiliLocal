#include "Common.h"
#include "Plain.h"
#include "GraphicPrivate.h"
#include "../Local.h"

using namespace GraphicPrivate;

Plain::Plain(const Comment &comment)
{
	source = &comment;
	sprite = lApp->findObject<ARender>()->getSprite();
	sprite->setAuto(comment);
	sprite->prepare();
	rect.setSize(sprite->getSize());
}

Plain::~Plain()
{
	delete sprite;
}

void Plain::draw(QPainter *painter)
{
	if (enabled){
		sprite->setRect(rect);
		sprite->draw(painter);
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
