#pragma once

#include <QList>
#include <QSize>
#include <QVariant>

class ABuffer
{
public:
	enum HandleType
	{
		NoHandle,
		GLTextureHandle
	};

	virtual void release()
	{
		delete this;
	}

	virtual bool map() = 0;
	virtual const uchar *bits() const = 0;
	virtual QList<QSize> size() const = 0;
	virtual void unmap() = 0;

	virtual HandleType handleType() const = 0;
	virtual QVariant handle() const = 0;

protected:
	virtual ~ABuffer() = default;
};
