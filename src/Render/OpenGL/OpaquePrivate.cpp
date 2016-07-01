#include "Common.h"
#include "OpaquePrivate.h"
#include "../ABuffer.h"
#include "../PFormat.h"

OpenGLOpaqueRenderPrivate::~OpenGLOpaqueRenderPrivate()
{
	glDeleteTextures(3, frame);
	if (data) {
		data->release();
	}
}

void OpenGLOpaqueRenderPrivate::loadTexture(int index, const uchar *data, int channel, int width, int height)
{
	int format;
	switch (channel) {
	case 1:
		format = GL_LUMINANCE;
		break;
	case 2:
		format = GL_LUMINANCE_ALPHA;
		break;
	case 3:
		format = GL_RGB;
		break;
	case 4:
		format = GL_RGBA;
		break;
	default:
		return;
	}
	glBindTexture(GL_TEXTURE_2D, frame[index]);
	glPixelStorei(GL_UNPACK_ALIGNMENT, alignment);
	glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

void OpenGLOpaqueRenderPrivate::paint(QPaintDevice * device)
{
	QPainter painter(device);
	painter.setRenderHints(QPainter::SmoothPixmapTransform);
	QRect rect(QPoint(0, 0), lApp->findObject<ARender>()->getActualSize());
	if (lApp->findObject<APlayer>()->getState() == APlayer::Stop) {
		drawStop(&painter, rect);
	}
	else {
		drawPlay(&painter, rect);
	}
}

void OpenGLOpaqueRenderPrivate::initialize()
{
	OpenGLRenderPrivate::initialize();
	data = nullptr;
	glGenTextures(3, frame);
	tex[0] = 0; tex[1] = 0;
	tex[2] = 1; tex[3] = 0;
	tex[4] = 0; tex[5] = 1;
	tex[6] = 1; tex[7] = 1;
}

void OpenGLOpaqueRenderPrivate::drawData(QPainter * painter, QRect rect)
{
	if (inner.isEmpty()) {
		return;
	}
	painter->beginNativePainting();

	QOpenGLShaderProgram *p = nullptr;
	QMutexLocker locker(&dataLock);
	switch(data->handleType()){
	case ABuffer::NoHandle:
		if (dirty && data->map()) {
			const uchar *bits = data->bits();
			int w, h;
			switch (format) {
			case I420:
			case YV12:
				w = plane[0].width(); h = plane[0].height();
				loadTexture(0, bits, 1, w, h);
				bits += w * h;
				w = plane[1].width(); h = plane[1].height();
				loadTexture(1, bits, 1, w, h);
				bits += w * h;
				w = plane[2].width(); h = plane[2].height();
				loadTexture(2, bits, 1, w, h);
				break;
			case NV12:
			case NV21:
				w = plane[0].width(); h = plane[0].height();
				loadTexture(0, bits, 1, w, h);
				bits += w * h;
				w = plane[1].width(); h = plane[1].height();
				w /= 2;
				loadTexture(1, bits, 2, w, h);
				break;
			case RGBA:
			case BGRA:
			case ARGB:
				w = plane[0].width(); h = plane[0].height();
				w /= 4;
				loadTexture(0, bits, 4, w, h);
				break;
			default:
				break;
			}
			data->unmap();
			dirty = false;
		}
		locker.unlock();
		switch (format) {
		case I420:
		case YV12:
			glActiveTexture(GL_TEXTURE2);
			glBindTexture(GL_TEXTURE_2D, frame[2]);
		case NV12:
		case NV21:
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, frame[1]);
		case RGBA:
		case BGRA:
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, frame[0]);
			break;
		default:
			break;
		}
		p = &program[format];
		break;
	case ABuffer::GLTextureHandle:
		switch (format) {
		case RGBA:
		case BGRA:
		case ARGB:
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, data->handle().toUInt());
			break;
		default:
			break;
		}
		p = &program[RGBA];
		break;
	}
	glViewport(rect.x(), rect.y(), rect.width(), rect.height());
	QRect dest = fitRect(lApp->findObject<ARender>()->getPreferSize(), rect);
	GLfloat h = 2.0f / rect.width(), v = 2.0f / rect.height();
	GLfloat l = dest.left() * h - 1, r = dest.right() * h - 1, t = 1 - dest.top() * v, b = 1 - dest.bottom() * v;
	vtx[0] = l; vtx[1] = t;
	vtx[2] = r; vtx[3] = t;
	vtx[4] = l; vtx[5] = b;
	vtx[6] = r; vtx[7] = b;
	p->bind();
	p->setAttributeArray(0, vtx, 2);
	p->setAttributeArray(1, tex, 2);
	p->enableAttributeArray(0);
	p->enableAttributeArray(1);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	painter->endNativePainting();
}

namespace
{
	void align(int &size, int alignment)
	{
		--alignment;
		size = (size + alignment) & (~alignment);
	}
}

void OpenGLOpaqueRenderPrivate::setFormat(PFormat *f)
{
	dataLock.lock();
	if (data) {
		data->release();
	}
	data = nullptr;
	dataLock.unlock();

	if (f->chroma == "YV12") {
		format = YV12;
	}
	else if (f->chroma == "NV12") {
		format = NV12;
	}
	else if (f->chroma == "NV21") {
		format = NV21;
	}
	else if (f->chroma == "RGBA") {
		format = RGBA;
	}
	else if (f->chroma == "BGRA") {
		format = BGRA;
	}
	else if (f->chroma == "ARGB") {
		format = ARGB;
	}
	else {
		format = I420;
		f->chroma = "I420";
	}

	inner = f->size;

	if (alignment >= 8) {
		alignment = 8;
	}
	else if (alignment >= 4) {
		alignment = 4;
	}
	else if (alignment >= 2) {
		alignment = 2;
	}
	else {
		alignment = 1;
	}
	this->alignment = alignment;

	plane.clear();
	QSize alloc = f->size;
	switch (format) {
	case I420:
	case YV12:
		plane.append(alloc);
		alloc /= 2;
		plane.append(alloc);
		plane.append(alloc);
		break;
	case NV12:
	case NV21:
		plane.append(alloc);
		alloc.rheight() /= 2;
		plane.append(alloc);
		break;
	case RGBA:
	case BGRA:
	case ARGB:
		alloc.rwidth() *= 4;
		plane.append(alloc);
		break;
	default:
		return;
	}
	for (QSize &iter : plane) {
		align(iter.rwidth(), alignment);
	}
	f->alloc = plane;
}

void OpenGLOpaqueRenderPrivate::setBuffer(ABuffer *buffer)
{
	QMutexLocker locker(&dataLock);
	dirty = true;
	if (data) {
		data->release();
	}
	data = buffer;
}

QSize OpenGLOpaqueRenderPrivate::getBufferSize()
{
	return inner;
}
