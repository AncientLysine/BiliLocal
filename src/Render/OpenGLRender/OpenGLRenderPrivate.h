#pragma once

#include "../ARenderPrivate.h"
#include <QOpenGLFunctions>

class OpenGLRenderPrivate :public ARenderPrivate, public QOpenGLFunctions
{
public:
	QOpenGLShaderProgram program[8];

	struct DrawCall
	{
		GLuint texture;
		QOpenGLShaderProgram *program;
		GLushort size;
	};

	struct DrawAttr
	{
		GLfloat vtxCoord[2];
		GLfloat texCoord[2];
		GLubyte color[4];
	};

	QVector<DrawCall> drawList;
	QVector<DrawAttr> attrList;
	QOpenGLBuffer vtxBuffer;
	QOpenGLBuffer idxBuffer;

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
