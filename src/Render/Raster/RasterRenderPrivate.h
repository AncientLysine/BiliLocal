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
	QList<QSize> plane;
	QMutex dataLock;
	SwsContext *swsctx;
	QImage frame;
	QWidget *widget;

	RasterRenderPrivate();
	virtual ~RasterRenderPrivate();
	virtual void drawData(QPainter *painter, QRect rect) override;
	virtual void drawDanm(QPainter *painter, QRect rect) override;
	virtual void setFormat(PFormat *format) override;
	virtual void setBuffer(ABuffer *buffer) override;
};
