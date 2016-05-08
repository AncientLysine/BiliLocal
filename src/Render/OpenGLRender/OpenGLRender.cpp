#include "Common.h"
#include "OpenGLRender.h"
#include "OpenGLRenderPrivate.h"
#include "WidgetPrivate.h"
#include "WindowPrivate.h"
#include "DetachPrivate.h"
#include "SyncTextureSprite.h"
#include "../../Config.h"
#include "../../Player/APlayer.h"

namespace
{
	OpenGLRenderPrivate *choose()
	{
		bool detach = Config::getValue<bool>("/Performance/Option/OpenGL/Detach", 0);
		bool buffer = Config::getValue<bool>("/Performance/Option/OpenGL/Buffer", 1);
		if (detach)
			return new OpenGLDetachRenderPrivate;
		else if (buffer)
			return new OpenGLWidgetRenderPrivate;
		else
			return new OpenGLWindowRenderPrivate;
	}
}

OpenGLRender::OpenGLRender(QObject *parent) :
ARender(choose(), parent)
{
	ins = this;
	setObjectName("ORender");
}

ASprite *OpenGLRender::getSprite()
{
	Q_D(OpenGLRender);
	return new SyncTextureSprite(d);
}

quintptr OpenGLRender::getHandle()
{
	Q_D(OpenGLRender);
	return d->getHandle();
}

void OpenGLRender::resize(QSize size)
{
	Q_D(OpenGLRender);
	d->resize(size);
}

QSize OpenGLRender::getActualSize()
{
	Q_D(OpenGLRender);
	return d->getActualSize();
}

QSize OpenGLRender::getBufferSize()
{
	Q_D(OpenGLRender);
	return d->getBufferSize();
}

void OpenGLRender::draw(QRect rect)
{
	Q_D(OpenGLRender);
	d->draw(rect.isValid() ? rect : QRect(QPoint(0, 0), getActualSize()));
}

namespace
{
	const char *vShaderData =
		"attribute mediump vec4 a_VtxCoord;\n"
		"attribute mediump vec2 a_TexCoord;\n"
		"varying mediump vec2 v_vTexCoord;\n"
		"void main(void)\n"
		"{\n"
		"    gl_Position = a_VtxCoord;\n"
		"    v_vTexCoord = a_TexCoord;\n"
		"}\n";

	const char *fShaderI420 =
		"precision lowp float;\n"
		"varying mediump vec2 v_vTexCoord;\n"
		"uniform sampler2D u_SamplerY;\n"
		"uniform sampler2D u_SamplerU;\n"
		"uniform sampler2D u_SamplerV;\n"
		"void main(void)\n"
		"{\n"
		"    vec4 yuv;\n"
		"    yuv.r = texture2D(u_SamplerY, v_vTexCoord).r;\n"
		"    yuv.g = texture2D(u_SamplerU, v_vTexCoord).r;\n"
		"    yuv.b = texture2D(u_SamplerV, v_vTexCoord).r;\n"
		"    yuv.a = 1.0;\n"
		"    gl_FragColor = mat4(\n"
		"         1.164,  1.164,  1.164, 0, \n"
		"         0,     -0.391,  2.018, 0, \n"
		"         1.596, -0.813,  0,     0, \n"
		"        -0.871,  0.529, -1.082, 1) * yuv;\n"
		"}\n";

	const char *fShaderNV12 =
		"precision lowp float;\n"
		"varying mediump vec2 v_vTexCoord;\n"
		"uniform sampler2D u_SamplerY;\n"
		"uniform sampler2D u_SamplerC;\n"
		"void main(void)\n"
		"{\n"
		"    vec4 yuv;\n"
		"    yuv.r  = texture2D(u_SamplerY, v_vTexCoord).r; \n"
		"    yuv.gb = texture2D(u_SamplerC, v_vTexCoord).ra;\n"
		"    yuv.a  = 1.0;\n"
		"    gl_FragColor = mat4(\n"
		"         1.164,  1.164,  1.164, 0, \n"
		"         0,     -0.391,  2.018, 0, \n"
		"         1.596, -0.813,  0,     0, \n"
		"        -0.871,  0.529, -1.082, 1) * yuv;\n"
		"}\n";

