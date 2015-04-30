/*=======================================================================
*
*   Copyright (C) 2013-2015 Lysine.
*
*   Filename:    ARender.cpp
*   Time:        2013/12/27
*   Author:      Lysine
*
*   Lysine is a student majoring in Software Engineering
*   from the School of Software, SUN YAT-SEN UNIVERSITY.
*
*   This program is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option) any later version.
*
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.

*   You should have received a copy of the GNU General Public License
*   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
=========================================================================*/

#include "ARender.h"
#include "ARenderPrivate.h"
#include "../Config.h"
#include "../Local.h"
#include "../Utils.h"
#include "../Model/Danmaku.h"
#include "../Player/APlayer.h"
#include <QtWidgets>

#ifdef RENDER_RASTER
#include "RasterRender.h"
#endif
#ifdef RENDER_OPENGL
#include "OpenGLRender.h"
#endif
#ifdef RENDER_DETACH
#include "DetachRender.h"
#endif

ARender *ARender::ins = nullptr;

ARender *ARender::instance()
{
	if (ins){
		return ins;
	}
	QString r;
	QStringList l = Utils::getRenderModules();
	switch (l.size()){
	case 0:
		break;
	case 1:
		r = l[0];
		break;
	default:
		r = Config::getValue("/Performance/Render", l[0]);
		r = l.contains(r) ? r : l[0];
		break;
	}
#ifdef RENDER_OPENGL
	if (r == "OpenGL"){
		return new OpenGLRender(qApp);
	}
#endif
#ifdef RENDER_RASTER
	if (r=="Raster"){
		return new RasterRender(qApp);
	}
#endif
#ifdef RENDER_DETACH
	if (r == "Detach"){
		return new DetachRender(qApp);
	}
#endif
	return nullptr;
}

ARender::ARender(ARenderPrivate *data, QObject *parent) :
QObject(parent), d_ptr(data)
{
	Q_D(ARender);
	d->time = 0;
	connect(lApp, &Local::aboutToQuit, this, &ARender::deleteLater);
	if (Config::getValue("/Interface/Version", true)){
		d->tv.setFileName(":/Picture/tv.gif");
		d->tv.setCacheMode(QMovie::CacheAll);
		d->tv.start();
		d->me = QImage(":/Picture/version.png");
		connect(APlayer::instance(), &APlayer::begin, &d->tv, &QMovie::stop);
		connect(APlayer::instance(), &APlayer::reach, &d->tv, &QMovie::start);
		connect(&d->tv, &QMovie::updated, [=](){
			QImage cf = d->tv.currentImage();
			QPoint ps = QRect(QPoint(0, 0), getActualSize()).center() - cf.rect().center();
			ps.ry() -= 40;
			draw(QRect(ps, cf.size()));
		});
	}
	QString path = Config::getValue("/Interface/Background", QString());
	if (!path.isEmpty()){
		d->background = QImage(path);
	}
	d->sound = QImage(":/Picture/sound.png");
	d->music = 1;
	d->dirty = 0;
	d->videoAspectRatio = 0;
	d->pixelAspectRatio = 1;
	connect(APlayer::instance(), &APlayer::stateChanged, [d](){d->last = QTime(); });
}

ARender::~ARender()
{
	delete d_ptr;
}

QList<quint8 *> ARender::getBuffer()
{
	Q_D(ARender);
	return d->getBuffer();
}

void ARender::releaseBuffer()
{
	Q_D(ARender);
	d->releaseBuffer();
}

void ARender::setBuffer(QString &chroma, QSize size, QList<QSize> *bufferSize)
{
	Q_D(ARender);
	d->setBuffer(chroma, size, bufferSize);
}

void ARender::setBackground(QString path)
{
	Q_D(ARender);
	d->background = QImage(path);
	Config::setValue("/Interface/Background", path);
}

void ARender::setMusic(bool music)
{
	Q_D(ARender);
	d->music = music;
}

void ARender::setDisplayTime(double t)
{
	Q_D(ARender);
	d->time = t;
	QSize s = getActualSize();
	draw(QRect(0, s.height() - 2, s.width()*d->time, 2));
}

void ARender::setVideoAspectRatio(double ratio)
{
	Q_D(ARender);
	d->videoAspectRatio = ratio;
}

void ARender::setPixelAspectRatio(double ratio)
{
	Q_D(ARender);
	d->pixelAspectRatio = ratio;
}

QSize ARender::getPreferSize()
{
	Q_D(ARender);
	if (d->music){
		return QSize();
	}
	if (d->pref.isValid()){
		return d->pref;
	}
	QSize s = getBufferSize();
	d->pixelAspectRatio > 1 ? (s.rwidth() *= d->pixelAspectRatio) : (s.rheight() /= d->pixelAspectRatio);
	return s;
}

void ARender::setPreferSize(QSize size)
{
	Q_D(ARender);
	d->pref = size;
}


QRect ARenderPrivate::fitRect(QSize size, QRect rect)
{
	QRect dest;
	QSizeF s = videoAspectRatio > 0 ? QSizeF(videoAspectRatio, 1) : QSizeF(size);
	dest.setSize(s.scaled(rect.size(), Qt::KeepAspectRatio).toSize() / 4 * 4);
	dest.moveCenter(rect.center());
	return dest;
}

void ARenderPrivate::drawPlay(QPainter *painter, QRect rect)
{
	painter->fillRect(rect, Qt::black);
	if (music){
		painter->drawImage(rect.center() - QRect(QPoint(0, 0), sound.size()).center(), sound);
	}
	else{
		drawData(painter, rect);
	}
	drawDanm(painter, rect);
}

void ARenderPrivate::drawStop(QPainter *painter, QRect rect)
{
	if (background.isNull()){
		painter->fillRect(rect, qApp->palette().color(QPalette::Window));
	}
	else{
		QRect dest = rect;
		dest.setSize(background.size().scaled(dest.size(), Qt::KeepAspectRatioByExpanding));
		dest.moveCenter(rect.center());
		painter->drawImage(dest, background);
	}
	int w = rect.width(), h = rect.height();
	QImage cf = tv.currentImage();
	painter->drawImage((w - cf.width()) / 2, (h - cf.height()) / 2 - 40, cf);
	painter->drawImage((w - me.width()) / 2, (h - me.height()) / 2 + 40, me);
}

void ARenderPrivate::drawDanm(QPainter *painter, QRect)
{
	qint64 time = last.isNull() ? 0 : last.elapsed();
	if (APlayer::instance()->getState() == APlayer::Play)
		last.start();
	Danmaku::instance()->draw(painter, time);
}

void ARenderPrivate::drawTime(QPainter *painter, QRect rect)
{
	if (time <= 0){
		return;
	}
	rect = QRect(0, rect.height() - 2, rect.width()*time, 2);
	QLinearGradient gradient;
	gradient.setStart(rect.center().x(), rect.top());
	gradient.setFinalStop(rect.center().x(), rect.bottom());
	QColor outline = qApp->palette().background().color().darker(140);
	QColor highlight = qApp->palette().color(QPalette::Highlight);
	QColor highlightedoutline = highlight.darker(140);
	if (qGray(outline.rgb()) > qGray(highlightedoutline.rgb())){
		outline = highlightedoutline;
	}
	painter->setPen(QPen(outline));
	gradient.setColorAt(0, highlight);
	gradient.setColorAt(1, highlight.lighter(130));
	painter->setBrush(gradient);
	painter->drawRect(rect);
}
