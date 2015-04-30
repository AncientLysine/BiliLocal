#include "RasterRender.h"
#include "ARenderPrivate.h"
#include "AsyncRasterCache.h"
#include "../Config.h"
#include "../Local.h"
#include "../Player/APlayer.h"
#include <QWidget>

extern "C"
{
#include <libavutil/mem.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

namespace
{
	class RWidget;
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

		Buffer(AVPixelFormat format, QSize size) :
			format(AV_PIX_FMT_NONE), size(size)
		{
			memset(data, 0, sizeof(data[0]) * 4);
			if (av_image_fill_linesizes(width, format, size.width()) < 0)
				return;
			const AVPixFmtDescriptor *desc = av_pix_fmt_desc_get(format);
			if (!desc || desc->flags&PIX_FMT_HWACCEL)
				return;
			int i, p[4] = { 0 };
			for (i = 0; i < 4; i++)
				p[desc->comp[i].plane] = 1;
			lines[0] = size.height();
			int n = width[0] * lines[0];
			for (i = 1; i < 4 && p[i]; i++){
				int s = (i == 1 || i == 2) ? desc->log2_chroma_h : 0;
				lines[i] = (size.height() + (1 << s) - 1) >> s;
				n += width[i] * lines[i];
			}
			data[0] = new quint8[n];
			for (i = 1; i < 4 && p[i]; i++){
				data[i] = data[i - 1] + width[i - 1] * lines[i - 1];
			}
			this->format = format;
		}

		bool isValid()
		{
			return format != AV_PIX_FMT_NONE;
		}

		~Buffer()
		{
			if (isValid()){
				delete data[0];
			}
		}
	};

	QTimer *power;
	SwsContext *swsctx;
	Buffer *srcFrame;
	Buffer *dstFrame;
	QImage frame;
	QMutex dataLock;
	RWidget *widget;

	RasterRenderPrivate()
	{
		swsctx = NULL;
		srcFrame = NULL;
		dstFrame = NULL;
	}

	AVPixelFormat getFormat(QString &chroma)
	{
		static QHash<QString, AVPixelFormat> f;
		if (f.isEmpty()){
			f.insert("RV32", AV_PIX_FMT_RGB32);
			f.insert("RV24", AV_PIX_FMT_RGB24);
			f.insert("RGB8", AV_PIX_FMT_RGB8);
			f.insert("RV12", AV_PIX_FMT_RGB444);
			f.insert("RV15", AV_PIX_FMT_RGB555);
			f.insert("RV16", AV_PIX_FMT_RGB565);
			f.insert("RGBA", AV_PIX_FMT_RGBA);
			f.insert("ARGB", AV_PIX_FMT_ARGB);
			f.insert("BGRA", AV_PIX_FMT_BGRA);
			f.insert("I410", AV_PIX_FMT_YUV410P);
			f.insert("I411", AV_PIX_FMT_YUV411P);
			f.insert("I420", AV_PIX_FMT_YUV420P);
			f.insert("IYUV", AV_PIX_FMT_YUV420P);
			f.insert("I422", AV_PIX_FMT_YUV422P);
			f.insert("I440", AV_PIX_FMT_YUV440P);
			f.insert("I444", AV_PIX_FMT_YUV444P);
			f.insert("J420", AV_PIX_FMT_YUVJ420P);
			f.insert("J422", AV_PIX_FMT_YUVJ422P);
			f.insert("J440", AV_PIX_FMT_YUVJ440P);
			f.insert("J444", AV_PIX_FMT_YUVJ444P);
			f.insert("I40A", AV_PIX_FMT_YUVA420P);
			f.insert("I42A", AV_PIX_FMT_YUVA422P);
			f.insert("YUVA", AV_PIX_FMT_YUVA444P);
			f.insert("YA0L", AV_PIX_FMT_YUVA444P10LE);
			f.insert("YA0B", AV_PIX_FMT_YUVA444P10BE);
			f.insert("NV12", AV_PIX_FMT_NV12);
			f.insert("NV21", AV_PIX_FMT_NV21);
			f.insert("I09L", AV_PIX_FMT_YUV420P9LE);
			f.insert("I09B", AV_PIX_FMT_YUV420P9BE);
			f.insert("I29L", AV_PIX_FMT_YUV422P9LE);
			f.insert("I29B", AV_PIX_FMT_YUV422P9BE);
			f.insert("I49L", AV_PIX_FMT_YUV444P9LE);
			f.insert("I49B", AV_PIX_FMT_YUV444P9BE);
			f.insert("I0AL", AV_PIX_FMT_YUV420P10LE);
			f.insert("I0AB", AV_PIX_FMT_YUV420P10BE);
			f.insert("I2AL", AV_PIX_FMT_YUV422P10LE);
			f.insert("I2AB", AV_PIX_FMT_YUV422P10BE);
			f.insert("I4AL", AV_PIX_FMT_YUV444P10LE);
			f.insert("I4AB", AV_PIX_FMT_YUV444P10BE);
			f.insert("UYVY", AV_PIX_FMT_UYVY422);
			f.insert("YUYV", AV_PIX_FMT_YUYV422);
			f.insert("YUY2", AV_PIX_FMT_YUYV422);
		}
		chroma = chroma.toUpper();
		if (f.contains(chroma)){
		}
		else if (chroma == "YV12"){
			chroma = "I420";
		}
		else if (chroma == "NV16"){
			chroma = "NV12";
		}
		else if (chroma == "NV61"){
			chroma = "NV21";
		}
		else if (chroma == "VYUY" ||
			chroma == "YVYU" ||
			chroma == "V422" ||
			chroma == "CYUV"){
			chroma = "UYVY";
		}
		else if (chroma == "V210"){
			chroma = "I0AL";
		}
		else{
			chroma = "I420";
		}
		return f[chroma];
	}

	void drawData(QPainter *painter, QRect rect)
	{
		if (!srcFrame->isValid()){
			return;
		}
		QRect dest = fitRect(ARender::instance()->getPreferSize(), rect);
		QSize dstSize = dest.size()*painter->device()->devicePixelRatio();
		if (!dstFrame || dstFrame->size != dstSize){
			if (dstFrame){
				delete dstFrame;
			}
			dstFrame = new Buffer(AV_PIX_FMT_RGB32, dstSize);
			frame = QImage(*dstFrame->data, dstSize.width(), dstSize.height(), QImage::Format_RGB32);
			dirty = true;
		}
		if (dirty){
			swsctx = sws_getCachedContext(swsctx,
				srcFrame->size.width(), srcFrame->size.height(), srcFrame->format,
				dstFrame->size.width(), dstFrame->size.height(), dstFrame->format,
				SWS_FAST_BILINEAR, NULL, NULL, NULL);
			dataLock.lock();
			sws_scale(swsctx,
				srcFrame->data, srcFrame->width,
				0, srcFrame->size.height(),
				dstFrame->data, dstFrame->width);
			dirty = false;
			dataLock.unlock();
		}
		painter->drawImage(dest, frame);
	}

	QList<quint8 *> getBuffer()
	{
		dataLock.lock();
		QList<quint8 *> p;
		for (int i = 0; i < 4; ++i){
			if (srcFrame->width[i] == 0){
				break;
			}
			p.append(srcFrame->data[i]);
		}
		return p;
	}

	void setBuffer(QString &chroma, QSize size, QList<QSize> *bufferSize)
	{
		if (srcFrame){
			delete srcFrame;
		}
		srcFrame = new Buffer(getFormat(chroma), size);
		if (bufferSize){
			bufferSize->clear();
		}
		for (int i = 0; i < 4; ++i){
			int l;
			if ((l = srcFrame->width[i]) == 0){
				break;
			}
			if (bufferSize){
				bufferSize->append(QSize(srcFrame->width[i], srcFrame->lines[i]));
			}
		}
	}

	void releaseBuffer()
	{
		dirty = true;
		dataLock.unlock();
	}

	virtual ~RasterRenderPrivate()
	{
		if (swsctx){
			sws_freeContext(swsctx);
		}
		if (srcFrame){
			delete srcFrame;
		}
		if (dstFrame){
			delete dstFrame;
		}
	}
};

