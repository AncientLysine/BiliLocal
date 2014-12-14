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
#include "Local.h"
#include <QtWidgets>

Render *Render::ins=NULL;

class RenderPrivate
{
public:
	QMovie tv;
	double time;
	QImage me,background,sound;
	QTime last;
	QTimer *power;
	bool music;
	bool dirty;
	double videoAspectRatio;
	double pixelAspectRatio;

	virtual ~RenderPrivate()
	{
	}

	QRect fitRect(QSize size,QRect rect)
	{
		QRect dest;
		QSizeF s=videoAspectRatio>0?QSizeF(videoAspectRatio,1):QSizeF(size);
		pixelAspectRatio>1?(s.rwidth()*=pixelAspectRatio):(s.rheight()/=pixelAspectRatio);
		dest.setSize(s.scaled(rect.size(),Qt::KeepAspectRatio).toSize()/4*4);
		dest.moveCenter(rect.center());
		return dest;
	}

	void drawPlay(QPainter *painter,QRect rect)
	{
		painter->fillRect(rect,Qt::black);
		if(music){
			painter->drawImage(rect.center()-QRect(QPoint(0,0),sound.size()).center(),sound);
		}
		else{
			drawData(painter,rect);
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

	void drawStop(QPainter *painter,QRect rect)
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

	void drawTime(QPainter *painter,QRect rect)
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

	virtual void drawData(QPainter *painter,QRect rect)=0;
	virtual QList<quint8 *> getBuffer()=0;
	virtual void releaseBuffer()=0;
	virtual void setBuffer(QString &chroma,QSize size,QList<QSize> *bufferSize=0)=0;

};

#ifdef RENDER_RASTER
extern "C"
{
#include <libavutil/mem.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

class RasterRenderPrivate:public RenderPrivate
{
public:
	class Buffer
	{
	public:
		quint8 *data[4];
		qint32 width[4];
		qint32 lines[4];
		AVPixelFormat format;
		QSize size;

		Buffer(AVPixelFormat format,QSize size):
			format(AV_PIX_FMT_NONE), size(size)
		{
			memset(data,0,sizeof(data[0])*4);
			if (av_image_fill_linesizes(width,format,size.width())<0)
				return;
			const AVPixFmtDescriptor *desc=av_pix_fmt_desc_get(format);
			if(!desc||desc->flags&PIX_FMT_HWACCEL)
				return;
			int i,p[4]={0};
			for(i=0;i<4;i++)
				p[desc->comp[i].plane]=1;
			lines[0]=size.height();
			int n=width[0]*lines[0];
			for(i=1;i<4&&p[i];i++){
				int s=(i==1||i==2)?desc->log2_chroma_h:0;
				lines[i]=(size.height()+(1<<s)-1)>>s;
				n+=width[i]*lines[i];
			}
			data[0]=new quint8[n]; 
			for(i=1;i<4&&p[i];i++){
				data[i]=data[i-1]+width[i-1]*lines[i-1];
			}
			this->format=format;
		}

		bool isValid()
		{
			return format!=AV_PIX_FMT_NONE;
		}

		~Buffer()
		{
			if(isValid()){
				delete data[0];
			}
		}
	};

	SwsContext *swsctx;
	Buffer *srcFrame;
	Buffer *dstFrame;
	QImage frame;
	static QMutex dataLock;

	RasterRenderPrivate()
	{
		swsctx=NULL;
		srcFrame=NULL;
		dstFrame=NULL;
	}

	AVPixelFormat getFormat(QString &chroma)
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
		}
		chroma=chroma.toUpper();
		if(f.contains(chroma)){
			//Recognized
		}
		else if(chroma=="YV12"||
				chroma=="IYUV"){
			chroma="I420";
		}
		else if(chroma=="VYUY"||
				chroma=="YUYV"||
				chroma=="YUY2"||
				chroma=="YVYU"||
				chroma=="V422"||
				chroma=="YVYU"||
				chroma=="Y211"||
				chroma=="CYUV"){
			chroma="UYVY";
		}
		else if(chroma=="V210"||
				chroma=="NV16"||
				chroma=="NV61"){
			chroma="NV12";
		}
		else{
			chroma="RV32";
		}
		return f[chroma];
	}

	void drawData(QPainter *painter,QRect rect)
	{
		if(!srcFrame->isValid()){
			return;
		}
		QRect dest=fitRect(srcFrame->size,rect);
		if(!dstFrame||dstFrame->size!=dest.size()){
			if(dstFrame){
				delete dstFrame;
			}
			QSize dstSize=dest.size();
			dstFrame=new Buffer(AV_PIX_FMT_RGB32,dstSize);
			frame=QImage(*dstFrame->data,dstSize.width(),dstSize.height(),QImage::Format_RGB32);
			dirty=true;
		}
		if(dirty){
			swsctx=sws_getCachedContext(swsctx,
										srcFrame->size.width(),srcFrame->size.height(),srcFrame->format,
										dstFrame->size.width(),dstFrame->size.height(),dstFrame->format,
										SWS_FAST_BILINEAR,NULL,NULL,NULL);
			dataLock.lock();
			sws_scale(swsctx,
					  srcFrame->data,srcFrame->width,
					  0,srcFrame->size.height(),
					  dstFrame->data,dstFrame->width);
			dirty=false;
			dataLock.unlock();
		}
		painter->drawImage(dest,frame);
	}

	QList<quint8 *> getBuffer()
	{
		dataLock.lock();
		QList<quint8 *> p;
		for(int i=0;i<4;++i){
			if(srcFrame->width[i]==0){
				break;
			}
			p.append(srcFrame->data[i]);
		}
		return p;
	}

	void setBuffer(QString &chroma,QSize size,QList<QSize> *bufferSize)
	{
		if(srcFrame){
			delete srcFrame;
		}
		srcFrame=new Buffer(getFormat(chroma),size);
		if (bufferSize){
			bufferSize->clear();
		}
		for(int i=0;i<4;++i){
			int l;
			if((l=srcFrame->width[i])==0){
				break;
			}
			if (bufferSize){
				bufferSize->append(QSize(srcFrame->width[i],srcFrame->lines[i]));
			}
		}
	}

	void releaseBuffer()
	{
		dirty=true;
		dataLock.unlock();
	}

	virtual ~RasterRenderPrivate()
	{
		if(swsctx){
			sws_freeContext(swsctx);
		}
		if(srcFrame){
			delete srcFrame;
		}
		if(dstFrame){
			delete dstFrame;
		}
	}
};

QMutex RasterRenderPrivate::dataLock;

class RWidget:public QWidget
{
public:
	RWidget(RasterRenderPrivate *render):
		QWidget(lApp->mainWidget()),render(render)
	{
		setAttribute(Qt::WA_TransparentForMouseEvents);
	}

private:
	RasterRenderPrivate *render;

