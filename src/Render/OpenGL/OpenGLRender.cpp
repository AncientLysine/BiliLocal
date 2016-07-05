#include "Common.h"
#include "OpenGLRender.h"
#include "OpenGLRenderPrivate.h"
#include "SyncTextureSprite.h"
#ifdef INTERFACE_WIDGET
#include "WidgetPrivate.h"
#include "WindowPrivate.h"
#endif
#ifdef INTERFACE_QUICK2
#include "Quick2Private.h"
#endif
#include "DetachPrivate.h"
#include "../../Config.h"
#include "../../Player/APlayer.h"
#include "../../UI/Interface.h"

OpenGLRender::OpenGLRender(QObject *parent) :
	ARender(parent)
{
	setObjectName("ORender");
}

void OpenGLRender::setup()
{
	QStringList l;
#ifdef INTERFACE_WIDGET
	l << "Widget";
	l << "Window";
#endif
#ifdef INTERFACE_QUICK2
	l << "Quick2";
#endif
	l << "Detach";
	QString name;
	switch (l.size()) {
	case 0:
		return;
	case 1:
		name = l[0];
		break;
	default:
		name = Config::getValue("/Render/Option/OpenGL/Output", l[0]);
		name = l.contains(name) ? name : l[0];
		break;
	}
	auto ui = lApp->findObject<Interface>();
	if (name == "Detach") {
		d_ptr = new OpenGLDetachRenderPrivate();
	}
#ifdef INTERFACE_WIDGET
	else if (ui->widget() != nullptr && name == "Window") {
		d_ptr = new OpenGLWindowRenderPrivate();
	}
	else if (ui->widget() != nullptr) {
		d_ptr = new OpenGLWidgetRenderPrivate();
	}
#endif
#ifdef INTERFACE_QUICK2
	else if (ui->widget() == nullptr) {
		d_ptr = new OpenGLQuick2RenderPrivate();
	}
#endif
	else{
		return;
	}

	ARender::setup();

	Q_D(OpenGLRender);

	connect(
		lApp->findObject<APlayer>(),
		&APlayer::timeChanged,
		this,
		[d]() { d->manager->squeeze(5000); });

	connect(
		lApp->findObject<APlayer>(),
		&APlayer::reach,
		this,
		[d]() { d->manager->squeeze(0); },
		Qt::QueuedConnection);
}

void OpenGLRender::setFormat(PFormat *format)
{
	Q_D(OpenGLRender);
	d->setFormat(format);
}

void OpenGLRender::setBuffer(ABuffer *buffer)
{
	Q_D(OpenGLRender);
	d->setBuffer(buffer);
}

ASprite *OpenGLRender::getSprite()
{
	Q_D(OpenGLRender);
	return new SyncTextureSprite(d);
}

