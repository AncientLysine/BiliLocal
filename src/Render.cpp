/*=======================================================================
*
*   Copyright (C) 2013 Lysine.
*
*   Filename:    Render.cpp
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

#include "Render.h"
#include "VPlayer.h"
#include "Danmaku.h"

Render *Render::ins=NULL;

Render::Render(QWindow *parent):
	QWindow(parent)
{
	device=NULL;
	context=NULL;
	setSurfaceType(QWindow::OpenGLSurface);
	connect(VPlayer::instance(),&VPlayer::stateChanged,[this](){last=QTime();});
	ins=this;
}

void Render::draw()
{
	if(!isExposed()){
		return;
	}
	bool needInitialize=false;
	if(!context){
		context=new QOpenGLContext(this);
		context->create();
		needInitialize=true;
	}
	context->makeCurrent(this);
	if(!device){
		device=new QOpenGLPaintDevice;
	}
	if(needInitialize){
		initializeOpenGLFunctions();
	}
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);
	device->setSize(size());
	QPainter painter(device);
	painter.setRenderHints(QPainter::SmoothPixmapTransform);
	VPlayer *vplayer=VPlayer::instance();
	Danmaku *danmaku=Danmaku::instance();
	QRect rect(QPoint(0,0),size());
	vplayer->draw(&painter,rect);
	qint64 time=0;
	if(!last.isNull()){
		time=last.elapsed();
	}
	if(vplayer->getState()==VPlayer::Play){
		last.start();
	}
	danmaku->draw(&painter,rect,time);
	context->swapBuffers(this);
}