	void paintEvent(QPaintEvent *e)
	{
		QPainter painter(this);
		QRect rect(QPoint(0,0),Render::instance()->getActualSize());
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

class RasterRender:public Render
{
public:
	RasterRender(QObject *parent=0):
		Render(new RasterRenderPrivate,parent)
	{
		Q_D(RasterRender);
		ins=this;
		setObjectName("RRender");
		widget=new RWidget(d);
	}

private:
	RWidget *widget;
	Q_DECLARE_PRIVATE(RasterRender)

public slots:
	quintptr getHandle()
	{
		return (quintptr)widget;
	}

	void resize(QSize size)
	{
		widget->resize(size);
	}

	QSize getActualSize()
	{
		return widget->size();
	}

	QSize getBufferSize()
	{
		Q_D(RasterRender);
		return d->srcFrame->size;
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
};
#endif

#ifdef RENDER_OPENGL
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
		"varying highp vec2 TexCoordOut;\n"
		"uniform sampler2D SamplerY;\n"
		"uniform sampler2D SamplerU;\n"
		"uniform sampler2D SamplerV;\n"
		"void main(void)\n"
		"{\n"
		"    mediump vec3 yuv;\n"
		"    mediump vec3 rgb;\n"
		"    yuv.x = texture2D(SamplerY, TexCoordOut).r - 0.0625;\n"
		"    yuv.y = texture2D(SamplerU, TexCoordOut).r - 0.5;   \n"
		"    yuv.z = texture2D(SamplerV, TexCoordOut).r - 0.5;   \n"
		"    rgb = mat3(1.164,  1.164, 1.164,   \n"
		"               0,     -0.391, 2.018,   \n"
		"               1.596, -0.813, 0) * yuv;\n"
		"    gl_FragColor = vec4(rgb, 1);\n"
		"}";

class OpenGLRenderPrivate:public RenderPrivate,protected QOpenGLFunctions
{
public:
	QSize inner;
	bool UVReverted;
	QOpenGLShaderProgram program;
	GLuint frame[3];
	static QMutex dataLock;
	QList<quint8 *> buffer;

	void initialize()
	{
		initializeOpenGLFunctions();
		glGenTextures(3,frame);
		program.addShaderFromSourceCode(QOpenGLShader::Vertex  ,vShaderCode);
		program.addShaderFromSourceCode(QOpenGLShader::Fragment,fShaderCode);
		program.bindAttributeLocation("VtxCoord",0);
		program.bindAttributeLocation("TexCoord",1);
		glPixelStorei(GL_UNPACK_ALIGNMENT,1);
	}

	void uploadTexture(int i,int w,int h)
	{
		glBindTexture(GL_TEXTURE_2D,frame[i]);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
		glTexImage2D(GL_TEXTURE_2D,0,GL_LUMINANCE,w,h,0,GL_LUMINANCE,GL_UNSIGNED_BYTE,buffer[i]);
	}

	void drawData(QPainter *painter,QRect rect)
	{
		if (inner.isEmpty()){
			return;
		}
		QRect dest=fitRect(inner,rect);
		painter->beginNativePainting();
		if (dirty){
			int w=inner.width(),h=inner.height();
			dataLock.lock();
			uploadTexture(0,w,h);
			uploadTexture(1,w/2,h/2);
			uploadTexture(2,w/2,h/2);
			dirty=false;
			dataLock.unlock();
		}
		program.bind();
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
		program.setAttributeArray(0,vtx,2);
		program.setAttributeArray(1,tex,2);
		program.enableAttributeArray(0);
		program.enableAttributeArray(1);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D,frame[0]);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D,frame[1]);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D,frame[2]);
		program.setUniformValue("SamplerY",0);
		program.setUniformValue("SamplerU",UVReverted?2:1);
		program.setUniformValue("SamplerV",UVReverted?1:2);
		glDrawArrays(GL_TRIANGLE_STRIP,0,4);
		painter->endNativePainting();
	}

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
		int s=size.width()*size.height();
		for(quint8 *iter:buffer){
			delete[]iter;
		}
		buffer.clear();
		buffer.append(new quint8[s]);
		s/=4;
		buffer.append(new quint8[s]);
		buffer.append(new quint8[s]);
		if (bufferSize){
			bufferSize->clear();
			bufferSize->append(size);
			size/=2;
			bufferSize->append(size);
			bufferSize->append(size);
		}
	}

