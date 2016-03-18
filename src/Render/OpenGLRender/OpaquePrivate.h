#pragma once

#include "OpenGLRenderPrivate.h"
#include "../ARender.h"
#include "../../Player/APlayer.h"

class OpenGLOpaqueRenderPrivate :public OpenGLRenderPrivate
{
public:
	QSize inner;
	int format;
	GLuint frame[3];
	QMutex dataLock;
	QList<quint8 *> buffer;
	int alignment;

	virtual void initialize() override
	{
		OpenGLRenderPrivate::initialize();
		glGenTextures(3, frame);
	}

	void loadTexture(int index, int channel, int width, int height)
	{
		OpenGLRenderPrivate::loadTexture(frame[index], channel, width, height, buffer[index], alignment);
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
		QRect dest = fitRect(ARender::instance()->getPreferSize(), rect);
		drawTexture(frame, format, dest, rect);
		painter->endNativePainting();
	}

	void paint(QPaintDevice *device)
	{
		QPainter painter(device);
		painter.setRenderHints(QPainter::SmoothPixmapTransform);
		QRect rect(QPoint(0, 0), ARender::instance()->getActualSize());
		if (APlayer::instance()->getState() == APlayer::Stop){
			drawStop(&painter, rect);
		}
		else{
			drawPlay(&painter, rect);
			drawDanm(&painter, rect);
			drawTime(&painter, rect);
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
	}
};
