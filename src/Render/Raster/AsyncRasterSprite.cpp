#include "Common.h"
#include "AsyncRasterSprite.h"
#include "../../Graphic/GraphicPrivate.h"

void qt_blurImage(QPainter *p, QImage &blurImage, qreal radius,
	bool quality, bool alphaOnly, int transposed);

void AsyncRasterSprite::prepare()
{
	QPainter painter;
	QColor out = qGray(color.rgb()) < 30 ? Qt::white : Qt::black;
	QImage src(GraphicPrivate::getSize(text, font) + QSize(4, 4), QImage::Format_ARGB32_Premultiplied);
	src.fill(Qt::transparent);
	painter.begin(&src);
	painter.setPen(color.rgb());
	painter.setFont(font);
	painter.setCompositionMode(QPainter::CompositionMode_Source);
	painter.drawText(src.rect().adjusted(2, 2, -2, -2), text);
	painter.end();
	QImage fst(src.size(), QImage::Format_ARGB32_Premultiplied);
	fst.fill(Qt::transparent);
	if (effect == 2) {
		QImage blr = src;
		painter.begin(&fst);
		painter.save();
		qt_blurImage(&painter, blr, 4, false, true, 0);
		painter.restore();
		painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
		painter.fillRect(src.rect(), out);
		painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
		painter.drawImage(0, 0, src);
		painter.end();
		QImage sec(src.size(), QImage::Format_ARGB32_Premultiplied);
		sec.fill(Qt::transparent);
		painter.begin(&sec);
		blr = fst;
		painter.save();
		qt_blurImage(&painter, blr, 4, false, true, 0);
		painter.restore();
		painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
		painter.fillRect(sec.rect(), out);
		painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
		painter.drawImage(0, 0, fst);
		painter.end();
		fst = sec;
	}
	else {
		QImage edg = src;
		painter.begin(&edg);
		painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
		painter.fillRect(edg.rect(), out);
		painter.end();
		painter.begin(&fst);
		switch (effect) {
		case 0:
			painter.drawImage(+1, 0, edg);
			painter.drawImage(-1, 0, edg);
			painter.drawImage(0, +1, edg);
			painter.drawImage(0, -1, edg);
			break;
		case 1:
			painter.drawImage(1, 1, edg);
			break;
		}
		painter.drawImage(0, 0, src);
		painter.end();
	}

	if (frame) {
		painter.begin(&fst);
		painter.setPen(QColor(100, 255, 255));
		painter.setBrush(Qt::NoBrush);
		painter.drawRect(fst.rect().adjusted(0, 0, -1, -1));
		painter.end();
	}

	preAlpha = color.alphaF();
	if (!qFuzzyCompare(preAlpha, 1.0)) {
		QImage sec(src.size(), QImage::Format_ARGB32_Premultiplied);
		sec.fill(Qt::transparent);
		painter.begin(&sec);
		painter.setOpacity(preAlpha);
		painter.drawImage(QPoint(0, 0), fst);
		painter.end();
		fst = sec;
	}
	preImage = fst;
}

void AsyncRasterSprite::draw(QPainter * painter)
{
	if (qFuzzyCompare(preAlpha, 1.0)) {
		painter->setOpacity(color.alphaF());
	}
	else {
		painter->setOpacity(1.0);
	}
	painter->setTransform(transform);
	painter->drawImage(0, 0, preImage);
}

QSize AsyncRasterSprite::getSize()
{
	return preImage.size();
}
