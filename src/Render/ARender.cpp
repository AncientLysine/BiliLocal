/*=======================================================================
*
*   Copyright (C) 2013-2016 Lysine.
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

#include "Common.h"
#include "ARender.h"
#include "ARenderPrivate.h"
#include "../Config.h"
#include "../Local.h"
#include "../Define/Comment.h"
#include "../Model/Running.h"
#include "../Player/APlayer.h"

#ifdef RENDER_OPENGL
#include "OpenGL/OpenGLRender.h"
#endif
#ifdef RENDER_RASTER
#include "Raster/RasterRender.h"
#endif

QStringList ARender::getModules()
{
	QStringList modules;
#ifdef RENDER_OPENGL
	modules << "OpenGL";
#endif
#ifdef RENDER_RASTER
	modules << "Raster";
#endif
	return modules;
}

ARender *ARender::create(QObject *parent, QString name)
{
	if (name.isEmpty()) {
		QStringList l = getModules();
		switch (l.size()) {
		case 0:
			break;
		case 1:
			name = l[0];
			break;
		default:
			name = Config::getValue("/Render/Type", l[0]);
			name = l.contains(name) ? name : l[0];
			break;
		}
	}
#ifdef RENDER_OPENGL
	if (name == "OpenGL") {
		return new OpenGLRender(parent);
	}
#endif
#ifdef RENDER_RASTER
	if (name == "Raster") {
		return new RasterRender(parent);
	}
#endif
	return nullptr;
}

ARender::ARender(QObject *parent)
	: QObject(parent), d_ptr(nullptr)
{
	setObjectName("ARender");
}

void ARender::setup()
{
	Q_D(ARender);
	d->time = 0;
	if (Config::getValue("/Interface/Version", true)){
		d->tv.setFileName(":/Picture/tv.gif");
		d->tv.setCacheMode(QMovie::CacheAll);
		d->tv.start();
		d->me = QImage(":/Picture/version.png");
		connect(lApp->findObject<APlayer>(), &APlayer::begin, &d->tv, &QMovie::stop);
		connect(lApp->findObject<APlayer>(), &APlayer::reach, &d->tv, &QMovie::start);
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
	auto reset = [this]() { setDisplayTime(0); };
	connect(lApp->findObject<APlayer>(), &APlayer::begin, this, reset);
	connect(lApp->findObject<APlayer>(), &APlayer::reach, this, reset);
	connect(lApp->findObject<APlayer>(), &APlayer::stateChanged, this, [d](){
		d->timer.invalidate();
	});
}

ARender::~ARender()
{
	delete d_ptr;
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

void ARender::draw()
{
	QRect rect;
	rect.setSize(getActualSize());
	draw(rect);
}

void ARender::draw(QRect rect)
{
	draw(rect.isValid() ? rect : QRect(QPoint(0, 0), getActualSize()));
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
	clear(painter, Qt::black);
	if (music){
		painter->drawImage(rect.center() - QRect(QPoint(0, 0), sound.size()).center(), sound);
	}
	else{
		drawData(painter, rect);
	}
	drawDanm(painter, rect);
	drawTime(painter, rect);
#ifdef GRAPHIC_DEBUG
	auto time = QTime::currentTime();
	static QTime last = time;
	static int count = 0;
	static int speed = 0;
	static int frame = 0;
	if (last.second() != time.second()) {
		last = time;
		count = lApp->findObject<Running>()->size();
		speed = frame;
		frame = 0;
	}
	++frame;
	QFont font;
	font.setPixelSize(20);
	painter->setFont(font);
	painter->setPen(Qt::yellow);
	QRect info(0, 0, 150, 60);
	painter->drawText(info, QString("Count: %1\nFrame: %2").arg(count).arg(speed));
#endif
}

void ARenderPrivate::drawStop(QPainter *painter, QRect rect)
{
	if (background.isNull()){
		clear(painter, qApp->palette().color(QPalette::Window));
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

void ARenderPrivate::drawDanm(QPainter *painter, QRect)
{
	lApp->findObject<Running>()->draw(painter, timer.step());
}
