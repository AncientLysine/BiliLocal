#include "Common.h"
#include "SyncTextureSprite.h"
#include "../ARender.h"
#include "OpenGLRenderPrivate.h"
#include "../../Graphic/GraphicPrivate.h"
#include "../../Utils.h"

GLuint SyncTextureSprite::Sprite::getTexture() const
{
	return parent->getTexture();
}

SyncTextureSprite::Atlas::Atlas(OpenGLRenderPrivate *render)
	: refNum(0)
	, render(render)
	, cached(0)
	, buffer(0)
{
	QString extensions((const char *)render->glGetString(GL_EXTENSIONS));
	render->glGenFramebuffers(1, &buffer);
	render->glBindFramebuffer(GL_FRAMEBUFFER, buffer);
	render->glGenTextures(1, &cached);
	render->glBindTexture(GL_TEXTURE_2D, cached);
	GLenum format = GL_RGB;
	if (extensions.contains("texture_rg")) {
		format = GL_RG_EXT;
	}
	render->glTexImage2D(
		GL_TEXTURE_2D,
		0,
		format,
		MaxSize, MaxSize,
		0,
		format,
		GL_UNSIGNED_BYTE,
		nullptr);
	render->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	render->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	render->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	render->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	render->glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, cached, 0);
	render->glBindFramebuffer(GL_FRAMEBUFFER, QOpenGLContext::currentContext()->defaultFramebufferObject());
}

SyncTextureSprite::Atlas::~Atlas()
{
	render->glBindFramebuffer(GL_FRAMEBUFFER, QOpenGLContext::currentContext()->defaultFramebufferObject());
	render->glDeleteFramebuffers(1, &buffer);
	render->glDeleteTextures(1, &cached);
}

void SyncTextureSprite::Atlas::insert(QList<QImage> &source, int effect, QList<SyncTextureSprite::Sprite> &result)
{
	for (auto iter = source.begin(); iter != source.end();) {
		bool fitted = false;
		auto offset = QPoint(0, 0);
		for (QSize &size : usages) {
			if (size.height() >= iter->height() && size.width() + iter->width() <= MaxSize) {
				offset.rx() = size.width();
				size.rwidth() += iter->width();
				fitted = true;
				break;
			}
			offset.ry() += size.height();
		}
		if (!fitted && offset.y() + iter->height() <= MaxSize) {
			usages.append(iter->size());
			fitted = true;
		}
		if (fitted) {
			constexpr int pad = Atlas::Padding;
			Sprite sprite(this);
			sprite.rect = QRect(offset, iter->size());

			GLuint foreId = 0;
			render->glGenTextures(1, &foreId);
			render->glBindTexture(GL_TEXTURE_2D, foreId);
			render->glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
			render->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			render->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			render->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			render->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			render->glTexImage2D(
				GL_TEXTURE_2D,
				0,
				GL_LUMINANCE,
				iter->width(),
				iter->height(),
				0,
				GL_LUMINANCE,
				GL_UNSIGNED_BYTE,
				iter->constBits());

			QOpenGLShaderProgram *p = nullptr;
			switch (effect) {
			case 0:
				p = &render->program[5];
				break;
			case 1:
				p = &render->program[6];
				break;
			case 2:
				p = &render->program[7];
				break;
			default:
				return;
			}
			GLfloat vtx[8];
			GLfloat tex[8];
			const QRect &dest = sprite.rect;
			const GLfloat s = 1.0 / MaxSize;
			GLfloat l = s * dest.left();
			GLfloat r = s * dest.right();
			GLfloat t = s * dest.top();
			GLfloat b = s * dest.bottom();
			l = l * 2 - 1;
			r = r * 2 - 1;
			t = t * 2 - 1;
			b = b * 2 - 1;
			vtx[0] = l; vtx[1] = t;
			vtx[2] = r; vtx[3] = t;
			vtx[4] = l; vtx[5] = b;
			vtx[6] = r; vtx[7] = b;
			tex[0] = 0; tex[1] = 0;
			tex[2] = 1; tex[3] = 0;
			tex[4] = 0; tex[5] = 1;
			tex[6] = 1; tex[7] = 1;
			render->glBindFramebuffer(GL_FRAMEBUFFER, this->buffer);
			render->glViewport(0, 0, MaxSize, MaxSize);
			p->bind();
			p->setAttributeArray(0, vtx, 2);
			p->setAttributeArray(1, tex, 2);
			p->enableAttributeArray(0);
			p->enableAttributeArray(1);
			p->setUniformValue("u_vPixelSize", QVector2D(1.0 / iter->width(), 1.0 / iter->height()));
			render->glActiveTexture(GL_TEXTURE0);
			render->glBindTexture(GL_TEXTURE_2D, foreId);
			render->glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
			render->glBindFramebuffer(GL_FRAMEBUFFER, QOpenGLContext::currentContext()->defaultFramebufferObject());

			render->glDeleteTextures(1, &foreId);

			result.append(sprite);
			iter = source.erase(iter);
		}
		else {
			return;
		}
	}
}

namespace 
{
	typedef SyncTextureSprite::AtlasMgr::CreateInfo CreateInfo;

	inline uint qHash(const CreateInfo &i, uint seed = 0)
	{
		uint h = ::qHash(i.effect, seed);
		h = (h << 1) ^ ::qHash(i.text, seed);
		h = (h << 1) ^ ::qHash(i.font, seed);
		return h;
	}

