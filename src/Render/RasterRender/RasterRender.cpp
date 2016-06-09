#include "Common.h"
#include "RasterRender.h"
#include "RasterRenderPrivate.h"
#include "AsyncRasterSprite.h"
#include "../../Config.h"
#include "../../Local.h"
#include "../../Player/APlayer.h"
#include "../../UI/Interface.h"

RasterRender::RasterRender(QObject *parent)
	: ARender(parent)
{
	setObjectName("RRender");
}

void RasterRender::setup()
{
	d_ptr = new RasterRenderPrivate();

	ARender::setup();

	Q_D(RasterRender);
	d->power = new QTimer(this);
	d->power->setTimerType(Qt::PreciseTimer);
	int fps = Config::getValue("/Performance/Option/Raster/Update", 100);
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
	d->widget = new RasterRenderPrivate::Widget(d);
}

ASprite *RasterRender::getSprite()
{
	return new AsyncRasterSprite();
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

RasterRenderPrivate::Buffer::Buffer(AVPixelFormat format, QSize size, int alignment)
	: format(AV_PIX_FMT_NONE), size(size)
{
	int len = av_image_alloc(data, width, size.width(), size.height(), format, alignment);
	if (len < 0 || data[0] == nullptr) {
		return;
	}
	int i = 0;
	for (; i < 3; ++i) {
		quint8 *ptr = data[i + 1];
		if (ptr == nullptr) {
			break;
		}
		int num = ptr - data[i];
		if (num == 0) {
			continue;
		}
		lines[i] = num / width[i];
		len -= num;
	}
	lines[i] = len / width[i];
	++i;
	for (; i < 4; ++i) {
		lines[i] = 0;
	}
	this->format = format;
}

RasterRenderPrivate::Buffer::~Buffer()
{
	if (isValid()) {
		av_freep(&data[0]);
	}
}

bool RasterRenderPrivate::Buffer::isValid()
{
	return format != AV_PIX_FMT_NONE;
}

RasterRenderPrivate::Widget::Widget(RasterRenderPrivate *render)
	: QWidget(lApp->findObject<Interface>()->widget())
	, render(render)
{
	setAttribute(Qt::WA_TransparentForMouseEvents);
	lower();
}

void RasterRenderPrivate::Widget::paintEvent(QPaintEvent *)
{
	QPainter painter(this);
	QRect rect(QPoint(0, 0), lApp->findObject<ARender>()->getActualSize());
	painter.setRenderHints(QPainter::SmoothPixmapTransform);
	if (lApp->findObject<APlayer>()->getState() == APlayer::Stop){
		render->drawStop(&painter, rect);
	}
	else{
		render->drawPlay(&painter, rect);
		render->timer.swap();
	}
}

RasterRenderPrivate::RasterRenderPrivate()
{
	swsctx = nullptr;
	srcFrame = nullptr;
	dstFrame = nullptr;
}

AVPixelFormat RasterRenderPrivate::getFormat(QString &chroma)
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

void RasterRenderPrivate::drawData(QPainter *painter, QRect rect)
{
	if (!srcFrame->isValid()){
		return;
	}
	QRect dest = fitRect(lApp->findObject<ARender>()->getPreferSize(), rect);
	QSize dstSize = dest.size()*painter->device()->devicePixelRatio();
	if (!dstFrame || dstFrame->size != dstSize){
		delete dstFrame;
		dstFrame = new Buffer(AV_PIX_FMT_RGB32, dstSize, 4);
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

void RasterRenderPrivate::drawDanm(QPainter *painter, QRect rect)
{
	painter->save();
	ARenderPrivate::drawDanm(painter, rect);
	painter->restore();
}

QList<quint8 *> RasterRenderPrivate::getBuffer()
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

void RasterRenderPrivate::setBuffer(QString &chroma, QSize size, int alignment, QList<QSize> *bufferSize)
{
	delete srcFrame;
	srcFrame = new Buffer(getFormat(chroma), size, alignment);
	if (bufferSize){
		bufferSize->clear();
	}
	for (int i = 0; i < 4; ++i){
		int l = srcFrame->width[i];
		if (l == 0){
			break;
		}
		if (bufferSize){
			bufferSize->append(QSize(srcFrame->width[i], srcFrame->lines[i]));
		}
	}
}

void RasterRenderPrivate::releaseBuffer()
{
	dirty = true;
	dataLock.unlock();
}

RasterRenderPrivate::~RasterRenderPrivate()
{
	sws_freeContext(swsctx);
	delete srcFrame;
	delete dstFrame;
}
