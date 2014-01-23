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

Render::Render(QWidget *parent):
	tv(":/Picture/tv.gif"),me(":/Picture/version.png"),parent(parent)
{
	device=NULL;
	context=NULL;
	tv.start();
	setSurfaceType(QWindow::OpenGLSurface);
	connect(VPlayer::instance(),&VPlayer::stateChanged,[this](){last=QTime();});
	connect(VPlayer::instance(),&VPlayer::begin,&tv,&QMovie::stop);
	connect(VPlayer::instance(),&VPlayer::reach,&tv,&QMovie::start);
	connect(&tv,&QMovie::updated,this,&Render::draw);
	widget=QWidget::createWindowContainer(this,parent);
	QString path=Utils::getConfig("/Interface/Background",QString());
	if(!path.isEmpty()){
		background=QImage(path);
	}
}

bool Render::event(QEvent *e)
{
	switch(e->type()){
	case QEvent::Drop:
	case QEvent::KeyPress:
	case QEvent::KeyRelease:
	case QEvent::Enter:
	case QEvent::Leave:
	case QEvent::MouseMove:
	case QEvent::MouseButtonPress:
	case QEvent::MouseButtonRelease:
	case QEvent::MouseButtonDblClick:
	case QEvent::Wheel:
	case QEvent::DragEnter:
	case QEvent::DragMove:
	case QEvent::DragLeave:
	case QEvent::ContextMenu:
		if(parent){
			QBackingStore *backing=parent->backingStore();
			if(backing){
				QWindow *window=backing->window();
				if(window){
					return qApp->sendEvent(window,e);
				}
			}
		}
		return false;
	case QEvent::Expose:
		draw();
		return false;
	default:
		return QWindow::event(e);
	}
}

void Render::drawPlay(QPainter *painter, QRect rect)
{
	VPlayer *vplayer=VPlayer::instance();
	Danmaku *danmaku=Danmaku::instance();
	vplayer->draw(painter,rect);
	qint64 time=0;
	if(!last.isNull()){
		time=last.elapsed();
	}
	if(vplayer->getState()==VPlayer::Play){
		last.start();
	}
	danmaku->draw(painter,rect,time);
}

void Render::drawStop(QPainter *painter,QRect rect)
{
	if(background.isNull()){
		painter->fillRect(rect,qApp->palette().color(QPalette::Window));
	}
	else{
		QRect dest=rect;
		dest.setSize(background.size().scaled(dest.size(),Qt::KeepAspectRatioByExpanding));
		dest.moveCenter(rect.center());
		painter->drawImage(dest,background);
	}
	int w=rect.width(),h=rect.height();
	QImage cf=tv.currentImage();
	painter->drawImage((w-cf.width())/2,(h-cf.height())/2-40,cf);
	painter->drawImage((w-me.width())/2,(h-me.height())/2+40,me);
}

void Render::drawTime(QPainter *painter,QRect rect)
{
	;
}

void Render::draw()
{
	if(!isExposed()){
		return;
	}
	bool initialize=false;
	if(!context){
		context=new QOpenGLContext(this);
		context->create();
		initialize=true;
	}
	context->makeCurrent(this);
	if(!device){
		device=new QOpenGLPaintDevice;
	}
	if(initialize){
		glEnable(GL_TEXTURE_2D);
		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	}
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);
	device->setSize(size());
	QPainter painter(device);
	painter.setRenderHints(QPainter::SmoothPixmapTransform);
	QRect rect(QPoint(0,0),size());
	if(VPlayer::instance()->getState()==VPlayer::Stop){
		drawStop(&painter,rect);
	}
	else{
		drawPlay(&painter,rect);
	}
	if(Utils::getConfig("/Playing/Slider",true)){
		drawTime(&painter,rect);
	}
	context->swapBuffers(this);
}
