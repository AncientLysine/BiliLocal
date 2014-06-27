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
#include "Config.h"
#include "APlayer.h"
#include "Danmaku.h"

#ifdef RENDER_RASTER
extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

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
		if(APlayer::instance()->getState()==APlayer::Stop){
			render->drawStop(&painter,rect);
		}
		else{
			render->drawPlay(&painter,rect);
			render->drawTime(&painter,rect);
		}
		QWidget::paintEvent(e);
	}
};

int avpicture_alloc(AVPicture *picture,enum AVPixelFormat pix_fmt,int width,int height)
{
	int ret=av_image_alloc(picture->data,picture->linesize,width,height,pix_fmt,1);
	if(ret<0){
		memset(picture,0,sizeof(AVPicture));
		return ret;
	}
	return 0;
}

void avpicture_free(AVPicture *picture)
{
	av_free(picture->data[0]);
}

static AVPixelFormat getFormat(QString &chroma)
{
	static QHash<QString,AVPixelFormat> f;
	if(f.isEmpty()){
		f.insert("RV32",AV_PIX_FMT_RGB32);
		f.insert("I410",AV_PIX_FMT_YUV410P);
		f.insert("I411",AV_PIX_FMT_YUV411P);
		f.insert("I420",AV_PIX_FMT_YUV420P);
		f.insert("I422",AV_PIX_FMT_YUV422P);
		f.insert("I440",AV_PIX_FMT_YUV440P);
		f.insert("I444",AV_PIX_FMT_YUV444P);
		f.insert("J420",AV_PIX_FMT_YUVJ420P);
		f.insert("J422",AV_PIX_FMT_YUVJ422P);
		f.insert("J440",AV_PIX_FMT_YUVJ440P);
		f.insert("J444",AV_PIX_FMT_YUVJ444P);
		f.insert("I40A",AV_PIX_FMT_YUVA420P);
		f.insert("I42A",AV_PIX_FMT_YUVA422P);
		f.insert("YUVA",AV_PIX_FMT_YUVA444P);
		f.insert("NV12",AV_PIX_FMT_NV12);
		f.insert("NV21",AV_PIX_FMT_NV21);
		f.insert("NV16",AV_PIX_FMT_NV16);
		f.insert("I09L",AV_PIX_FMT_YUV420P9LE);
		f.insert("I09B",AV_PIX_FMT_YUV420P9BE);
		f.insert("I29L",AV_PIX_FMT_YUV422P9LE);
		f.insert("I29B",AV_PIX_FMT_YUV422P9BE);
		f.insert("I49L",AV_PIX_FMT_YUV444P9LE);
		f.insert("I49B",AV_PIX_FMT_YUV444P9BE);
		f.insert("I0AL",AV_PIX_FMT_YUV420P10LE);
		f.insert("I0AB",AV_PIX_FMT_YUV420P10BE);
		f.insert("I2AL",AV_PIX_FMT_YUV422P10LE);
		f.insert("I2AB",AV_PIX_FMT_YUV422P10BE);
		f.insert("I4AL",AV_PIX_FMT_YUV444P10LE);
		f.insert("I4AB",AV_PIX_FMT_YUV444P10BE);
		f.insert("UYVY",AV_PIX_FMT_UYVY422);
		f.insert("YUY2",AV_PIX_FMT_YUYV422);
		f.insert("XY12",AV_PIX_FMT_XYZ12);
	}
	chroma=chroma.toUpper();
	if(!f.contains(chroma)){
		if(c=="NV61"){
			chroma="NV16";
		}
		else if(c=="YV12"||
				c=="IYUV"){
			chroma="I420";
		}
		else if(c=="UYNV"||
				c=="UYNY"||
				c=="Y422"||
				c=="HDYC"||
				c=="AVUI"||
				c=="UYV1"||
				c=="2VUY"||
				c=="2VU1"){
			chroma="UYVY";
		}
		else if(c=="VYUY"||
				c=="YUYV"||
				c=="YUNV"||
				c=="V422"||
				c=="YVYU"||
				c=="Y211"||
				c=="CYUV"){
			chroma="YUY2";
		}
		else{
			chroma="RV32";
		}
	}
	return f[chroma];
}