	const char *fShaderNV21 =
		"precision lowp float;\n"
		"varying mediump vec2 v_vTexCoord;\n"
		"uniform sampler2D u_SamplerY;\n"
		"uniform sampler2D u_SamplerC;\n"
		"void main(void)\n"
		"{\n"
		"    vec4 yuv;\n"
		"    yuv.r  = texture2D(u_SamplerY, v_vTexCoord).r; \n"
		"    yuv.gb = texture2D(u_SamplerC, v_vTexCoord).ar;\n"
		"    yuv.a  = 1.0;\n"
		"    gl_FragColor = mat4(\n"
		"         1.164,  1.164,  1.164, 0, \n"
		"         0,     -0.391,  2.018, 0, \n"
		"         1.596, -0.813,  0,     0, \n"
		"        -0.871,  0.529, -1.082, 1) * yuv;\n"
		"}\n";

	const char *vShaderDanm =
		"precision lowp float;\n"
		"attribute mediump vec4 a_VtxCoord;\n"
		"attribute mediump vec2 a_TexCoord;\n"
		"attribute vec4 ForeColor;\n"
		"varying mediump vec2 v_vTexCoord;\n"
		"varying vec3 v_ForeColor;\n"
		"varying vec3 v_BackColor;\n"
		"varying float v_Alpha;\n"
		"void main(void)\n"
		"{\n"
		"    gl_Position = a_VtxCoord;\n"
		"    v_vTexCoord = a_TexCoord;\n"
		"    v_ForeColor = ForeColor.rgb;\n"
		"    if (0.12 > dot(v_ForeColor, vec3(0.34375, 0.5, 0.15625))) {\n"
		"        v_BackColor = vec3(1.0, 1.0, 1.0);\n"
		"    } else {\n"
		"        v_BackColor = vec3(0.0, 0.0, 0.0);\n"
		"    }\n"
		"    v_Alpha = ForeColor.a;\n"
		"}\n";

	const char *fShaderDanm =
		"precision lowp float;\n"
		"varying mediump vec2 v_vTexCoord;\n"
		"varying vec3 v_ForeColor;\n"
		"varying vec3 v_BackColor;\n"
		"varying float v_Alpha;\n"
		"uniform sampler2D u_SamplerD;\n"
		"void main(void)\n"
		"{\n"
		"    vec4 f;\n"
		"    vec4 b;\n"
		"    f.rgb = v_ForeColor;\n"
		"    b.rgb = v_BackColor;\n"
		"    vec2 d = texture2D(u_SamplerD, v_vTexCoord).rg;\n"
		"    f.a = d.r;\n"
		"    b.a = d.g;\n"
		"    float a = mix(b.a, 1.0, f.a);\n"
		"    gl_FragColor.rgb = mix(b.rgb * b.a, f.rgb, f.a) / a;\n"
		"    gl_FragColor.a   = a * v_Alpha;\n"
		"}\n";

	const char *vShaderPost =
		"precision lowp float;\n"
		"attribute mediump vec4 a_VtxCoord;\n"
		"attribute mediump vec2 a_TexCoord;\n"
		"varying mediump vec2 v_vTexCoord;\n"
		"void main(void)\n"
		"{\n"
		"    gl_Position = a_VtxCoord;\n"
		"    v_vTexCoord = a_TexCoord;\n"
		"}\n";

	const char *fShaderStro =
		"precision lowp float;\n"
		"varying mediump vec2 v_vTexCoord;\n"
		"uniform sampler2D u_SamplerA;\n"
		"uniform mediump vec2 u_vPixelSize;\n"
		"void main(void)\n"
		"{\n"
		"    float a;\n"
		"    a = max(a, texture2D(u_SamplerA, vec2(v_vTexCoord.x, v_vTexCoord.y + u_vPixelSize.y)).r);\n"
		"    a = max(a, texture2D(u_SamplerA, vec2(v_vTexCoord.x, v_vTexCoord.y - u_vPixelSize.y)).r);\n"
		"    a = max(a, texture2D(u_SamplerA, vec2(v_vTexCoord.x + u_vPixelSize.x, v_vTexCoord.y)).r);\n"
		"    a = max(a, texture2D(u_SamplerA, vec2(v_vTexCoord.x - u_vPixelSize.x, v_vTexCoord.y)).r);\n"
		"    gl_FragColor.r = texture2D(u_SamplerA, v_vTexCoord).r;\n"
		"    gl_FragColor.g = a;\n"
		"    gl_FragColor.b = 0.0;\n"
		"    gl_FragColor.a = 1.0;\n"
		"}\n";

	const char *fShaderProj =
		"precision lowp float;\n"
		"varying mediump vec2 v_vTexCoord;\n"
		"uniform sampler2D u_SamplerA;\n"
		"uniform mediump vec2 u_vPixelSize;\n"
		"void main(void)\n"
		"{\n"
		"    float a;\n"
		"    a = texture2D(u_SamplerA, v_vTexCoord - u_vPixelSize).r;\n"
		"    gl_FragColor.r = texture2D(u_SamplerA, v_vTexCoord).r;\n"
		"    gl_FragColor.g = a;\n"
		"    gl_FragColor.b = 0.0;\n"
		"    gl_FragColor.a = 1.0;\n"
		"}\n";

