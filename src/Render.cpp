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

Render *Render::ins=NULL;

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
		//f.insert("NV16",AV_PIX_FMT_NV16);
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
		//f.insert("XY12",AV_PIX_FMT_XYZ12);
	}
	chroma=chroma.toUpper();
	if(!f.contains(chroma)){
		if(chroma=="NV61"){
			chroma="NV16";
		}
		else if(chroma=="YV12"||
				chroma=="IYUV"){
			chroma="I420";
		}
		else if(chroma=="UYNV"||
				chroma=="UYNY"||
				chroma=="Y422"||
				chroma=="HDYC"||
				chroma=="AVUI"||
				chroma=="UYV1"||
				chroma=="2VUY"||
				chroma=="2VU1"){
			chroma="UYVY";
		}
		else if(chroma=="VYUY"||
				chroma=="YUYV"||
				chroma=="YUNV"||
				chroma=="V422"||
				chroma=="YVYU"||
				chroma=="Y211"||
				chroma=="CYUV"){
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
		ins=this;
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
	static QMutex dataLock;

public slots:
	QList<quint8 *> getBuffer()
	{
		dataLock.lock();
		QList<quint8 *> p;
		for(int i=0;i<8;++i){
			if(srcFrame->linesize[i]==0){
				break;
			}
			p.append(srcFrame->data[i]);
		}
		return p;
	}

	void setBuffer(QString &chroma,QSize size,QList<QSize> *bufferSize)
	{
		srcSize=size;
		if(srcFrame){
			avpicture_free(srcFrame);
		}
		else{
			srcFrame=new AVPicture;
		}
		srcFormat=getFormat(chroma);
		avpicture_alloc(srcFrame,srcFormat,srcSize.width(),srcSize.height());
		if (bufferSize){
			bufferSize->clear();
		}
		for(int i=0;i<3;++i){
			int l;
			if((l=srcFrame->linesize[i])==0){
				break;
			}
			if (bufferSize){
				bufferSize->append(QSize(l,srcSize.height()));
			}
		}
	}

	void releaseBuffer()
	{
		dirty=true;
		dataLock.unlock();
	}

	QSize getPreferredSize()
	{
		if(music){
			return QSize();
		}
		QSize s=srcSize;
		pixelAspectRatio>1?(s.rwidth()*=pixelAspectRatio):(s.rheight()/=pixelAspectRatio);
		return s;
	}

	void draw(QRect rect)
	{
		if(rect.isValid()){
			widget->update(rect);
		}
		else{
			widget->update();
		}
	}

	void drawBuffer(QPainter *painter,QRect rect)
	{
		if (srcSize.isEmpty()){
			return;
		}
		QRect dest=fitRect(srcSize,rect);
		dataLock.lock();
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
					  0, srcSize.height(),
					  dstFrame->data,dstFrame->linesize);
			dirty=false;
		}
		dataLock.unlock();
		painter->drawImage(dest,frame);
	}
};

QMutex RasterRender::dataLock;
#endif

#ifdef RENDER_OPENGL
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

#ifdef QT_OPENGL_ES_2
static const char *vShaderCode=
		"attribute vec4 VtxCoord;\n"
		"attribute vec2 TexCoord;\n"
		"varying vec2 TexCoordOut;\n"
		"void main(void)\n"
		"{\n"
		"  gl_Position = VtxCoord;\n"
		"  TexCoordOut = TexCoord;\n"
		"}\n";

static const char *fShaderCode=
		"varying lowp vec2 TexCoordOut;\n"
		"uniform sampler2D SamplerY;\n"
		"uniform sampler2D SamplerU;\n"
		"uniform sampler2D SamplerV;\n"
		"void main(void)\n"
		"{\n"
		"    mediump vec3 yuv;\n"
		"    lowp vec3 rgb;\n"
		"    yuv.x = texture2D(SamplerY, TexCoordOut).r - 0.0625;\n"
		"    yuv.y = texture2D(SamplerU, TexCoordOut).r - 0.5;   \n"
		"    yuv.z = texture2D(SamplerV, TexCoordOut).r - 0.5;   \n"
		"    rgb = mat3(1.164,  1.164, 1.164,   \n"
		"               0,     -0.391, 2.018,   \n"
		"               1.596, -0.813, 0) * yuv;\n"
		"    gl_FragColor = vec4(rgb, 1);\n"
		"}";
#else
static const char *vShaderCode=
		"#version 120\n"
		"attribute vec4 VtxCoord;\n"
		"attribute vec2 TexCoord;\n"
		"varying vec2 TexCoordOut;\n"
		"void main(void)\n"
		"{\n"
		"  gl_Position = VtxCoord;\n"
		"  TexCoordOut = TexCoord;\n"
		"}\n";

