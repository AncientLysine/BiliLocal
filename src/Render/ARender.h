/*=======================================================================
*
*   Copyright (C) 2013-2015 Lysine.
*
*   Filename:    ARender.h
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

#pragma once

#include <QtGui>
#include <QtCore>

class ARenderPrivate;
class Comment;
class ISpirit;

class ARender :public QObject
{
	Q_OBJECT
public:
	static QStringList getModules();

	virtual ~ARender();
	static ARender *instance();

protected:
	static ARender *ins;
	ARenderPrivate *const d_ptr;
	Q_DECLARE_PRIVATE(ARender);
	ARender(ARenderPrivate *data, QObject *parent = 0);

public slots:
	virtual QList<quint8 *> getBuffer();
	virtual void releaseBuffer();
	virtual void setBuffer(QString &chroma, QSize size, QList<QSize> *bufferSize = 0);
	virtual ISpirit *getSpirit(const QImage &) = 0;

	void setBackground(QString path);
	void setMusic(bool music);
	void setDisplayTime(double t);
	void setVideoAspectRatio(double ratio);
	void setPixelAspectRatio(double ratio);
	void draw();
	virtual quintptr getHandle() = 0;
	virtual void resize(QSize size) = 0;
	virtual QSize getBufferSize() = 0;
	virtual QSize getActualSize() = 0;
	virtual	QSize getPreferSize();
	virtual void setPreferSize(QSize size);
	virtual void draw(QRect rect) = 0;
};
