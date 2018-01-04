#pragma once

#include "Common.h"
#include "../ARenderPrivate.h"
#include <QWidget>

extern "C"
{
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

class ABuffer;

class RasterRenderPrivate :public ARenderPrivate
{
public:
	QTimer *power;
	ABuffer *data;
	AVPixelFormat format;
	QSize inner;
	QMutex dataLock;
	SwsContext *swsctx;
	QImage frame;
	QWidget *widget;

	RasterRenderPrivate();
	virtual ~RasterRenderPrivate();
	virtual void drawData(QPainter *painter, QRect rect) override;
	virtual void drawDanm(QPainter *painter, QRect rect) override;
	virtual void clear(QPainter *painter, QColor color) override;
	void setFormat(PFormat *format);
	void setBuffer(ABuffer *buffer);
};