static const char *fShaderCode=
		"#version 120\n"
		"varying vec2 TexCoordOut;\n"
		"uniform sampler2D SamplerY;\n"
		"uniform sampler2D SamplerU;\n"
		"uniform sampler2D SamplerV;\n"
		"void main(void)\n"
		"{\n"
		"    vec3 yuv;\n"
		"    vec3 rgb;\n"
		"    yuv.x = texture2D(SamplerY, TexCoordOut).r - 0.0625;\n"
		"    yuv.y = texture2D(SamplerU, TexCoordOut).r - 0.5;   \n"
		"    yuv.z = texture2D(SamplerV, TexCoordOut).r - 0.5;   \n"
		"    rgb = mat3(1.164,  1.164, 1.164,   \n"
		"               0,     -0.391, 2.018,   \n"
		"               1.596, -0.813, 0) * yuv;\n"
		"    gl_FragColor = vec4(rgb, 1);\n"
		"}";
#endif

class OpenGLRender:public Render,protected QOpenGLFunctions
{
public:
	explicit OpenGLRender(QWidget *parent=0):
		Render(parent)
	{
		window=new Window(this,parent);
		widget=QWidget::createWindowContainer(window,parent);
		initialize=true;
		vShader=fShader=program=0;
		ins=this;
	}

	~OpenGLRender()
	{
		if(!initialize){
			glDeleteShader(vShader);
			glDeleteShader(fShader);
			glDeleteProgram(program);
			glDeleteTextures(3,frame);
		}
		for(quint8 *iter:buffer){
			delete[]iter;
		}
		delete window;
	}

private:
	Window *window;
	QSize inner;
	bool initialize;
	bool UVReverted;
	GLuint vShader;
	GLuint fShader;
	GLuint program;
	GLuint frame[3];
	static QMutex dataLock;
	QList<quint8 *> buffer;

	void uploadTexture(int i,int w,int h)
	{
		glActiveTexture(GL_TEXTURE0+i);
		glBindTexture(GL_TEXTURE_2D,frame[i]);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
		glTexImage2D(GL_TEXTURE_2D,0,GL_LUMINANCE,w,h,0,GL_LUMINANCE,GL_UNSIGNED_BYTE,buffer[i]);
	}

public slots:
	QList<quint8 *> getBuffer()
	{
		dataLock.lock();
		return buffer;
	}

	void releaseBuffer()
	{
		dirty=true;
		dataLock.unlock();
	}

	void setBuffer(QString &chroma,QSize size,QList<QSize> *bufferSize)
	{
		if(chroma=="YV12"){
			UVReverted=1;
		}
		else{
			UVReverted=0;
			chroma="I420";
		}
		inner=size;
		int w=size.width(),h=size.height();
		for(quint8 *iter:buffer){
			delete[]iter;
		}
		buffer.clear();
		buffer.append(new quint8[w*h]);
		buffer.append(new quint8[w*h/4]);
		buffer.append(new quint8[w*h/4]);
		if (bufferSize){
			bufferSize->clear();
			bufferSize->append(size);
			bufferSize->append(size/2);
			bufferSize->append(size/2);
		}
	}

	QSize getPreferredSize()
	{
		if(music){
			return QSize();
		}
		QSize s=inner;
		pixelAspectRatio>1?(s.rwidth()*=pixelAspectRatio):(s.rheight()/=pixelAspectRatio);
		return s;
	}

	void draw(QRect)
	{
		window->draw();
	}