QObject *OpenGLRender::getHandle()
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
		"uniform mediump vec2 u_ValidY;\n"
		"uniform mediump vec2 u_ValidU;\n"
		"uniform mediump vec2 u_ValidV;\n"
		"void main(void)\n"
		"{\n"
		"    vec4 yuv;\n"
		"    yuv.r = texture2D(u_SamplerY, v_vTexCoord * u_ValidY).r;\n"
		"    yuv.g = texture2D(u_SamplerU, v_vTexCoord * u_ValidU).r;\n"
		"    yuv.b = texture2D(u_SamplerV, v_vTexCoord * u_ValidV).r;\n"
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
		"uniform mediump vec2 u_ValidY;\n"
		"uniform mediump vec2 u_ValidC;\n"
		"void main(void)\n"
		"{\n"
		"    vec4 yuv;\n"
		"    yuv.r  = texture2D(u_SamplerY, v_vTexCoord * u_ValidY).r; \n"
		"    yuv.gb = texture2D(u_SamplerC, v_vTexCoord * u_ValidC).ra;\n"
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
		"uniform mediump vec2 u_ValidY;\n"
		"uniform mediump vec2 u_ValidC;\n"
		"void main(void)\n"
		"{\n"
		"    vec4 yuv;\n"
		"    yuv.r  = texture2D(u_SamplerY, v_vTexCoord * u_ValidY).r; \n"
		"    yuv.gb = texture2D(u_SamplerC, v_vTexCoord * u_ValidC).ar;\n"
		"    yuv.a  = 1.0;\n"
		"    gl_FragColor = mat4(\n"
		"         1.164,  1.164,  1.164, 0, \n"
		"         0,     -0.391,  2.018, 0, \n"
		"         1.596, -0.813,  0,     0, \n"
		"        -0.871,  0.529, -1.082, 1) * yuv;\n"
		"}\n";

	const char *fShaderRGBA =
		"precision lowp float;\n"
		"varying mediump vec2 v_vTexCoord;\n"
		"uniform sampler2D u_SamplerP;\n"
		"uniform mediump vec2 u_ValidP;\n"
		"void main(void)\n"
		"{\n"
		"    gl_FragColor.rgba = texture2D(u_SamplerP, v_vTexCoord * u_ValidP).rgba;\n"
		"}\n";

	const char *fShaderBGRA =
		"precision lowp float;\n"
		"varying mediump vec2 v_vTexCoord;\n"
		"uniform sampler2D u_SamplerP;\n"
		"uniform mediump vec2 u_ValidP;\n"
		"void main(void)\n"
		"{\n"
		"    gl_FragColor.bgra = texture2D(u_SamplerP, v_vTexCoord * u_ValidP).rgba;\n"
		"}\n";

	const char *fShaderARGB =
		"precision lowp float;\n"
		"varying mediump vec2 v_vTexCoord;\n"
		"uniform sampler2D u_SamplerP;\n"
		"uniform mediump vec2 u_ValidP;\n"
		"void main(void)\n"
		"{\n"
		"    gl_FragColor.argb = texture2D(u_SamplerP, v_vTexCoord * u_ValidP).rgba;\n"
		"}\n";

	const char *vShaderDanm =
		"precision lowp float;\n"
		"attribute mediump vec4 a_VtxCoord;\n"
		"attribute mediump vec2 a_TexCoord;\n"
		"attribute vec4 a_ForeColor;\n"
		"varying mediump vec2 v_vTexCoord;\n"
		"varying vec3 v_ForeColor;\n"
		"varying vec3 v_BackColor;\n"
		"varying float v_Alpha;\n"
		"void main(void)\n"
		"{\n"
		"    gl_Position = a_VtxCoord;\n"
		"    v_vTexCoord = a_TexCoord;\n"
		"    v_ForeColor = a_ForeColor.rgb;\n"
		"    if (0.12 > dot(v_ForeColor, vec3(0.34375, 0.5, 0.15625))) {\n"
		"        v_BackColor = vec3(1.0, 1.0, 1.0);\n"
		"    } else {\n"
		"        v_BackColor = vec3(0.0, 0.0, 0.0);\n"
		"    }\n"
		"    v_Alpha = a_ForeColor.a;\n"
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

GLenum OpenGLRenderPrivate::pixelFormat(int channel, bool renderable) const
{
	switch (channel)
	{
	case 1:
		if (extensions.contains("texture_rg")) {
#ifdef GL_RED_EXT
			return GL_RED_EXT;
#else
			return GL_RED;
#endif
		}
		else if (renderable) {
			return GL_RGB;
		}
		else {
			return GL_LUMINANCE;
		}
	case 2:
		if (extensions.contains("texture_rg")) {
#ifdef GL_RG_EXT
			return GL_RG_EXT;
#else
			return GL_RG;
#endif
		}
		else if (renderable) {
			return GL_RGB;
		}
		else {
			return GL_LUMINANCE_ALPHA;
		}
	case 3:
		return GL_RGB;
	case 4:
		return GL_RGBA;
	default:
		return GL_INVALID_ENUM;
	}
}

void OpenGLRenderPrivate::appendLoadCall(QRectF draw, QOpenGLShaderProgram *program, QRect data, const GLubyte *bits)
{
	GLuint source = manager->getUpload();
	GLuint target = manager->getBuffer();

	GLenum format = pixelFormat(1);
	glBindTexture(GL_TEXTURE_2D, source);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
	glTexSubImage2D(
		GL_TEXTURE_2D,
		0,
		data.x(),
		data.y(),
		data.width(),
		data.height(),
		format,
		GL_UNSIGNED_BYTE,
		bits);

	if (loadList.isEmpty()
		|| loadList.last().source != source
		|| loadList.last().target != target
		|| loadList.last().program != program) {
		OpenGLRenderPrivate::LoadCall l;
		l.source = source;
		l.target = target;
		l.program = program;
		l.size = 0;
		loadList.append(l);
	}

	GLfloat vtx[8];
	GLfloat tex[8];
	{
		const GLfloat s = 1.0 / Atlas::MaxSize;
		GLfloat l = s * draw.left();
		GLfloat r = s * draw.right();
		GLfloat t = s * draw.top();
		GLfloat b = s * draw.bottom();
		tex[0] = l; tex[1] = t;
		tex[2] = r; tex[3] = t;
		tex[4] = l; tex[5] = b;
		tex[6] = r; tex[7] = b;
		l = l * 2 - 1;
		r = r * 2 - 1;
		t = t * 2 - 1;
		b = b * 2 - 1;
		vtx[0] = l; vtx[1] = t;
		vtx[2] = r; vtx[3] = t;
		vtx[4] = l; vtx[5] = b;
		vtx[6] = r; vtx[7] = b;
	}

	for (int i = 0; i < 4; ++i) {
		loadAttr.append({
			vtx[i * 2], vtx[i * 2 + 1],
			tex[i * 2], tex[i * 2 + 1]
		});
	}

	++(loadList.last().size);
}

