#pragma once

#include <QtCore>
#include <QtGui>

class Sprite;
class OpenGLRenderPrivate;

class Atlas
{
public:
	static const int MaxSize;
	static const int Padding;
	static const int Spacing;

	explicit Atlas(OpenGLRenderPrivate *render);

	~Atlas();

	void insert(QList<QImage> &source, int effect, QList<Sprite> &result);

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

class Sprite
{
public:
	QRectF rect;

	explicit Sprite(Atlas *parent)
		: parent(parent)
	{
		parent->AddRef();
	}

	Sprite(const Sprite &other)
		: rect(other.rect), parent(other.parent)
	{
		parent->AddRef();
	}

	~Sprite()
	{
		parent->DecRef();
	}

	GLuint getTexture() const
	{
		return parent->getTexture();
	}

private:
	Atlas *parent;
};

class AtlasMgr
{
public:
	struct CreateInfo
	{
		QString text;
		QFont font;
		int effect;
	};

	explicit AtlasMgr(OpenGLRenderPrivate *render);
	~AtlasMgr();

	void insert(CreateInfo info, QList<Sprite> &result, QSize &size);

	inline const QList<Atlas *> &getAtlases()
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

	QList<Atlas *> atlases;
	GLuint upload;
	GLuint buffer;
	QHash<CreateInfo, QPair<QList<Sprite>, QSize>> cached;
};