class RasterRender:public Render
{
public:
	explicit RasterRender(QWidget *parent=0):
		Render(parent)
	{
		widget=new Widget(this,parent);
		swsctx=NULL;
		srcFrame=NULL;
		dstFrame=NULL;
		dstFormat=AV_PIX_FMT_RGB32;
	}

	~RasterRender()
	{
		if(swsctx){
			sws_freeContext(swsctx);
		}
		if(srcFrame){
			avpicture_free(srcFrame);
			delete srcFrame;
		}
		if(dstFrame){
			avpicture_free(dstFrame);
			delete dstFrame;
		}
	}

private:
	SwsContext *swsctx;
	AVPixelFormat srcFormat;
	AVPixelFormat dstFormat;
	AVPicture *srcFrame;
	AVPicture *dstFrame;
	QSize srcSize;
	QSize dstSize;
	QImage frame;

	void drawBuffer(QPainter *painter,QRect rect)
	{
		QRect dest=getRect(rect);
		data.lock();
		if(dstSize!=dest.size()){
			if(dstFrame){
				avpicture_free(dstFrame);
			}
			else{
				dstFrame=new AVPicture;
			}
			dstSize=dest.size();
			avpicture_alloc(dstFrame,dstFormat,dstSize.width(),dstSize.height());
			frame=QImage(*dstFrame->data,dstSize.width(),dstSize.height(),QImage::Format_RGB32);
			dirty=true;
		}
		if(dirty){
			swsctx=sws_getCachedContext(swsctx,
										srcSize.width(),srcSize.height(),srcFormat,
										dstSize.width(),dstSize.height(),dstFormat,
										SWS_FAST_BILINEAR,NULL,NULL,NULL);
			sws_scale(swsctx,
					  srcFrame->data,srcFrame->linesize,
					  0,srcSize.height(),
					  dstFrame->data,dstFrame->linesize);
			dirty=false;
		}
		data.unlock();
		painter->drawImage(dest,frame);
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
#endif

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

	~Window()
	{
		if(device){
			delete device;
		}
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
		}
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);
		device->setSize(size());
		QPainter painter(device);
		painter.setRenderHints(QPainter::SmoothPixmapTransform);
		QRect rect(QPoint(0,0),size());
		if(APlayer::instance()->getState()==APlayer::Stop){
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
	if(Config::getValue("/Interface/Accelerated",false)){
		return new OpenGLRender(parent);
	}
	else{
		return new RasterRender(parent);
	}
}

Render::Render(QWidget *parent):
	QObject(parent)
{
	time=0;
	connect(APlayer::instance(),&APlayer::stateChanged,[this](){last=QTime();});
	if(Config::getValue("/Interface/Version",true)){
		tv.setFileName(":/Picture/tv.gif");
		tv.start();
		me=QImage(":/Picture/version.png");
		connect(APlayer::instance(),&APlayer::begin,&tv,&QMovie::stop);
		connect(APlayer::instance(),&APlayer::reach,&tv,&QMovie::start);
		connect(&tv,&QMovie::updated,[this](){
			QImage cf=tv.currentImage();
			QPoint ps=widget->rect().center()-cf.rect().center();
			ps.ry()-=40;
			draw(QRect(ps,cf.size()));
		});
	}
	QString path=Config::getValue("/Interface/Background",QString());
	if(!path.isEmpty()){
		background=QImage(path);
	}
	sound=QImage("/Picture/sound.png");
	start=music=dirty=false;
}

void Render::drawPlay(QPainter *painter,QRect rect)
{
	painter->fillRect(rect,Qt::black);
	if(music){
		painter->drawImage(rect.center()-QRect(QPoint(0,0),sound.size()).center(),sound);
	}
	else if(start){
		drawBuffer(painter,rect);
	}
	APlayer *aplayer=APlayer::instance();
	Danmaku *danmaku=Danmaku::instance();
	qint64 time=0;
	if(!last.isNull()){
		time=last.elapsed();
	}
	if(aplayer->getState()==APlayer::Play){
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
