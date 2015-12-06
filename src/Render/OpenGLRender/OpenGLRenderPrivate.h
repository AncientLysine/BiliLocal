#pragma once

#include "../ARenderPrivate.h"
#include <QOpenGLFunctions>

class OpenGLRenderPrivate :public ARenderPrivate, public QOpenGLFunctions
{
public:
	QOpenGLShaderProgram program[5];
	GLfloat vtx[8];
	GLfloat tex[8];

	virtual void initialize();
	void loadTexture(GLuint texture, int channel, int width, int height, quint8 *data, int alignment);
	void drawTexture(GLuint *planes, int format, QRectF dest, QRectF rect);
	virtual void onSwapped();
	virtual bool isVisible() = 0;
	virtual quintptr getHandle() = 0;
	virtual void resize(QSize size) = 0;
	virtual QSize getActualSize() = 0;
	virtual QSize getBufferSize() = 0;
	virtual void draw(QRect rect) = 0;
};
