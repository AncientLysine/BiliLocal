#pragma once

#include <QList>
#include <QSize>
#include <QString>

class PFormat
{
public:
	QString chroma;		// IN/OUT
	QSize size;			// IN/OUT
	int alignment;		// IN
	QList<QSize> alloc;	// OUT
};

/*
fourcc	memory		pixel		qt				vlc			ffmpeg
RGBA	RRGGBBAA	0xAABBGGRR					RGBA		AV_PIX_FMT_RGBA
BGRA	BBGGRRAA	0xAARRGGBB	RGB32/ARGB32	RV32/BGRA	AV_PIX_FMT_RGB32/AV_PIX_FMT_BGRA
ARGB	AARRGGBB	0xBBGGRRAA	BGR32/BGRA32				AV_PIX_FMT_ARGB
ABGR	AABBGGRR	0xRRGGBBAA								AV_PIX_FMT_ABGR
*/