	void drawBuffer(QPainter *painter,QRect rect)
	{
		if (inner.isEmpty()){
			return;
		}
		QRect dest=fitRect(inner,rect);
		dataLock.lock();
		painter->beginNativePainting();
		if(dirty){
			if(initialize){
				initializeOpenGLFunctions();
				glGenTextures(3,frame);
				vShader=glCreateShader(GL_VERTEX_SHADER);
				fShader=glCreateShader(GL_FRAGMENT_SHADER);
				glShaderSource(vShader,1,&vShaderCode,NULL);
				glShaderSource(fShader,1,&fShaderCode,NULL);
				glCompileShader(vShader);
				glCompileShader(fShader);
				program=glCreateProgram();
				glAttachShader(program,vShader);
				glAttachShader(program,fShader);
				glBindAttribLocation(program,0,"VtxCoord");
				glBindAttribLocation(program,1,"TexCoord");
				glLinkProgram(program);
				initialize=false;
			}
			int w=inner.width(),h=inner.height();
			uploadTexture(0,w,h);
			uploadTexture(1,w/2,h/2);
			uploadTexture(2,w/2,h/2);
			dirty=false;
		}
		dataLock.unlock();
		glUseProgram(program);
		GLfloat h=dest.width()/(GLfloat)rect.width(),v=dest.height()/(GLfloat)rect.height();
		GLfloat vtx[8]={
			-h,-v,
			+h,-v,
			-h,+v,
			+h,+v
		};
		GLfloat tex[8]={
			0,1,
			1,1,
			0,0,
			1,0
		};
		glVertexAttribPointer(0,2,GL_FLOAT,0,0,vtx);
		glVertexAttribPointer(1,2,GL_FLOAT,0,0,tex);
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D,frame[0]);
		glUniform1i(glGetUniformLocation(program,"SamplerY"),0);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D,frame[UVReverted?2:1]);
		glUniform1i(glGetUniformLocation(program,"SamplerU"),1);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D,frame[UVReverted?1:2]);
		glUniform1i(glGetUniformLocation(program,"SamplerV"),2);
		glDrawArrays(GL_TRIANGLE_STRIP,0,4);
		painter->endNativePainting();
	}
};

QMutex OpenGLRender::dataLock;
#endif

Render *Render::instance(QWidget *parent)
{
	if(ins){
		return ins;
	}
	QString r;
	QStringList l=Utils::getRenderModules();
	switch(l.size()){
	case 0:
		break;
	case 1:
		r=l[0];
		break;
	default:
		r=Config::getValue("/Performance/Render",QString("OpenGL"));
		break;
	}
	if(r=="OpenGL"){
		return new OpenGLRender(parent);
	}
	if(r=="Raster"){
		return new RasterRender(parent);
	}
	return 0;
}

Render::Render(QWidget *parent):
	QObject(parent)
{
	time=0;
	if(Config::getValue("/Interface/Version",true)){
		tv.setFileName(":/Picture/tv.gif");
		tv.start();
		me=QImage(":/Picture/version.png");
		connect(APlayer::instance(),&APlayer::begin,&tv,&QMovie::stop );
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
	sound=QImage(":/Picture/sound.png");
	music=1;
	dirty=0;
	videoAspectRatio=0;
	pixelAspectRatio=1;

	connect(APlayer::instance(),&APlayer::stateChanged,[this](){last=QTime();});
	connect(APlayer::instance(),&APlayer::reach,[this](){
		setRefreshRate(Config::getValue("/Danmaku/Power",60));
	});

	power=new QTimer(this);
	power->setTimerType(Qt::PreciseTimer);
	connect(APlayer::instance(),&APlayer::decode,[this](){
		if(!power->isActive()){
			draw();
		}
	});
	connect(power,&QTimer::timeout,[this](){
		if(APlayer::instance()->getState()==APlayer::Play){
			draw();
		}
	});
	QMetaObject::invokeMethod(this,"setRefreshRate",Qt::QueuedConnection,
							  Q_ARG(int,Config::getValue("/Danmaku/Power",60)));
}

QRect Render::fitRect(QSize size,QRect rect)
{
	QRect dest;
	QSizeF s=videoAspectRatio>0?QSizeF(videoAspectRatio,1):QSizeF(size);
	pixelAspectRatio>1?(s.rwidth()*=pixelAspectRatio):(s.rheight()/=pixelAspectRatio);
	dest.setSize(s.scaled(rect.size(),Qt::KeepAspectRatio).toSize()/4*4);
	dest.moveCenter(rect.center());
	return dest;
}

void Render::setMusic(bool isMusic)
{
	if((music=isMusic)&&!power->isActive()){
		setRefreshRate(60,true);
	}
}

void Render::setRefreshRate(int rate,bool soft)
{
	if(rate){
		rate=qBound(30,rate,200);
		power->start(qRound(1000.0/rate));
		emit refreshRateChanged(rate);
	}
	else{
		rate=0;
		power->stop();
		emit refreshRateChanged(rate);
	}
	if(!soft){
		Config::setValue("/Danmaku/Power",rate);
	}
}

void Render::drawPlay(QPainter *painter,QRect rect)
{
	painter->fillRect(rect,Qt::black);
	if(music){
		painter->drawImage(rect.center()-QRect(QPoint(0,0),sound.size()).center(),sound);
	}
	else{
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
	danmaku->draw(painter,time);
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

void Render::setDisplayTime(double t)
{
	time=t;
	draw(QRect(0,widget->height()-2,widget->width()*time,2));
}
