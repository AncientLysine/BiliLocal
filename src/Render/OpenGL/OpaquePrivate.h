#pragma once

#include "OpenGLRenderPrivate.h"
#include "../ARender.h"
#include "../../Local.h"
#include "../../Player/APlayer.h"

class OpenGLOpaqueRenderPrivate :public OpenGLRenderPrivate
{
public:
	ABuffer *data;
	Format format;
	QSize inner;
	QList<QSize> plane;
	QList<QSize> alloc;
	QMutex dataLock;
	GLfloat vtx[8];
	GLfloat tex[8];
	GLuint frame[3];

	virtual ~OpenGLOpaqueRenderPrivate();

	void loadFrame(int index, const uchar *data, int channel, int width, int height);
	QVector2D validArea(int index);
	void paint(QPaintDevice *device);

	virtual void initialize() override;
	virtual void drawData(QPainter *painter, QRect rect) override;
	virtual void setFormat(PFormat *format) override;
	virtual void setBuffer(ABuffer *buffer) override;
	virtual QSize getBufferSize() override;
};