	inline bool operator ==(const CreateInfo &f, const CreateInfo &s)
	{
		return f.text == s.text && f.font == s.font && f.effect == s.effect;
	}
}

void SyncTextureSprite::AtlasMgr::insert(CreateInfo info, QList<SyncTextureSprite::Sprite> &result, QSize &size)
{
	auto itr = cache.find(info);
	if (cache.end() != itr) {
		result = itr->first; size = itr->second;
		return;
	}

	constexpr int pad = Atlas::Padding;
	QImage source(GraphicPrivate::getSize(info.text, info.font) + QSize(pad , pad) * 2, QImage::Format_Alpha8);
	source.fill(Qt::transparent);
	QPainter painter(&source);
	painter.setFont(info.font);
	painter.setCompositionMode(QPainter::CompositionMode_Source);
	painter.drawText(source.rect().adjusted(pad, pad, -pad, -pad), info.text);
	painter.end();

	QList<QImage> split;
	size = QSize(0, 0);
	size_t sw = source.width(), sh = source.height();
	for (size_t y = 0; y < sh; y += Atlas::MaxSize) {
		++size.rheight();
		for (size_t x = 0; x < sw; x += Atlas::MaxSize) {
			size_t w = std::min(sw - x, Atlas::MaxSize);
			size_t h = std::min(sh - y, Atlas::MaxSize);
			if (w == sw && h == sh) {
				split.append(source);
			}
			else {
				split.append(source.copy(x, y, w, h));
			}
		}
	}
	size.rwidth() = size.height() == 0 ? 0 : split.size() / size.height();

	result.clear();
	for (;;) {
		if (!atlases.isEmpty()) {
			atlases.last()->insert(split, info.effect, result);
		}
		if (split.isEmpty()) {
			break;
		}
		if (atlases.isEmpty() || !atlases.front()->isEmpty()) {
			atlases.append(new Atlas(render));
		}
		else {
			atlases.append(atlases.takeFirst());
		}
		cache.clear();
	}

	if (!result.isEmpty()) {
		cache.insert(info, qMakePair(result, size));
	}
}

void SyncTextureSprite::AtlasMgr::squeeze()
{
	while (atlases.size() > 2 && atlases.front()->isEmpty()) {
		delete atlases.takeFirst();
	}
}

SyncTextureSprite::SyncTextureSprite(OpenGLRenderPrivate *render)
	: render(render)
{
}

void SyncTextureSprite::prepare()
{
}

void SyncTextureSprite::draw(QPainter *)
{
	if (!size.isValid()) {
		AtlasMgr::instance(render).insert({ text, font, effect }, sprites, size);
	}

	if (sprites.isEmpty()) {
		return;
	}

	QOpenGLShaderProgram *p = &render->program[4];
	auto &list = render->drawList;
	auto &attr = render->attrList;
	for(int y = 0; y < size.height(); ++y){
		for (int x = 0; x < size.width(); ++x) {
			Sprite &s = sprites[y * size.width() + x];
			if (list.isEmpty()
				|| list.last().texture != s.getTexture()
				|| list.last().program != p) {
				OpenGLRenderPrivate::DrawCall d;
				d.texture = s.getTexture();
				d.program = p;
				d.size = 0;
				render->drawList.append(d);
			}
			OpenGLRenderPrivate::DrawCall &draw = render->drawList.last();
			GLfloat vtx[8];
			GLfloat tex[8];
			{
				QRectF rect(QPointF(0, 0), ARender::instance()->getActualSize());
				QRectF dest(QPointF(x, y) * Atlas::MaxSize, s.rect.size());
				dest = transform.mapRect(dest);
				GLfloat h = 2 / rect.width(), v = 2 / rect.height();
				GLfloat l = dest.left() * h - 1, r = dest.right() * h - 1, t = 1 - dest.top() * v, b = 1 - dest.bottom() * v;
				vtx[0] = l; vtx[1] = t;
				vtx[2] = r; vtx[3] = t;
				vtx[4] = l; vtx[5] = b;
				vtx[6] = r; vtx[7] = b;
			}
			{
				QRectF dest(s.rect);
				GLfloat s = 1.0 / Atlas::MaxSize;
				GLfloat l = dest.left() * s, r = dest.right() * s, t = dest.top() * s, b = dest.bottom() * s;
				tex[0] = l; tex[1] = t;
				tex[2] = r; tex[3] = t;
				tex[4] = l; tex[5] = b;
				tex[6] = r; tex[7] = b;
			}
			for (int i = 0; i < 4; ++i) {
				attr.append({
					vtx[i * 2], vtx[i * 2 + 1],
					tex[i * 2], tex[i * 2 + 1],
					(GLubyte)color.red(),
					(GLubyte)color.green(),
					(GLubyte)color.blue(),
					(GLubyte)color.alpha(),
				});
			}
			++draw.size;
		}
	}
}

QSize SyncTextureSprite::getSize()
{
	if (size.isNull()) {
		return QSize(0, 0);
	}
	else if (size.isValid()) {
		int w = 0, h = 0, sw = size.width(), sh = size.height();
		for (int x = 0; x < sw; ++x) {
			w += sprites[x].rect.width();
		}
		for (int y = 0; y < sh; ++y) {
			h += sprites[y * sw].rect.height();
		}
		return QSize(w, h);
	}
	else {
		constexpr int pad = Atlas::Padding;
		return GraphicPrivate::getSize(text, font) + QSize(pad, pad) * 2;
	}
}
