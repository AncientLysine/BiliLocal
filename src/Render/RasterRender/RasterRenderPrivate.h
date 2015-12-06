#pragma once

#include "Common.h"
#include "../ARenderPrivate.h"
#include <QWidget>

extern "C"
{
#include <libavutil/mem.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

class RasterRenderPrivate :public ARenderPrivate
{
public:
	class Buffer
	{
	public:
		quint8 *data[4];
		qint32 width[4];
		qint32 lines[4];
		AVPixelFormat format;
		QSize size;

		Buffer(AVPixelFormat format, QSize size);
		~Buffer();
		bool isValid();
	};


	class Widget :public QWidget
	{
	public:
		explicit Widget(RasterRenderPrivate *render);

	private:
		RasterRenderPrivate *const render;

		virtual void paintEvent(QPaintEvent *e) override;
	};

	QTimer *power;
	SwsContext *swsctx;
	Buffer *srcFrame;
	Buffer *dstFrame;
	Widget *widget;
	QImage frame;
	QMutex dataLock;

	RasterRenderPrivate();
	AVPixelFormat getFormat(QString &chroma);
	virtual void drawData(QPainter *painter, QRect rect) override;
	virtual QList<quint8 *> getBuffer() override;
	virtual void setBuffer(QString &chroma, QSize size, int alignment, QList<QSize> *bufferSize) override;
	virtual void releaseBuffer() override;
	virtual ~RasterRenderPrivate();
};
