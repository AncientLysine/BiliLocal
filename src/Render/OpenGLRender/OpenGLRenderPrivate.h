#pragma once

#include "../ARenderPrivate.h"
#include <QOpenGLFunctions>

class AtlasMgr;

class OpenGLRenderPrivate :public ARenderPrivate, public QOpenGLFunctions
{
public:
	QRect view;

	QOpenGLShaderProgram program[8];

	QByteArray extensions;

	QOpenGLBuffer vtxBuffer;
	QOpenGLBuffer idxBuffer;

	AtlasMgr *manager;

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
	virtual quintptr getHandle() = 0;
	virtual void resize(QSize size) = 0;
	virtual QSize getActualSize() = 0;
	virtual QSize getBufferSize() = 0;
	virtual void draw(QRect rect) = 0;
};
