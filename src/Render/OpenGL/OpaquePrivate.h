#pragma once

#include "OpenGLRenderPrivate.h"
#include "../ARender.h"
#include "../../Local.h"
#include "../../Player/APlayer.h"

class OpenGLOpaqueRenderPrivate :public OpenGLRenderPrivate
{
public:
	QSize inner;
	int format;
	GLuint frame[3];
	QMutex dataLock;
	QList<quint8 *> buffer;
	GLfloat vtx[8];
	GLfloat tex[8];
	int alignment;

	virtual void initialize() override
	{
		OpenGLRenderPrivate::initialize();
		glGenTextures(3, frame);
		tex[0] = 0; tex[1] = 0;
		tex[2] = 1; tex[3] = 0;
		tex[4] = 0; tex[5] = 1;
		tex[6] = 1; tex[7] = 1;
	}

	void loadTexture(int index, int channel, int width, int height)
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
		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, buffer[index]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}

	virtual void drawData(QPainter *painter, QRect rect) override
	{
		if (inner.isEmpty()){
			return;
		}
		painter->beginNativePainting();

		if (dirty){
			int w = inner.width(), h = inner.height();
			dataLock.lock();
			switch (format){
			case 0:
			case 1:
				loadTexture(0, 1, w, h);
				loadTexture(1, 1, w / 2, h / 2);
				loadTexture(2, 1, w / 2, h / 2);
				break;
			case 2:
			case 3:
				loadTexture(0, 1, w, h);
				loadTexture(1, 2, w / 2, h / 2);
				break;
			case 4:
				loadTexture(0, 4, w, h);
				break;
			}
			dirty = false;
			dataLock.unlock();
		}

		QRect dest = fitRect(lApp->findObject<ARender>()->getPreferSize(), rect);
		GLfloat h = 2.0f / rect.width(), v = 2.0f / rect.height();
		GLfloat l = dest.left() * h - 1, r = dest.right() * h - 1, t = 1 - dest.top() * v, b = 1 - dest.bottom() * v;
		vtx[0] = l; vtx[1] = t;
		vtx[2] = r; vtx[3] = t;
		vtx[4] = l; vtx[5] = b;
		vtx[6] = r; vtx[7] = b;
		QOpenGLShaderProgram &p = program[format];
		p.bind();
		p.setAttributeArray(0, vtx, 2);
		p.setAttributeArray(1, tex, 2);
		p.enableAttributeArray(0);
		p.enableAttributeArray(1);
		switch (format) {
		case 0:
		case 1:
			glActiveTexture(GL_TEXTURE2);
			glBindTexture(GL_TEXTURE_2D, frame[2]);
		case 2:
		case 3:
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, frame[1]);
		case 4:
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, frame[0]);
			break;
		default:
			break;
		}
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		painter->endNativePainting();
	}

	void paint(QPaintDevice *device)
	{
		QPainter painter(device);
		painter.setRenderHints(QPainter::SmoothPixmapTransform);
		QRect rect(QPoint(0, 0), lApp->findObject<ARender>()->getActualSize());
		if (lApp->findObject<APlayer>()->getState() == APlayer::Stop){
			drawStop(&painter, rect);
		}
		else{
			drawPlay(&painter, rect);
		}
	}

	virtual QList<quint8 *> getBuffer() override
	{
		dataLock.lock();
		return buffer;
	}

	virtual void releaseBuffer() override
	{
		dirty = true;
		dataLock.unlock();
	}

	inline void align(int &size, int alignment)
	{
		--alignment;
		size = (size + alignment) & (~alignment);
	}

	virtual void setBuffer(QString &chroma, QSize size, int alignment, QList<QSize> *bufferSize) override
	{
		if (chroma == "YV12"){
			format = 1;
		}
		else if (chroma == "NV12"){
			format = 2;
		}
		else if (chroma == "NV21"){
			format = 3;
		}
		else if (chroma == "BGRA"){
			format = 4;
		}
		else{
			format = 0;
			chroma = "I420";
		}

		inner = size;

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

		QList<QSize> plane;
		switch (format){
		case 0:
		case 1:
			plane.append(size);
			size /= 2;
			plane.append(size);
			plane.append(size);
			break;
		case 2:
		case 3:
			plane.append(size);
			size.rheight() /= 2;
			plane.append(size);
			break;
		case 4:
			size.rwidth() *= 4;
			plane.append(size);
			break;
		}
		if (buffer.size() > 0) {
			delete[]buffer[0];
			buffer.clear();
		}
		size_t len = 0;
		for (QSize &s : plane){
			align(s.rwidth(), alignment);
			len += s.width() * s.height();
		}
		quint8 *buf = new quint8[len];
		for (QSize &s : plane) {
			buffer.append(buf);
			buf += s.width() * s.height();
		}
		if (bufferSize)
			bufferSize->swap(plane);
	}

	virtual QSize getBufferSize() override
	{
		return inner;
	}

	virtual ~OpenGLOpaqueRenderPrivate()
	{
		if (!buffer.isEmpty()){
			delete[]buffer[0];
		}
		glDeleteTextures(3, frame);
	}
};
