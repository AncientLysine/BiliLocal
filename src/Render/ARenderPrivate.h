#pragma once

#include <QtGui>
#include <QtCore>
#include "ElapsedTimer.h"

class ABuffer;
class PFormat;

class ARenderPrivate
{
public:
	QMovie tv;
	double time;
	QImage me, background, sound;
	QSize pref;
	bool music;
	bool dirty;
	double videoAspectRatio;
	double pixelAspectRatio;
	ElapsedTimer timer;

	virtual ~ARenderPrivate() = default;
	QRect fitRect(QSize size, QRect rect);
	void drawPlay(QPainter *painter, QRect rect);
	void drawStop(QPainter *painter, QRect rect);
	void drawTime(QPainter *painter, QRect rect);
	virtual void drawDanm(QPainter *painter, QRect rect);
	virtual void drawData(QPainter *painter, QRect rect) = 0;
	virtual void clear(QPainter *painter, QColor color) = 0;
};
