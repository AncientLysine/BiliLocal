#pragma once

#include "ARenderPrivate.h"
#include <QOpenGLFunctions>

class OpenGLRenderPrivateBase :public ARenderPrivate, public QOpenGLFunctions
{
public:
	QOpenGLShaderProgram program[5];
	GLfloat vtx[8];
	GLfloat tex[8];

	void initialize();
	void uploadTexture(GLuint t, int c, int w, int h, quint8 *d);
	void drawTexture(GLuint *texture, int format, QRectF dest, QRectF rect);
};
