#pragma once

#include <QColor>
#include <QFont>
#include <QPainter>
#include <QRectF>
#include <QSize>
#include <QString>
#include <QTransform>

class Comment;

class ASprite
{
public:
	virtual void prepare() = 0;
	virtual void draw(QPainter *) = 0;
	virtual QSize getSize() = 0;
	virtual ~ASprite() = default;

	void setAuto(const Comment &comment);

	void setColor(QColor color)
	{
		this->color = color;
	}

	void setAlpha(double alpha)
	{
		this->color.setAlphaF(alpha);
	}

	void setEffect(int effect)
	{
		this->effect = effect;
	}

	void setFont(QFont font)
	{
		this->font = font;
	}

	void setFrame(bool frame)
	{
		this->frame = frame;
	}

	void setText(QString text)
	{
		this->text = text;
	}

	void setRect(QRectF rect);

	void setTransform(QTransform transform)
	{
		this->transform = transform;
	}

protected:
	QColor color;
	int effect;
	QFont font;
	bool frame;
	QString text;

	QTransform transform;
};
