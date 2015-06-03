#pragma once

#include "ARender.h"

class ISpirit;
class OpenGLRenderPrivate;

class OpenGLRender :public ARender
{
public:
	explicit OpenGLRender(QObject *parent = 0);

private:
	Q_DECLARE_PRIVATE(OpenGLRender);

public slots:
	virtual ISpirit *getSpirit(const QImage &) override;
	virtual quintptr getHandle() override;
	virtual void resize(QSize size) override;
	virtual QSize getActualSize() override;
	virtual QSize getBufferSize() override;
	virtual void draw(QRect rect = QRect()) override;
};
