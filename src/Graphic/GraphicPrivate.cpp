#include "Common.h"
#include "GraphicPrivate.h"
#include "../Local.h"
#include "../UI/Interface.h"

QFont GraphicPrivate::getFont(int pixelSize, QString family)
{
	QFont font;
	font.setBold(Config::getValue("/Danmaku/Effect", 5) % 2);
	font.setFamily(family);
	font.setPixelSize(pixelSize);
	return font;
}

QSize GraphicPrivate::getSize(QString string, QFont font)
{
	QString fixed;
	fixed.reserve(string.length());
	QVectorIterator<QStringRef> iter(string.splitRef(u'\n'));
	Q_ASSERT(iter.hasNext());
	for (;;) {
		const QStringRef &l = iter.next();
		int i = l.size() - 1;
		for (; i >= 0; --i) {
			const QChar &c = l.at(i);
			if (c == u' ' || c == u'　') {
				fixed.append(c);
			}
		}
		fixed.append(l.left(i));
		if (iter.hasNext()) {
			fixed.append(u'\n');
		}
		else {
			break;
		}
	}
	return QFontMetrics(font).size(0, fixed);
}

QSizeF GraphicPrivate::getPlayer(qint64 date)
{
	return date <= 1384099200 ? QSizeF(545, 388) : QSizeF(862, 568);
}

double GraphicPrivate::getScale(int mode, qint64 date, QSize size)
{
	int m = Config::getValue("/Danmaku/Scale/Fitted", 0x1);
	if (mode == 7 && (m & 0x1) == 0){
		return 0;
	}
	if (mode <= 6 && (m & 0x2) == 0){
		auto scr = lApp->findObject<Interface>()->window()->screen();
		auto dpi = scr ? scr->physicalDotsPerInch() : 96.0;
		return Config::getValue("/Danmaku/Scale/Factor", dpi / 240.0 + 0.6);
	}
	QSizeF player = getPlayer(date);
	return qMin(size.width() / player.width(), size.height() / player.height());
}

double GraphicPrivate::getOverlap(double ff, double fs, double sf, double ss)
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