	~OpenGLRenderPrivate()
	{
		for(quint8 *iter:buffer){
			delete[]iter;
		}
	}
};

QMutex OpenGLRenderPrivate::dataLock;

class OWidget:public QOpenGLWidget
{
public:
	explicit OWidget(OpenGLRenderPrivate *render):
		QOpenGLWidget(lApp->mainWidget()),render(render)
	{
		setAttribute(Qt::WA_TransparentForMouseEvents);
		connect(this,SIGNAL(frameSwapped()),this,SLOT(update()),Qt::QueuedConnection);
	}

private:
	OpenGLRenderPrivate  *render;

	void initializeGL()
	{
		render->initialize();
	}

	void paintGL()
	{
		QPainter painter(this);
		painter.setRenderHints(QPainter::SmoothPixmapTransform);
		QRect rect(QPoint(0,0),Render::instance()->getActualSize());
		if(APlayer::instance()->getState()==APlayer::Stop){
			render->drawStop(&painter,rect);
		}
		else{
			render->drawPlay(&painter,rect);
			render->drawTime(&painter,rect);
		}
	}
};

class OpenGLRender:public Render
{
public:
	OpenGLRender(QObject *parent=0):
		Render(new OpenGLRenderPrivate,parent)
	{
		Q_D(OpenGLRender);
		ins=this;
		setObjectName("ORender");
		widget=new OWidget(d);
	}

private:
	OWidget *widget;
	Q_DECLARE_PRIVATE(OpenGLRender)

public slots:
	quintptr getHandle()
	{
		return (quintptr)widget;
	}

