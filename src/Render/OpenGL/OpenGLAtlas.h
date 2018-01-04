#pragma once

#include <QtCore>
#include <QtGui>

class OpenGLSprite;
class OpenGLRenderPrivate;

class OpenGLAtlas
{
public:
	static const int MaxSize;
	static const int Padding;
	static const int Spacing;

	explicit OpenGLAtlas(OpenGLRenderPrivate *render);

	~OpenGLAtlas();

	void insert(QList<QImage> &source, int effect, QList<OpenGLSprite> &result);

	inline void AddRef()
	{
		++refNum;
	}

	inline void DecRef()
	{
		if (refNum == 0) {
			return;
		}

		--refNum;

		if (refNum == 0) {
			expiry.start();
			usages.clear();
		}
	}

	inline bool isEmpty() const
	{
		return refNum == 0;
	}

	inline bool expired(int timeout) const
	{
		return isEmpty() && expiry.hasExpired(timeout);
	}

	inline GLuint getTexture() const
	{
		return cached;
	}

private:
	OpenGLRenderPrivate *render;

	size_t refNum;
	GLuint cached;
	QVector<QSize> usages;
	QElapsedTimer expiry;
};

class OpenGLSprite
{
public:
	QRectF rect;

	explicit OpenGLSprite(OpenGLAtlas *parent)
		: parent(parent)
	{
		parent->AddRef();
	}

	OpenGLSprite(const OpenGLSprite &other)
		: rect(other.rect), parent(other.parent)
	{
		parent->AddRef();
	}

	~OpenGLSprite()
	{
		parent->DecRef();
	}

	GLuint getTexture() const
	{
		return parent->getTexture();
	}

private:
	OpenGLAtlas *parent;
};

class OpenGLPacker
{
public:
	struct CreateInfo
	{
		QString text;
		QFont font;
		int effect;
	};

	explicit OpenGLPacker(OpenGLRenderPrivate *render);
	~OpenGLPacker();

	void insert(CreateInfo info, QList<OpenGLSprite> &result, QSize &size);

	inline const QList<OpenGLAtlas *> &getAtlases()
	{
		return atlases;
	}

	inline GLuint getUpload() const
	{
		return upload;
	}

	inline GLuint getBuffer() const
	{
		return buffer;
	}

	void shuffle();
	void squeeze(int timeout);

private:
	OpenGLRenderPrivate *render;

	QList<OpenGLAtlas *> atlases;
	GLuint upload;
	GLuint buffer;
	QHash<CreateInfo, QPair<QList<OpenGLSprite>, QSize>> cached;
};
