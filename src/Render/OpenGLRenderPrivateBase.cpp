#include "OpenGLRenderPrivateBase.h"

namespace
{
	const char *vShaderCode =
		"attribute vec4 VtxCoord;\n"
		"attribute vec2 TexCoord;\n"
		"varying vec2 TexCoordOut;\n"
		"void main(void)\n"
		"{\n"
		"    gl_Position = VtxCoord;\n"
		"    TexCoordOut = TexCoord;\n"
		"}\n";

	const char *fShaderI420 =
		"varying highp vec2 TexCoordOut;\n"
		"uniform sampler2D SamplerY;\n"
		"uniform sampler2D SamplerU;\n"
		"uniform sampler2D SamplerV;\n"
		"void main(void)\n"
		"{\n"
		"    mediump vec3 yuv;\n"
		"    mediump vec3 rgb;\n"
		"    yuv.x = texture2D(SamplerY, TexCoordOut).r - 0.0625;\n"
		"    yuv.y = texture2D(SamplerU, TexCoordOut).r - 0.5;   \n"
		"    yuv.z = texture2D(SamplerV, TexCoordOut).r - 0.5;   \n"
		"    rgb = mat3(1.164,  1.164, 1.164,   \n"
		"               0,     -0.391, 2.018,   \n"
		"               1.596, -0.813, 0) * yuv;\n"
		"    gl_FragColor = vec4(rgb, 1);\n"
		"}";

	const char *fShaderNV12 =
		"varying highp vec2 TexCoordOut;\n"
		"uniform sampler2D SamplerY;\n"
		"uniform sampler2D SamplerA;\n"
		"void main(void)\n"
		"{\n"
		"    mediump vec3 yuv;\n"
		"    mediump vec3 rgb;\n"
		"    yuv.x = texture2D(SamplerY, TexCoordOut).r - 0.0625;\n"
		"    yuv.y = texture2D(SamplerA, TexCoordOut).r - 0.5;   \n"
		"    yuv.z = texture2D(SamplerA, TexCoordOut).a - 0.5;   \n"
		"    rgb = mat3(1.164,  1.164, 1.164,   \n"
		"               0,     -0.391, 2.018,   \n"
		"               1.596, -0.813, 0) * yuv;\n"
		"    gl_FragColor = vec4(rgb, 1);\n"
		"}";

	const char *fShaderNV21 =
		"varying highp vec2 TexCoordOut;\n"
		"uniform sampler2D SamplerY;\n"
		"uniform sampler2D SamplerA;\n"
		"void main(void)\n"
		"{\n"
		"    mediump vec3 yuv;\n"
		"    mediump vec3 rgb;\n"
		"    yuv.x = texture2D(SamplerY, TexCoordOut).r - 0.0625;\n"
		"    yuv.y = texture2D(SamplerA, TexCoordOut).a - 0.5;   \n"
		"    yuv.z = texture2D(SamplerA, TexCoordOut).r - 0.5;   \n"
		"    rgb = mat3(1.164,  1.164, 1.164,   \n"
		"               0,     -0.391, 2.018,   \n"
		"               1.596, -0.813, 0) * yuv;\n"
		"    gl_FragColor = vec4(rgb, 1);\n"
		"}";

	const char *fShaderBGRP =
		"varying highp vec2 TexCoordOut;\n"
		"uniform sampler2D SamplerP;\n"
		"void main(void)\n"
		"{\n"
		"    mediump vec4 p;\n"
		"    p = texture2D(SamplerP, TexCoordOut);\n"
		"    if (p.a != 0.0) {\n"
		"        p.r /= p.a;\n"
		"        p.g /= p.a;\n"
		"        p.b /= p.a;\n"
		"        gl_FragColor = vec4(p.b, p.g, p.r, p.a);\n"
		"    } else {\n"
		"        gl_FragColor = vec4(0.0, 0.0, 0.0, 0.0);\n"
		"    }\n"
		"}";
}

void OpenGLRenderPrivateBase::initialize()
{
	initializeOpenGLFunctions();
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	for (int i = 0; i < 5; ++i){
		const char *fShaderCode = nullptr;
		switch (i){
		case 0:
		case 1:
			fShaderCode = fShaderI420;
			break;
		case 2:
			fShaderCode = fShaderNV12;
			break;
		case 3:
			fShaderCode = fShaderNV21;
			break;
		case 4:
			fShaderCode = fShaderBGRP;
			break;
		}
		QOpenGLShaderProgram &p = program[i];
		p.addShaderFromSourceCode(QOpenGLShader::Vertex, vShaderCode);
		p.addShaderFromSourceCode(QOpenGLShader::Fragment, fShaderCode);
		p.bindAttributeLocation("VtxCoord", 0);
		p.bindAttributeLocation("TexCoord", 1);
		p.bind();
		switch (i){
		case 0:
			p.setUniformValue("SamplerY", 0);
			p.setUniformValue("SamplerU", 1);
			p.setUniformValue("SamplerV", 2);
			break;
		case 1:
			p.setUniformValue("SamplerY", 0);
			p.setUniformValue("SamplerV", 1);
			p.setUniformValue("SamplerU", 2);
			break;
		case 2:
		case 3:
			p.setUniformValue("SamplerY", 0);
			p.setUniformValue("SamplerA", 1);
			break;
		case 4:
			p.setUniformValue("SamplerP", 0);
			break;
		}
	}
	tex[0] = 0; tex[1] = 0;
	tex[2] = 1; tex[3] = 0;
	tex[4] = 0; tex[5] = 1;
	tex[6] = 1; tex[7] = 1;
}

void OpenGLRenderPrivateBase::uploadTexture(GLuint t, int c, int w, int h, quint8 *d)
{
	int f;
	switch (c){
	case 1:
		f = GL_LUMINANCE;
		break;
	case 2:
		f = GL_LUMINANCE_ALPHA;
		break;
	case 3:
		f = GL_RGB;
		break;
	case 4:
		f = GL_RGBA;
		break;
	default:
		return;
	}
	glBindTexture(GL_TEXTURE_2D, t);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, f, w, h, 0, f, GL_UNSIGNED_BYTE, d);
}

void OpenGLRenderPrivateBase::drawTexture(GLuint *texture, int format, QRectF dest, QRectF rect)
{
	QOpenGLShaderProgram &p = program[format];
	p.bind();
	GLfloat h = 2 / rect.width(), v = 2 / rect.height();
	GLfloat l = dest.left()*h - 1, r = dest.right()*h - 1, t = 1 - dest.top()*v, b = 1 - dest.bottom()*v;
	vtx[0] = l; vtx[1] = t;
	vtx[2] = r; vtx[3] = t;
	vtx[4] = l; vtx[5] = b;
	vtx[6] = r; vtx[7] = b;
	p.setAttributeArray(0, vtx, 2);
	p.setAttributeArray(1, tex, 2);
	p.enableAttributeArray(0);
	p.enableAttributeArray(1);
	switch (format){
	case 0:
	case 1:
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, texture[2]);
	case 2:
	case 3:
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, texture[1]);
	case 4:
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture[0]);
		break;
	}
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}