	void resize(QSize size)
	{
		widget->resize(size);
	}

	QSize getActualSize()
	{
		return widget->size();
	}

	QSize getBufferSize()
	{
		Q_D(OpenGLRender);
		return d->inner;
	}
};
#endif

#ifdef RENDER_DETACH
class DetachRenderPrivate:public RenderPrivate
{
public:
	void drawData(QPainter *,QRect)
	{
	}
	
	void drawDanm(QPainter *painter,QRect)
	{
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

	QList<quint8 *> getBuffer()
	{
		return QList<quint8 *>();
	}

	void setBuffer(QString &chroma,QSize,QList<QSize> *)
	{
		chroma="NONE";
	}

	void releaseBuffer()
	{
	}
};

class OWindow:public QOpenGLWindow
{
public:
	explicit OWindow(DetachRenderPrivate *render):
		render(render)
	{
		QSurfaceFormat f=format();
		f.setAlphaBufferSize(8);
		setFormat(f);
		setFlags(flags()|Qt::Tool|Qt::FramelessWindowHint|Qt::WindowTransparentForInput|Qt::WindowStaysOnTopHint);
		setGeometry(qApp->desktop()->screenGeometry());
		connect(this,SIGNAL(frameSwapped()),this,SLOT(update()),Qt::QueuedConnection);
		connect(context(),SIGNAL(aboutToBeDestroyed()),this,SLOT(deleteLater()));
	}

private:
	DetachRenderPrivate  *render;

	void paintGL()
	{
		QPainter painter(this);
		painter.setRenderHints(QPainter::SmoothPixmapTransform);
		QRect rect(QPoint(0,0),Render::instance()->getActualSize());
		painter.setCompositionMode(QPainter::CompositionMode_Source    );
		painter.fillRect(rect,Qt::transparent);
		painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
		render->drawDanm(&painter,rect);
	}
};

class DetachRender:public Render
{
public:
	DetachRender(QObject *parent=0):
		Render(new DetachRenderPrivate,parent)
	{
		ins=this;
		setObjectName("DRender");
		Q_D(DetachRender);
		d->tv.disconnect();
		window=new OWindow(d);
		connect(APlayer::instance(),&APlayer::begin,	window,&QWindow::show);
		connect(APlayer::instance(),&APlayer::reach,	window,&QWindow::hide);
		connect(APlayer::instance(),&APlayer::destroyed,window,&QWindow::hide);
	}
	
private:
	OWindow *window;
	Q_DECLARE_PRIVATE(DetachRender)

public slots:
	quintptr getHandle()
	{
		return (quintptr)window;
	}

	void resize(QSize)
	{
	}

	QSize getActualSize()
	{
		return window->size();
	}

	QSize getBufferSize()
	{
		return QSize();
	}
};
#endif

Render *Render::instance()
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
#ifdef RENDER_OPENGL
	if(r=="OpenGL"){
		return new OpenGLRender(qApp);
	}
#endif
#ifdef RENDER_RASTER
	if(r=="Raster"){
		return new RasterRender(qApp);
	}
#endif
#ifdef RENDER_DETACH
	if(r=="Detach"){
		return new DetachRender(qApp);
	}
#endif
	return 0;
}

