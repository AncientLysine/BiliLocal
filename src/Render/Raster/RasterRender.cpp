#include "Common.h"
#include "RasterRender.h"
#include "RasterRenderPrivate.h"
#include "AsyncRasterSprite.h"
#include "../ABuffer.h"
#include "../PFormat.h"
#include "../../Config.h"
#include "../../Local.h"
#include "../../Player/APlayer.h"
#include "../../UI/Interface.h"

RasterRender::RasterRender(QObject *parent)
	: ARender(parent)
{
	setObjectName("RRender");
}

namespace
{
	class Widget :public QWidget
	{
	public:
		explicit Widget(RasterRenderPrivate *render)
			: QWidget(lApp->findObject<Interface>()->widget())
			, render(render)
		{
			setAttribute(Qt::WA_TransparentForMouseEvents);
			lower();
		}

	private:
		RasterRenderPrivate *const render;

		virtual void paintEvent(QPaintEvent *) override
		{
			QPainter painter(this);
			QRect rect(QPoint(0, 0), lApp->findObject<ARender>()->getActualSize());
			painter.setRenderHints(QPainter::SmoothPixmapTransform);
			if (lApp->findObject<APlayer>()->getState() == APlayer::Stop) {
				render->drawStop(&painter, rect);
			}
			else {
				render->drawPlay(&painter, rect);
				render->timer.swap();
			}
		}
	};
}

void RasterRender::setup()
{
	d_ptr = new RasterRenderPrivate();

	ARender::setup();

	Q_D(RasterRender);
	d->power = new QTimer(this);
	d->power->setTimerType(Qt::PreciseTimer);
	int fps = Config::getValue("/Render/Option/Raster/Update", 100);
	d->power->start(1000 / (fps > 0 ? fps : 60));
	connect(lApp->findObject<APlayer>(), &APlayer::decode, d->power, [=](){
		if (!d->power->isActive()){
			draw();
		}
	});
	connect(d->power, &QTimer::timeout, lApp->findObject<APlayer>(), [=](){
		if (lApp->findObject<APlayer>()->getState() == APlayer::Play){
			draw();
		}
	});
	d->widget = new Widget(d);
}

RasterRenderPrivate::~RasterRenderPrivate()
{
	sws_freeContext(swsctx);
	if (data) {
		data->release();
	}
}

void RasterRender::setFormat(PFormat *format)
{
	Q_D(RasterRender);
	d->setFormat(format);
}

void RasterRender::setBuffer(ABuffer *buffer)
{
	Q_D(RasterRender);
	d->setBuffer(buffer);
}

ASprite *RasterRender::getSprite()
{
	return new AsyncRasterSprite();
}

QObject *RasterRender::getHandle()
{
	Q_D(RasterRender);
	return d->widget;
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
	return d->inner;
}

void RasterRender::draw(QRect rect)
{
	Q_D(RasterRender);
	d->widget->update(rect.isValid() ? rect : QRect(QPoint(0, 0), getActualSize()));
}

RasterRenderPrivate::RasterRenderPrivate()
{
	data = nullptr;
	format = AV_PIX_FMT_NONE;
	swsctx = nullptr;
}

void RasterRenderPrivate::drawData(QPainter *painter, QRect rect)
{
	painter->fillRect(rect, Qt::black);
	if (data == nullptr || format == AV_PIX_FMT_NONE) {
		return;
	}
	const QRect &dstRect = fitRect(lApp->findObject<ARender>()->getPreferSize(), rect);
	const QSize &dstSize = dstRect.size() * painter->device()->devicePixelRatio();
	if (dstSize != frame.size()) {
		frame = QImage(dstSize, QImage::Format_RGB32);
		dirty = true;
	}
	if (dirty) {
		const QSize &srcSize = inner;
		swsctx = sws_getCachedContext(swsctx,
			srcSize.width(), srcSize.height(), format,
			dstSize.width(), dstSize.height(), AV_PIX_FMT_RGB32,
			SWS_FAST_BILINEAR, NULL, NULL, NULL);
		dataLock.lock();
		data->map();
		const uchar *b = data->bits();
		const QList<QSize> &alloc = data->size();
		const uchar *srcSlice[4] = { nullptr };
		int srcWidth[4] = { 0 };
		for (int i = 0; i < alloc.size() && i < 4; ++i) {
			int w = alloc[i].width(), h = alloc[i].height();
			srcSlice[i] = b;
			srcWidth[i] = w;
			b += w * h;
		}
		uchar *dstSlice[1] = { frame.bits() };
		int dstWidth[1] = { frame.bytesPerLine() };
		sws_scale(swsctx,
			srcSlice, srcWidth,
			0, srcSize.height(),
			dstSlice, dstWidth);
		data->unmap();
		dirty = false;
		dataLock.unlock();
	}
	painter->drawImage(dstRect, frame);
}

void RasterRenderPrivate::drawDanm(QPainter *painter, QRect rect)
{
	painter->save();
	ARenderPrivate::drawDanm(painter, rect);
	painter->restore();
}

namespace
{
	AVPixelFormat getFormat(QString &chroma)
	{
		static QHash<QString, AVPixelFormat> f;
		if (f.isEmpty()) {
			f.insert("RV24", AV_PIX_FMT_BGR24);
			f.insert("RV32", AV_PIX_FMT_RGB32);
			f.insert("RGBA", AV_PIX_FMT_RGBA);
			f.insert("BGRA", AV_PIX_FMT_BGRA);
			f.insert("ARGB", AV_PIX_FMT_ARGB);
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
		if (f.contains(chroma)) {
		}
		else if (chroma == "YV12") {
			chroma = "I420";
		}
		else if (chroma == "NV16") {
			chroma = "NV12";
		}
		else if (chroma == "NV61") {
			chroma = "NV21";
		}
		else if (chroma == "VYUY" || chroma == "YVYU" || chroma == "V422" || chroma == "CYUV") {
			chroma = "UYVY";
		}
		else if (chroma == "V210") {
			chroma = "I0AL";
		}
		else if (chroma == "RGB8" || chroma == "RV12" || chroma == "RV15" || chroma == "RV16") {
			chroma = "RV32";
		}
		else {
			chroma = "I420";
		}
		return f[chroma];
	}
}

void RasterRenderPrivate::setFormat(PFormat *f)
{
	dataLock.lock();
	if (data) {
		data->release();
	}
	data = nullptr;
	dataLock.unlock();

	inner = f->size;

	format = getFormat(f->chroma);

	f->plane.clear();
	int i;
	int w = f->size.width(), h = f->size.height();
	const AVPixFmtDescriptor *desc = av_pix_fmt_desc_get(format);
	if (!desc) {
		return;
	}
	int linesizes[4];
	if (av_image_fill_linesizes(linesizes, format, w) < 0) {
		return;
	}
	uchar *pointers[4];
	int total = av_image_fill_pointers(pointers, format, h, NULL, linesizes);
	if (total < 0) {
		return;
	}
	for (i = 0; i < 3; ++i) {
		if (pointers[i + 1] == nullptr) {
			break;
		}
		int plane = pointers[i + 1] - pointers[i];
		if (plane == 0) {
			continue;
		}
		int width = linesizes[i];
		f->plane.append(QSize(width, plane / width));
		total -= plane;
	}
	{
		int width = linesizes[i];
		f->plane.append(QSize(width, total / width));
	}
}

void RasterRenderPrivate::setBuffer(ABuffer *buffer)
{
	QMutexLocker locker(&dataLock);
	dirty = true;
	if (data) {
		data->release();
	}
	data = buffer;
}