void OpenGLRenderPrivate::appendDrawCall(QRectF draw, QRectF data, GLuint texture, QColor color)
{
	if (drawList.isEmpty()
		|| drawList.last().texture != texture) {
		OpenGLRenderPrivate::DrawCall d;
		d.texture = texture;
		d.size = 0;
		drawList.append(d);
	}

	GLfloat vtx[8];
	{
		GLfloat h = 2.0 / view.width(), v = 2.0 / view.height();
		GLfloat l = draw.left() * h - 1;
		GLfloat r = draw.right() * h - 1;
		GLfloat t = 1 - draw.top() * v;
		GLfloat b = 1 - draw.bottom() * v;
		vtx[0] = l; vtx[1] = t;
		vtx[2] = r; vtx[3] = t;
		vtx[4] = l; vtx[5] = b;
		vtx[6] = r; vtx[7] = b;
	}
	GLfloat tex[8];
	{
		GLfloat s = 1.0 / Atlas::MaxSize;
		GLfloat l = s * data.left();
		GLfloat r = s * data.right();
		GLfloat t = s * data.top();
		GLfloat b = s * data.bottom();
		tex[0] = l; tex[1] = t;
		tex[2] = r; tex[3] = t;
		tex[4] = l; tex[5] = b;
		tex[6] = r; tex[7] = b;
	}
	GLubyte col[4] = {
		(GLubyte)color.red(),
		(GLubyte)color.green(),
		(GLubyte)color.blue(),
		(GLubyte)color.alpha()
	};

	for (int i = 0; i < 4; ++i) {
		drawAttr.append({
			vtx[i * 2], vtx[i * 2 + 1],
			tex[i * 2], tex[i * 2 + 1],
			col[0], col[1], col[2], col[3]
		});
	}

	++(drawList.last().size);
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

void OpenGLRenderPrivate::flushLoad()
{
	glEnable(GL_BLEND);
	glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	glViewport(0, 0, Atlas::MaxSize, Atlas::MaxSize);

	const LoadAttr *vtxData = nullptr;
	const GLushort *idxData = nullptr;

	if (vtxBuffer.isCreated()) {
		vtxBuffer.bind();
		vtxBuffer.allocate(loadAttr.constData(), loadAttr.size() * sizeof(LoadAttr));
	}
	else {
		vtxData = loadAttr.constData();
	}

	GLushort size = 0;
	for (const LoadCall &iter : loadList) {
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

	for (const LoadCall &iter : loadList) {
		glBindFramebuffer(GL_FRAMEBUFFER, iter.target);
		auto p = iter.program;
		p->bind();
		p->setAttributeArray(0, vtxData->vtxCoord, 2, sizeof(LoadAttr));
		p->setAttributeArray(1, vtxData->texCoord, 2, sizeof(LoadAttr));
		p->enableAttributeArray(0);
		p->enableAttributeArray(1);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, iter.source);
		glDrawElements(GL_TRIANGLES, iter.size * 6, GL_UNSIGNED_SHORT, idxData);
		vtxData += iter.size * 4;
	}

	if (vtxBuffer.isCreated()) {
		vtxBuffer.release();
	}
	if (idxBuffer.isCreated()) {
		idxBuffer.release();
	}

	loadList.resize(0);
	loadAttr.resize(0);
}

void OpenGLRenderPrivate::flushDraw()
{
	glEnable(GL_BLEND);
	glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	glViewport(view.x(), view.y(), view.width(), view.height());
	glBindFramebuffer(GL_FRAMEBUFFER, QOpenGLContext::currentContext()->defaultFramebufferObject());

	const DrawAttr *vtxData = nullptr;
	const GLushort *idxData = nullptr;

	if (vtxBuffer.isCreated()) {
		vtxBuffer.bind();
		vtxBuffer.allocate(drawAttr.constData(), drawAttr.size() * sizeof(DrawAttr));
	}
	else {
		vtxData = drawAttr.constData();
	}

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
		auto p = &program[Danm];
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

	drawList.resize(0);
	drawAttr.resize(0);
}

void OpenGLRenderPrivate::initialize()
{
	initializeOpenGLFunctions();
	program.reset(new QOpenGLShaderProgram[FormatMax]);
	for (int i = 0; i < FormatMax; ++i){
		const char *vShaderCode = nullptr;
		const char *fShaderCode = nullptr;
		switch (i){
		case I420:
		case YV12:
			vShaderCode = vShaderData;
			fShaderCode = fShaderI420;
			break;
		case NV12:
			vShaderCode = vShaderData;
			fShaderCode = fShaderNV12;
			break;
		case NV21:
			vShaderCode = vShaderData;
			fShaderCode = fShaderNV21;
			break;
		case RGBA:
			vShaderCode = vShaderData;
			fShaderCode = fShaderRGBA;
			break;
		case BGRA:
			vShaderCode = vShaderData;
			fShaderCode = fShaderBGRA;
			break;
		case ARGB:
			vShaderCode = vShaderData;
			fShaderCode = fShaderARGB;
			break;
		case Danm:
			vShaderCode = vShaderDanm;
			fShaderCode = fShaderDanm;
			break;
		case Stro:
			vShaderCode = vShaderPost;
			fShaderCode = fShaderStro;
			break;
		case Proj:
			vShaderCode = vShaderPost;
			fShaderCode = fShaderProj;
			break;
		case Glow:
			vShaderCode = vShaderPost;
			fShaderCode = fShaderGlow;
			break;
		}
		QOpenGLShaderProgram &p = program[i];
		p.addShaderFromSourceCode(QOpenGLShader::Vertex,   vShaderCode);
		p.addShaderFromSourceCode(QOpenGLShader::Fragment, fShaderCode);
		switch (i){
		case I420:
			p.bindAttributeLocation("a_VtxCoord", 0);
			p.bindAttributeLocation("a_TexCoord", 1);
			p.bind();
			p.setUniformValue("u_SamplerY", 0);
			p.setUniformValue("u_SamplerU", 1);
			p.setUniformValue("u_SamplerV", 2);
			break;
		case YV12:
			p.bindAttributeLocation("a_VtxCoord", 0);
			p.bindAttributeLocation("a_TexCoord", 1);
			p.bind();
			p.setUniformValue("u_SamplerY", 0);
			p.setUniformValue("u_SamplerV", 1);
			p.setUniformValue("u_SamplerU", 2);
			break;
		case NV12:
		case NV21:
			p.bindAttributeLocation("a_VtxCoord", 0);
			p.bindAttributeLocation("a_TexCoord", 1);
			p.bind();
			p.setUniformValue("u_SamplerY", 0);
			p.setUniformValue("u_SamplerC", 1);
			break;
		case RGBA:
		case BGRA:
		case ARGB:
			p.bindAttributeLocation("a_VtxCoord", 0);
			p.bindAttributeLocation("a_TexCoord", 1);
			p.bind();
			p.setUniformValue("u_SamplerP", 0);
			break;
		case Danm:
			p.bindAttributeLocation("a_VtxCoord", 0);
			p.bindAttributeLocation("a_TexCoord", 1);
			p.bindAttributeLocation("a_ForeColor", 2);
			p.bind();
			p.setUniformValue("u_SamplerD", 0);
			break;
		case Stro:
		case Proj:
		case Glow:
			p.bindAttributeLocation("a_VtxCoord", 0);
			p.bindAttributeLocation("a_TexCoord", 1);
			p.bind();
			p.setUniformValue("u_SamplerA", 0);
			double size = 1.0 / Atlas::MaxSize;
			p.setUniformValue("u_vPixelSize", QVector2D(size, size));
			break;
		}
	}

	extensions = (const char *)glGetString(GL_EXTENSIONS);

	vtxBuffer = QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
	vtxBuffer.setUsagePattern(QOpenGLBuffer::StreamDraw);
	vtxBuffer.create();

	idxBuffer = QOpenGLBuffer(QOpenGLBuffer::IndexBuffer);
	idxBuffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
	idxBuffer.create();

	manager.reset(new AtlasMgr(this));

	QOpenGLContext *c = QOpenGLContext::currentContext();
	timer.setInterval(c->format().swapInterval() / (double)c->screen()->refreshRate());
}

void OpenGLRenderPrivate::drawDanm(QPainter *painter, QRect rect)
{
	painter->beginNativePainting();

	view = rect;
	ARenderPrivate::drawDanm(painter, rect);
	flushLoad();
	flushDraw();

#ifdef GRAPHIC_DEBUG
	auto atlases = manager->getAtlases();
	for (int i = 0; i < atlases.size(); ++i) {
		auto *a = atlases[i];
		auto &p = program[Danm];
		p.bind();
		GLfloat h = 2.0 / rect.width(), v = 2.0 / rect.height();
		GLfloat s = qMin(256.0f, rect.width() / (GLfloat)atlases.size());
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
}

void OpenGLRenderPrivate::onSwapped()
{
	if (isVisible() && lApp->findObject<APlayer>()->getState() == APlayer::Play){
		if (timer.swap()){
			lApp->findObject<ARender>()->draw();
		}
		else{
			QTimer::singleShot(1, lApp->findObject<ARender>(), SLOT(draw()));
		}
	}
}
