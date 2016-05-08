#pragma once

#include "../ASprite.h"
#include <QOpenGLFunctions>

class OpenGLRenderPrivate;

class SyncTextureSprite : public ASprite
{
public:
	SyncTextureSprite(OpenGLRenderPrivate *render);
	virtual void prepare() override;
	virtual void draw(QPainter *painter) override;
	virtual QSize getSize() override;

	class Atlas;

	class Sprite
	{
	public:
		QRect rect;

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

		GLuint getTexture() const;

	private:
		Atlas *parent;
	};

	class Atlas
	{
	public:
		static const size_t MaxSize = 2048;
		static const size_t Padding = 2;

		explicit Atlas(OpenGLRenderPrivate *render);

		~Atlas();

		void insert(QList<QImage> &source, int effect, QList<Sprite> &result);

		inline void AddRef()
		{
			++refNum;
		}

		inline void DecRef()
		{
			--refNum;

			if (refNum == 0) {
				usages.clear();
			}
		}

		inline bool isEmpty()
		{
			return refNum == 0;
		}

		inline GLuint getTexture() const
		{
			return cached;
		}

	private:
		OpenGLRenderPrivate *render;

		size_t refNum;
		GLuint cached;
		GLuint buffer;
		QVector<QSize> usages;
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


		static AtlasMgr &instance(OpenGLRenderPrivate *render)
		{
			static AtlasMgr manager(render);
			return manager;
		}

		void insert(CreateInfo info, QList<Sprite> &result, QSize &size);

		inline const QList<Atlas *> &getAtlases()
		{
			return atlases;
		}

		void squeeze();

	private:
		OpenGLRenderPrivate *render;

		QList<Atlas *> atlases;

		QHash<CreateInfo, QPair<QList<Sprite>, QSize>> cache;

		explicit AtlasMgr(OpenGLRenderPrivate *render)
			: render(render)
		{
		}
	};

private:
	QList<Sprite> sprites;
	QSize size;
	OpenGLRenderPrivate *const render;
};
