#include "Common.h"
#include "OpaquePrivate.h"
#include "../ABuffer.h"
#include "../PFormat.h"
#include "../../Utility/Sample.h"

OpenGLOpaqueRenderPrivate::~OpenGLOpaqueRenderPrivate()
{
	glDeleteTextures(3, frame);
	if (data) {
		data->release();
	}
}

void OpenGLOpaqueRenderPrivate::loadFrame(int index, const uchar *data, int channel, int width, int height)
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
	int align;
	if (width % 8 == 0){
		align = 8;
	}
	else if (width % 4 == 0){
		align = 4;
	}
	else if (width % 2 == 0) {
		align = 2;
	}
	else {
		align = 1;
	}
	glBindTexture(GL_TEXTURE_2D, frame[index]);
	glPixelStorei(GL_UNPACK_ALIGNMENT, align);
	glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

QVector2D OpenGLOpaqueRenderPrivate::validArea(int index)
{
	const QSize &p = plane[index];
	const QSize &a = alloc[index];
	float x = p.width() / (float)a.width(), y = p.height() / (float)a.height();
	return QVector2D(x, y);
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
	Sample s("OpaqueRender::drawData");

	painter->beginNativePainting();

	QOpenGLShaderProgram *p = nullptr;
	QMutexLocker locker(&dataLock);
	switch(data->handleType()){
	case ABuffer::NoHandle:
		if (dirty && data->map()) {
			const uchar *bits = data->bits();
			alloc = data->size();
			int w, h;
			switch (format) {
			case I420:
			case YV12:
				w = alloc[0].width(); h = alloc[0].height();
				loadFrame(0, bits, 1, w, h);
				bits += w * h;
				w = alloc[1].width(); h = alloc[1].height();
				loadFrame(1, bits, 1, w, h);
				bits += w * h;
				w = alloc[2].width(); h = alloc[2].height();
				loadFrame(2, bits, 1, w, h);
				break;
			case NV12:
			case NV21:
				w = alloc[0].width(); h = alloc[0].height();
				loadFrame(0, bits, 1, w, h);
				bits += w * h;
				w = alloc[1].width(); h = alloc[1].height();
				w /= 2;
				loadFrame(1, bits, 2, w, h);
				break;
			case RGBA:
			case BGRA:
			case ARGB:
				w = alloc[0].width(); h = alloc[0].height();
				w /= 4;
				loadFrame(0, bits, 4, w, h);
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
		case ARGB:
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, frame[0]);
			break;
		default:
			break;
		}
		p = &resource->program[format];
		p->bind();
		switch (format) {
		case I420:
			p->setUniformValue("u_ValidY", validArea(0));
			p->setUniformValue("u_ValidU", validArea(1));
			p->setUniformValue("u_ValidV", validArea(2));
			break;
		case YV12:
			p->setUniformValue("u_ValidY", validArea(0));
			p->setUniformValue("u_ValidV", validArea(1));
			p->setUniformValue("u_ValidU", validArea(2));
			break;
		case NV12:
		case NV21:
			p->setUniformValue("u_ValidY", validArea(0));
			p->setUniformValue("u_ValidC", validArea(1));
			break;
		case RGBA:
		case BGRA:
		case ARGB:
			p->setUniformValue("u_ValidP", validArea(0));
			break;
		default:
			break;
		}
		break;
	case ABuffer::GLTexture2DHandle:
		switch (format) {
		case RGBA:
		case BGRA:
		case ARGB:
		case GL2D:
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, data->handle().toUInt());
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			break;
		default:
			break;
		}
		p = &resource->program[GL2D];
		p->bind();
		break;
#ifdef GL_TEXTURE_EXTERNAL_OES
	case ABuffer::GLTextureExHandle:
		switch (format) {
		case RGBA:
		case BGRA:
		case ARGB:
		case GLEX:
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_EXTERNAL_OES, data->handle().toUInt());
			glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			break;
		default:
			break;
		}
		p = &resource->program[GLEX];
		p->bind();
		switch (format) {
		case RGBA:
		case BGRA:
		case ARGB:
		case GLEX:
			p->setUniformValue("u_TexMatrix", data->argument("Transform").value<QMatrix4x4>());
			break;
		default:
			break;
		}
		break;
#endif
	}

	QRectF dest = fitRect(lApp->findObject<ARender>()->getPreferSize(), rect);
	GLfloat h = 2.0f / rect.width(), v = 2.0f / rect.height();
	GLfloat l = dest.left() * h - 1, r = dest.right() * h - 1, t = 1 - dest.top() * v, b = 1 - dest.bottom() * v;
	vtx[0] = l; vtx[1] = t;
	vtx[2] = r; vtx[3] = t;
	vtx[4] = l; vtx[5] = b;
	vtx[6] = r; vtx[7] = b;
	p->setAttributeArray(0, vtx, 2);
	p->setAttributeArray(1, tex, 2);
	p->enableAttributeArray(0);
	p->enableAttributeArray(1);
	rect = scaleRect(rect, painter).toRect();
	glViewport(rect.x(), rect.y(), rect.width(), rect.height());
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	painter->endNativePainting();
}

void OpenGLOpaqueRenderPrivate::clear(QPainter *painter, QColor color)
{
	painter->beginNativePainting();

	glClearColor(color.redF(), color.greenF(), color.blueF(), color.alphaF());
	glClear(GL_COLOR_BUFFER_BIT);

	painter->endNativePainting();
}

void OpenGLOpaqueRenderPrivate::setFormat(PFormat *f)
{
	dataLock.lock();
	if (data) {
		data->release();
	}
	data = nullptr;
	dataLock.unlock();
	
	if (f->chroma == "GL2D") {
		format = GL2D;
	}
#ifdef GL_TEXTURE_EXTERNAL_OES
	else if (f->chroma == "GLEX") {
		format = GLEX;
	}
#endif
	else if (f->chroma == "YV12") {
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

	plane.clear();
	QSize size = f->size;
	switch (format) {
	case I420:
	case YV12:
		plane.append(size);
		size.rwidth() /= 2; size.rheight() /= 2;
		plane.append(size);
		plane.append(size);
		break;
	case NV12:
	case NV21:
		plane.append(size);
		size.rheight() /= 2;
		plane.append(size);
		break;
	case RGBA:
	case BGRA:
	case ARGB:
		size.rwidth() *= 4;
		plane.append(size);
		break;
	default:
		return;
	}
	f->plane = plane;
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