Render::Render(RenderPrivate *data,QObject *parent):
	QObject(parent),d_ptr(data)
{
	Q_D(Render);
	d->time=0;
	if(Config::getValue("/Interface/Version",true)){
		d->tv.setFileName(":/Picture/tv.gif");
		d->tv.start();
		d->me=QImage(":/Picture/version.png");
		connect(APlayer::instance(),&APlayer::begin,&d->tv,&QMovie::stop );
		connect(APlayer::instance(),&APlayer::reach,&d->tv,&QMovie::start);
		connect(&d->tv,&QMovie::updated,[=](){
			QImage cf=d->tv.currentImage();
			QPoint ps=QRect(QPoint(0,0),getActualSize()).center()-cf.rect().center();
			ps.ry()-=40;
			draw(QRect(ps,cf.size()));
		});
	}
	QString path=Config::getValue("/Interface/Background",QString());
	if(!path.isEmpty()){
		d->background=QImage(path);
	}
	d->sound=QImage(":/Picture/sound.png");
	d->music=1;
	d->dirty=0;
	d->videoAspectRatio=0;
	d->pixelAspectRatio=1;

	connect(APlayer::instance(),&APlayer::stateChanged,[d](){d->last=QTime();});
	connect(APlayer::instance(),&APlayer::reach,[this](){
		setRefreshRate(Config::getValue("/Danmaku/Power",60));
	});

	d->power=new QTimer(this);
	d->power->setTimerType(Qt::PreciseTimer);
	connect(APlayer::instance(),&APlayer::decode,[=](){
		if(!d->power->isActive()){
			draw();
		}
	});
	connect(d->power,&QTimer::timeout,APlayer::instance(),[this](){
		if(APlayer::instance()->getState()==APlayer::Play){
			draw();
		}
	});
	QMetaObject::invokeMethod(this,"setRefreshRate",Qt::QueuedConnection,
							  Q_ARG(int,Config::getValue("/Danmaku/Power",60)));
}

Render::~Render()
{
	delete d_ptr;
}

QList<quint8 *> Render::getBuffer()
{
	Q_D(Render);
	return d->getBuffer();
}

void Render::releaseBuffer()
{
	Q_D(Render);
	d->releaseBuffer();
}

void Render::setBuffer(QString &chroma,QSize size,QList<QSize> *bufferSize)
{
	Q_D(Render);
	d->setBuffer(chroma,size,bufferSize);
}

void Render::setMusic(bool isMusic)
{
	Q_D(Render);
	if((d->music=isMusic)&&!d->power->isActive()){
		setRefreshRate(60,true);
	}
}

void Render::setRefreshRate(int rate,bool soft)
{
	Q_D(Render);
	if(rate){
		rate=qBound(30,rate,200);
		d->power->start(qRound(1000.0/rate));
		emit refreshRateChanged(rate);
	}
	else{
		rate=0;
		d->power->stop();
		emit refreshRateChanged(rate);
	}
	if(!soft){
		Config::setValue("/Danmaku/Power",rate);
	}
}

void Render::setDisplayTime(double t)
{
	Q_D(Render);
	d->time=t;
	QSize s=getActualSize();
	draw(QRect(0,s.height()-2,s.width()*d->time,2));
}

void Render::setVideoAspectRatio(double ratio)
{
	Q_D(Render);
	d->videoAspectRatio=ratio;
}

void Render::setPixelAspectRatio(double ratio)
{
	Q_D(Render);
	d->pixelAspectRatio=ratio;
}

QSize Render::getPreferredSize()
{
	Q_D(Render);
	if(d->music){
		return QSize();
	}
	QSize s=getBufferSize();
	d->pixelAspectRatio>1?(s.rwidth()*=d->pixelAspectRatio):(s.rheight()/=d->pixelAspectRatio);
	return s;
}

void Render::draw(QRect rect)
{
	Q_UNUSED(rect);
}