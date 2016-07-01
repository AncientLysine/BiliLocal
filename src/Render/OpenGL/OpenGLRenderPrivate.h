#pragma once

#include "../ARenderPrivate.h"
#include "Atlas.h"
#include <QOpenGLFunctions>

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
		FormatMax
	};

	QScopedArrayPointer<QOpenGLShaderProgram> program;

	QByteArray extensions;

	QOpenGLBuffer vtxBuffer;
	QOpenGLBuffer idxBuffer;

	QScopedPointer<AtlasMgr> manager;

	GLenum pixelFormat(int channel, bool renderable = false) const;

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
	virtual void onSwapped();
	virtual bool isVisible() = 0;
	virtual QObject *getHandle() = 0;
	virtual void resize(QSize size) = 0;
	virtual QSize getActualSize() = 0;
	virtual QSize getBufferSize() = 0;
	virtual void draw(QRect rect) = 0;
};
