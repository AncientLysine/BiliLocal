#pragma once

#include <QtGui>
#include <QtCore>
#include "ElapsedTimer.h"

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
	void drawDanm(QPainter *painter, QRect rect);
	void drawTime(QPainter *painter, QRect rect);
	virtual void drawData(QPainter *painter, QRect rect) = 0;
	virtual QList<quint8 *> getBuffer() = 0;
	virtual void releaseBuffer() = 0;
	virtual void setBuffer(QString &chroma, QSize size, QList<QSize> *bufferSize = 0) = 0;

};