namespace
{
	class RWidget :public QWidget
	{
	public:
		RWidget(RasterRenderPrivate *render) :
			QWidget(lApp->mainWidget()), render(render)
		{
			setAttribute(Qt::WA_TransparentForMouseEvents);
			lower();
		}

	private:
		RasterRenderPrivate *const render;

		void paintEvent(QPaintEvent *e)
		{
			QPainter painter(this);
			QRect rect(QPoint(0, 0), ARender::instance()->getActualSize());
			painter.setRenderHints(QPainter::SmoothPixmapTransform);
			if (APlayer::instance()->getState() == APlayer::Stop){
				render->drawStop(&painter, rect);
			}
			else{
				render->drawPlay(&painter, rect);
				render->drawTime(&painter, rect);
			}
			QWidget::paintEvent(e);
		}
	};
}

RasterRender::RasterRender(QObject *parent) :
ARender(new RasterRenderPrivate, parent)
{
	Q_D(RasterRender);
	ins = this;
	setObjectName("RRender");
	d->power = new QTimer(this);
	d->power->setTimerType(Qt::PreciseTimer);
	int fps = Config::getValue("/Performance/Raster/FPS", 60);
	d->power->start(1000 / (fps > 0 ? fps : 60));
	connect(APlayer::instance(), &APlayer::decode, d->power, [=](){
		if (!d->power->isActive()){
			draw();
		}
	});
	connect(d->power, &QTimer::timeout, APlayer::instance(), [=](){
		if (APlayer::instance()->getState() == APlayer::Play){
			draw();
		}
	});
	d->widget = new RWidget(d);
}

ARender::ICache *RasterRender::getCache(const QImage &i)
{
	return new AsyncRasterCache(i);
}

quintptr RasterRender::getHandle()
{
	Q_D(RasterRender);
	return (quintptr)d->widget;
}

void RasterRender::resize(QSize size)
{
	Q_D(RasterRender);
	d->widget->resize(size);
}

QSize RasterRender::getActualSize()
{
	Q_D(RasterRender);
	return d->widget->size();
}

QSize RasterRender::getBufferSize()
{
	Q_D(RasterRender);
	return d->srcFrame->size;
}

void RasterRender::draw(QRect rect)
{
	Q_D(RasterRender);
	d->widget->update(rect.isValid() ? rect : QRect(QPoint(0, 0), getActualSize()));
}
