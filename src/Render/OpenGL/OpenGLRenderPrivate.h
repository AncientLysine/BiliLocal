#pragma once

#include "../ARenderPrivate.h"
#include "OpenGLAtlas.h"
#include <QtCore>
#include <QtGui>

class OpenGLRenderPrivate :public ARenderPrivate, public QOpenGLFunctions
{
public:
	QRect view;

	enum Format
	{
		I420,
		YV12,
		NV12,
		NV21,
		RGBA,
		BGRA,
		ARGB,
		Danm,
		Stro,
		Proj,
		Glow,
		Max
	};

	QByteArray extensions;

	struct OpenGLRenderResource
	{
		QOpenGLShaderProgram program[Max];
		QOpenGLBuffer vtxBuffer;
		QOpenGLBuffer idxBuffer;
		OpenGLAtlasMgr manager;

		explicit OpenGLRenderResource(OpenGLRenderPrivate *r);
	};
	QScopedPointer<OpenGLRenderResource> resource;

	GLenum pixelFormat(int channel, bool renderable = false) const;
	static QRectF scaleRect(QRectF rect, double factor);
	static QRectF scaleRect(QRectF rect, QPainter *painter);

	struct LoadCall
	{
		GLuint source;
		GLuint target;
		QOpenGLShaderProgram *program;
		GLushort size;
	};

	struct LoadAttr
	{
		GLfloat vtxCoord[2];
		GLfloat texCoord[2];
	};

	QVector<LoadCall> loadList;
	QVector<LoadAttr> loadAttr;

	void appendLoadCall(
		QRectF draw,
		QOpenGLShaderProgram *program,
		QRect data,
		const GLubyte *bits);

	struct DrawCall
	{
		GLuint texture;
		GLushort size;
	};

	struct DrawAttr
	{
		GLfloat vtxCoord[2];
		GLfloat texCoord[2];
		GLubyte color[4];
	};

	QVector<DrawCall> drawList;
	QVector<DrawAttr> drawAttr;

	void appendDrawCall(
		QRectF draw,
		QRectF data,
		GLuint texture,
		QColor color);

	void flushLoad();
	void flushDraw();

	virtual void initialize();
	virtual void drawDanm(QPainter *painter, QRect rect) override;
	virtual void setFormat(PFormat *format) = 0;
	virtual void setBuffer(ABuffer *buffer) = 0;
	virtual void onSwapped();
	virtual bool isVisible() = 0;
	virtual QObject *getHandle() = 0;
	virtual void resize(QSize size) = 0;
	virtual QSize getActualSize() = 0;
	virtual QSize getBufferSize() = 0;
	virtual void draw(QRect rect) = 0;
};