	const char *fShaderGlow =
		"precision lowp float;\n"
		"varying mediump vec2 v_vTexCoord;\n"
		"uniform sampler2D u_SamplerA;\n"
		"uniform mediump vec2 u_vPixelSize;\n"
		"void main(void)\n"
		"{\n"
		"    float a = 0.0;\n"
		"    float s = 0.0;\n"
		"    for (float i = -3.0; i <= 3.0; i += 1.0) {\n"
		"        for (float j = -3.0; j <= 3.0; j += 1.0) {\n"
		"            float l = 9.5 - abs(i) - abs(j);\n"
		"            s += l;\n"
		"            a += texture2D(u_SamplerA, v_vTexCoord + u_vPixelSize * vec2(i, j)).r * l;\n"
		"        }\n"
		"    }\n"
		"    a /= s;\n"
		"    gl_FragColor.r = texture2D(u_SamplerA, v_vTexCoord).r;\n"
		"    gl_FragColor.g = a;\n"
		"    gl_FragColor.b = 0.0;\n"
		"    gl_FragColor.a = 1.0;\n"
		"}\n";
}

void OpenGLRenderPrivate::initialize()
{
	initializeOpenGLFunctions();
	for (int i = 0; i < sizeof(program) / sizeof(QOpenGLShaderProgram); ++i){
		const char *vShaderCode = nullptr;
		const char *fShaderCode = nullptr;
		switch (i){
		case 0:
		case 1:
			vShaderCode = vShaderData;
			fShaderCode = fShaderI420;
			break;
		case 2:
			vShaderCode = vShaderData;
			fShaderCode = fShaderNV12;
			break;
		case 3:
			vShaderCode = vShaderData;
			fShaderCode = fShaderNV21;
			break;
		case 4:
			vShaderCode = vShaderDanm;
			fShaderCode = fShaderDanm;
			break;
		case 5:
			vShaderCode = vShaderPost;
			fShaderCode = fShaderStro;
			break;
		case 6:
			vShaderCode = vShaderPost;
			fShaderCode = fShaderProj;
			break;
		case 7:
			vShaderCode = vShaderPost;
			fShaderCode = fShaderGlow;
			break;
		}
		QOpenGLShaderProgram &p = program[i];
		p.addShaderFromSourceCode(QOpenGLShader::Vertex,   vShaderCode);
		p.addShaderFromSourceCode(QOpenGLShader::Fragment, fShaderCode);
		switch (i){
		case 0:
			p.bindAttributeLocation("a_VtxCoord", 0);
			p.bindAttributeLocation("a_TexCoord", 1);
			p.bind();
			p.setUniformValue("u_SamplerY", 0);
			p.setUniformValue("u_SamplerU", 1);
			p.setUniformValue("u_SamplerV", 2);
			break;
		case 1:
			p.bindAttributeLocation("a_VtxCoord", 0);
			p.bindAttributeLocation("a_TexCoord", 1);
			p.bind();
			p.setUniformValue("u_SamplerY", 0);
			p.setUniformValue("u_SamplerV", 1);
			p.setUniformValue("u_SamplerU", 2);
			break;
		case 2:
		case 3:
			p.bindAttributeLocation("a_VtxCoord", 0);
			p.bindAttributeLocation("a_TexCoord", 1);
			p.bind();
			p.setUniformValue("u_SamplerY", 0);
			p.setUniformValue("u_SamplerC", 1);
			break;
		case 4:
			p.bindAttributeLocation("a_VtxCoord", 0);
			p.bindAttributeLocation("a_TexCoord", 1);
			p.bindAttributeLocation("ForeColor", 2);
			p.bind();
			p.setUniformValue("u_SamplerD", 0);
			break;
		case 5:
		case 6:
		case 7:
			p.bindAttributeLocation("a_VtxCoord", 0);
			p.bindAttributeLocation("a_TexCoord", 1);
			p.bind();
			p.setUniformValue("u_SamplerA", 0);
			double size = 1.0 / SyncTextureSprite::Atlas::MaxSize;
			p.setUniformValue("u_vPixelSize", QVector2D(size, size));
			break;
		}
	}

	vtxBuffer = QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
	vtxBuffer.setUsagePattern(QOpenGLBuffer::StreamDraw);
	vtxBuffer.create();

	idxBuffer = QOpenGLBuffer(QOpenGLBuffer::IndexBuffer);
	idxBuffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
	idxBuffer.create();

	QOpenGLContext *c = QOpenGLContext::currentContext();
	timer.setInterval(c->format().swapInterval() / (double)c->screen()->refreshRate());
}

