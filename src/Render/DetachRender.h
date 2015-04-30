#pragma once

#include "ARender.h"

class DetachRenderPrivate;

class DetachRender :public ARender
{
public:
	explicit DetachRender(QObject *parent = 0);
	~DetachRender();

private:
	Q_DECLARE_PRIVATE(DetachRender);

public slots:
	ICache *getCache(const QImage &i);
	quintptr getHandle();
	void resize(QSize);
	QSize getActualSize();
	QSize getBufferSize();
	void draw(QRect rect = QRect());
};
