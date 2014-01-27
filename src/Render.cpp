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

class Widget:public QWidget
{
public:
	explicit Widget(Render *render,QWidget *parent=0):
		QWidget(parent),render(render)
	{
		setAttribute(Qt::WA_TransparentForMouseEvents);
		setFocusPolicy(Qt::NoFocus);
	}

private:
	Render *render;
	void paintEvent(QPaintEvent *e)
	{
		QPainter painter(this);
		QRect rect(QPoint(0,0),size());
		painter.setRenderHints(QPainter::SmoothPixmapTransform);
		if(VPlayer::instance()->getState()==VPlayer::Stop){
			render->drawStop(&painter,rect);
		}
		else{
			render->drawPlay(&painter,rect);
			render->drawTime(&painter,rect);
		}
		QWidget::paintEvent(e);
	}
};

class RasterRender:public Render
{
public:
	explicit RasterRender(QWidget *parent=0):
		Render(parent)
	{
		widget=new Widget(this,parent);
	}

public slots:
	void draw(QRect rect)
	{
		if(rect.isValid()){
			widget->update(rect);
		}
		else{
			widget->update();
		}
	}
};

class Window:public QWindow
{
public:
	explicit Window(Render *render,QWidget *parent=0):
		render(render),parent(parent)
	{
		device=NULL;
		context=NULL;
		setSurfaceType(QWindow::OpenGLSurface);
	}

	void draw()
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
		painter.setRenderHints(QPainter::SmoothPixmapTransform|QPainter::HighQualityAntialiasing);
		QRect rect(QPoint(0,0),size());
		if(VPlayer::instance()->getState()==VPlayer::Stop){
			render->drawStop(&painter,rect);
		}
		else{
			render->drawPlay(&painter,rect);
			render->drawTime(&painter,rect);
		}
		context->swapBuffers(this);
	}

private:
	Render *render;
	QWidget *parent;
	QOpenGLContext *context;
	QOpenGLPaintDevice *device;
	bool event(QEvent *e)
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
};

class OpenGLRender:public Render
{
public:
	explicit OpenGLRender(QWidget *parent=0):
		Render(parent)
	{
		window=new Window(this,parent);
		widget=QWidget::createWindowContainer(window,parent);
	}

	~OpenGLRender()
	{
		delete window;
	}

private:
	Window *window;

public slots:
	void draw(QRect){window->draw();}
};

Render *Render::create(QWidget *parent)
{
	if(Utils::getConfig("/Interface/Accelerated",false)){
		return new OpenGLRender(parent);
	}
	else{
		return new RasterRender(parent);
	}
}

Render::Render(QWidget *parent):
	QObject(parent),tv(":/Picture/tv.gif"),me(":/Picture/version.png")
{
	time=0;
	tv.start();
	connect(VPlayer::instance(),&VPlayer::stateChanged,[this](){last=QTime();});
	connect(VPlayer::instance(),&VPlayer::begin,&tv,&QMovie::stop);
	connect(VPlayer::instance(),&VPlayer::reach,&tv,&QMovie::start);
	connect(&tv,&QMovie::updated,[this](){
		QImage cf=tv.currentImage();
		QPoint ps=widget->rect().center()-cf.rect().center();
		ps.ry()-=40;
		draw(QRect(ps,cf.size()));
	});
	QString path=Utils::getConfig("/Interface/Background",QString());
	if(!path.isEmpty()){
		background=QImage(path);
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
	if(time<=0){
		return;
	}
	rect=QRect(0,rect.height()-2,rect.width()*time,2);
	QLinearGradient gradient;
	gradient.setStart(rect.center().x(),rect.top());
	gradient.setFinalStop(rect.center().x(),rect.bottom());
	QColor outline=qApp->palette().background().color().darker(140);
	QColor highlight=qApp->palette().color(QPalette::Highlight);
	QColor highlightedoutline=highlight.darker(140);
	if(qGray(outline.rgb())>qGray(highlightedoutline.rgb())){
		outline=highlightedoutline;
	}
	painter->setPen(QPen(outline));
	gradient.setColorAt(0,highlight);
	gradient.setColorAt(1,highlight.lighter(130));
	painter->setBrush(gradient);
	painter->drawRect(rect);
}

void Render::setTime(double t)
{
	time=t;
	draw(QRect(0,widget->height()-2,widget->width()*time,2));
}