namespace
{
	void fillIndices(QVector<GLushort> &indices, GLushort size)
	{
		indices.clear();
		indices.reserve(size * 6);
		for (GLushort i = 0; i < size; ++i) {
			indices.append(i * 4);
			indices.append(i * 4 + 1);
			indices.append(i * 4 + 3);
			indices.append(i * 4 + 3);
			indices.append(i * 4 + 2);
			indices.append(i * 4);
		}
	}
}

void OpenGLRenderPrivate::drawDanm(QPainter *painter, QRect rect)
{
	painter->beginNativePainting();

	ARenderPrivate::drawDanm(painter, rect);

	glEnable(GL_BLEND);
	glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	glViewport(rect.x(), rect.y(), rect.width(), rect.height());

	const DrawAttr *vtxData = nullptr;
	if (vtxBuffer.isCreated()) {
		vtxBuffer.bind();
		vtxBuffer.allocate(attrList.constData(), attrList.size() * sizeof(DrawAttr));
	}
	else {
		vtxData = attrList.constData();
	}

	const GLushort *idxData = nullptr;
	GLushort size = 0;
	for (const DrawCall &iter : drawList) {
		size = qMax(size, iter.size);
	}
	if (idxBuffer.isCreated()) {
		idxBuffer.bind();
		if (size * 6u > (GLushort)idxBuffer.size() / sizeof(GLushort)) {
			QVector<GLushort> indices;
			fillIndices(indices, qNextPowerOfTwo(size));
			idxBuffer.allocate(indices.constData(), indices.size() * sizeof(GLushort));
		}
	}
	else {
		static QVector<GLushort> indices;
		if (size * 6u > (GLushort)indices.size()) {
			fillIndices(indices, qNextPowerOfTwo(size));
		}
		idxData = indices.constData();
	}

	for (const DrawCall &iter : drawList) {
		auto p = iter.program;
		p->bind();
		p->setAttributeArray(0, vtxData->vtxCoord, 2, sizeof(DrawAttr));
		p->setAttributeArray(1, vtxData->texCoord, 2, sizeof(DrawAttr));
		p->setAttributeArray(2, GL_UNSIGNED_BYTE, vtxData->color, 4, sizeof(DrawAttr));
		p->enableAttributeArray(0);
		p->enableAttributeArray(1);
		p->enableAttributeArray(2);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, iter.texture);
		glDrawElements(GL_TRIANGLES, iter.size * 6, GL_UNSIGNED_SHORT, idxData);
		vtxData += iter.size * 4;
	}

	if (vtxBuffer.isCreated()) {
		vtxBuffer.release();
	}
	if (idxBuffer.isCreated()) {
		idxBuffer.release();
	}

#ifdef GRAPHIC_DEBUG
	auto atlases = SyncTextureSprite::AtlasMgr::instance(this).getAtlases();
	for (int i = 0; i < atlases.size(); ++i) {
		auto *a = atlases[i];
		auto &p = program[4];
		p.bind();
		GLfloat h = 2.0 / rect.width(), v = 2.0 / rect.height();
		GLfloat s = std::min(256.0f, rect.width() / (GLfloat)atlases.size());
		GLfloat hs = s * h, vs = s * v;
		GLfloat vtx[8] = {
			-1 + hs * i, -1 + vs,
			-1 + hs * (i + 1), -1 + vs,
			-1 + hs * i, -1,
			-1 + hs * (i + 1), -1
		};
		GLfloat tex[8] = { 0, 0, 1, 0, 0, 1, 1 ,1 };
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, a->getTexture());
		p.setAttributeArray(0, vtx, 2);
		p.setAttributeArray(1, tex, 2);
		p.setAttributeValue(2, QColor(Qt::white));
		p.enableAttributeArray(0);
		p.enableAttributeArray(1);
		p.disableAttributeArray(2);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	}
#endif
	painter->endNativePainting();

	drawList.clear();
	attrList.clear();
}

void OpenGLRenderPrivate::onSwapped()
{
	if (isVisible() && APlayer::instance()->getState() == APlayer::Play){
		if (timer.swap()){
			ARender::instance()->draw();
		}
		else{
			QTimer::singleShot(1, ARender::instance(), SLOT(draw()));
		}
	}
}
