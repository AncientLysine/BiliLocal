#include "GraphicPrivate.h"

QFont GraphicPrivate::getFont(int pixelSize, QString family)
{
	QFont font;
	font.setBold(Config::getValue("/Danmaku/Effect", 5) % 2);
	font.setFamily(family);
	font.setPixelSize(pixelSize);
	return font;
}

QSize  GraphicPrivate::getSize(QString string, QFont font)
{
	QStringList lines = string.split('\n');
	for (QString &line : lines){
		QChar h = ' ', f(0x3000);
		int hc = line.count(h), fc = line.count(f);
		line.remove(h).prepend(QString(hc, h));
		line.remove(f).prepend(QString(fc, f));
	}
	return QFontMetrics(font).size(0, lines.join('\n')) + QSize(4, 4);
}

QSizeF GraphicPrivate::getPlayer(qint64 date)
{
	return date <= 1384099200 ? QSizeF(545, 388) : QSizeF(862, 568);
}

double  GraphicPrivate::getScale(int mode, qint64 date, QSize size)
{
	int m = Config::getValue("/Danmaku/Scale/Fitted", 0x1);
	if (mode == 7 && (m & 0x1) == 0){
		return 0;
	}
	if (mode <= 6 && (m & 0x2) == 0){
		return Config::getValue("/Danmaku/Scale/Factor", 1.0);
	}
	QSizeF player = getPlayer(date);
	return qMin(size.width() / player.width(), size.height() / player.height());
}

void qt_blurImage(QPainter *p, QImage &blurImage, qreal radius,
	bool quality, bool alphaOnly, int transposed);

QImage  GraphicPrivate::getCache(QString string,
	int color,
	QFont font,
	QSize size,
	bool frame,
	int effect,
	int opacity)
{
	QPainter painter;
	QColor base(color), edge = qGray(color) < 30 ? Qt::white : Qt::black;
	QImage src(size, QImage::Format_ARGB32_Premultiplied);
	src.fill(Qt::transparent);
	painter.begin(&src);
	painter.setPen(base);
	painter.setFont(font);
	painter.drawText(src.rect().adjusted(2, 2, -2, -2), string);
	painter.end();
	QImage fst(size, QImage::Format_ARGB32_Premultiplied);
	fst.fill(Qt::transparent);
	if (effect == 2){
		QImage blr = src;
		painter.begin(&fst);
		painter.save();
		qt_blurImage(&painter, blr, 4, false, true, 0);
		painter.restore();
		painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
		painter.fillRect(src.rect(), edge);
		painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
		painter.drawImage(0, 0, src);
		painter.end();
		QImage sec(size, QImage::Format_ARGB32_Premultiplied);
		sec.fill(Qt::transparent);
		painter.begin(&sec);
		blr = fst;
		painter.save();
		qt_blurImage(&painter, blr, 4, false, true, 0);
		painter.restore();
		painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
		painter.fillRect(sec.rect(), edge);
		painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
		painter.drawImage(0, 0, fst);
		painter.end();
		fst = sec;
	}
	else{
		QImage edg = src;
		painter.begin(&edg);
		painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
		painter.fillRect(edg.rect(), edge);
		painter.end();
		painter.begin(&fst);
		switch (effect){
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
	if (frame){
		painter.begin(&fst);
		painter.setPen(QColor(100, 255, 255));
		painter.setBrush(Qt::NoBrush);
		painter.drawRect(fst.rect().adjusted(0, 0, -1, -1));
		painter.end();
	}
	if (opacity != 100){
		QImage sec(size, QImage::Format_ARGB32_Premultiplied);
		sec.fill(Qt::transparent);
		painter.begin(&sec);
		painter.setOpacity(opacity / 100.0);
		painter.drawImage(QPoint(0, 0), fst);
		painter.end();
		fst = sec;
	}
	return fst;
}

double  GraphicPrivate::getOverlap(double ff, double fs, double sf, double ss)
{
	if (sf <= ff&&ss >= fs){
		return fs - ff;
	}
	if (sf >= ff&&sf <= fs){
		return qMin(fs - sf, ss - sf);
	}
	if (ss <= fs&&ss >= ff){
		return qMin(ss - ff, ss - sf);
	}
	return 0;
}
