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

Render *Render::ins=nullptr;

class RenderPrivate
{
public:
	QMovie tv;
	double time;
	QImage me,background,sound;
	QTime last;
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
		drawDanm(painter,rect);
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

	void drawDanm(QPainter *painter,QRect)
	{
		qint64 time=last.isNull()?0:last.elapsed();
		if (APlayer::instance()->getState()==APlayer::Play)
			last.start();
		Danmaku::instance()->draw(painter,time);
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

	QTimer *power;
	SwsContext *swsctx;
	Buffer *srcFrame;
	Buffer *dstFrame;
	QImage frame;
	QMutex dataLock;

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
			f.insert("RV24",AV_PIX_FMT_RGB24);
			f.insert("RGB8",AV_PIX_FMT_RGB8);
			f.insert("RV12",AV_PIX_FMT_RGB444);
			f.insert("RV15",AV_PIX_FMT_RGB555);
			f.insert("RV16",AV_PIX_FMT_RGB565);
			f.insert("RGBA",AV_PIX_FMT_RGBA);
			f.insert("ARGB",AV_PIX_FMT_ARGB);
			f.insert("BGRA",AV_PIX_FMT_BGRA);
			f.insert("I410",AV_PIX_FMT_YUV410P);
			f.insert("I411",AV_PIX_FMT_YUV411P);
			f.insert("I420",AV_PIX_FMT_YUV420P);
			f.insert("IYUV",AV_PIX_FMT_YUV420P);
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
			f.insert("YA0L",AV_PIX_FMT_YUVA444P10LE);
			f.insert("YA0B",AV_PIX_FMT_YUVA444P10BE);
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
			f.insert("YUYV",AV_PIX_FMT_YUYV422);
			f.insert("YUY2",AV_PIX_FMT_YUYV422);
		}
		chroma=chroma.toUpper();
		if(f.contains(chroma)){
		}
		else if(chroma=="YV12"){
			chroma="I420";
		}
		else if(chroma=="NV16"){
			chroma="NV12";
		}
		else if(chroma=="NV61"){
			chroma="NV21";
		}
		else if(chroma=="VYUY"||
				chroma=="YVYU"||
				chroma=="V422"||
				chroma=="CYUV"){
			chroma="UYVY";
		}
		else if(chroma=="V210"){
			chroma="I0AL";
		}
		else{
			chroma="I420";
		}
		return f[chroma];
	}

	void drawData(QPainter *painter,QRect rect)
	{
		if(!srcFrame->isValid()){
			return;
		}
		QRect dest=fitRect(Render::instance()->getPreferSize(),rect);
		QSize dstSize=dest.size()*painter->device()->devicePixelRatio();
		if(!dstFrame||dstFrame->size!=dstSize){
			if(dstFrame){
				delete dstFrame;
			}
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

namespace
{
class RWidget:public QWidget
{
public:
	RWidget(RasterRenderPrivate *render):
		QWidget(lApp->mainWidget()),render(render)
	{
		setAttribute(Qt::WA_TransparentForMouseEvents);
		lower();
	}

private:
	RasterRenderPrivate *const render;

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

class ARCache:public Render::ICache
{
public:
	QImage i;

	ARCache(const QImage &i):i(i){}

	void draw(QPainter *p,QRectF r)
	{
		p->drawImage(r,i);
	}
};
}

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
		d->power=new QTimer(this);
		d->power->setTimerType(Qt::PreciseTimer);
		int fps=Config::getValue("/Performance/Raster/FPS",60);
		d->power->start(1000/(fps>0?fps:60));
		connect(APlayer::instance(),&APlayer::decode,d->power,[=](){
			if(!d->power->isActive()){
				draw();
			}
		});
		connect(d->power,&QTimer::timeout,APlayer::instance(),[=](){
			if(APlayer::instance()->getState()==APlayer::Play){
				draw();
			}
		});
	}

private:
	RWidget *widget;
	Q_DECLARE_PRIVATE(RasterRender);

public slots:
	ICache *getCache(const QImage &i)
	{
		return new ARCache(i);
	}

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

	void draw(QRect rect=QRect())
	{
		widget->update(rect.isValid()?rect:QRect(QPoint(0,0),getActualSize()));
	}
};
#endif

#ifdef RENDER_OPENGL
namespace
{
const char *vShaderCode=
	"attribute vec4 VtxCoord;\n"
	"attribute vec2 TexCoord;\n"
	"varying vec2 TexCoordOut;\n"
	"void main(void)\n"
	"{\n"
	"    gl_Position = VtxCoord;\n"
	"    TexCoordOut = TexCoord;\n"
	"}\n";

const char *fShaderI420=
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

const char *fShaderNV12=
	"varying highp vec2 TexCoordOut;\n"
	"uniform sampler2D SamplerY;\n"
	"uniform sampler2D SamplerA;\n"
	"void main(void)\n"
	"{\n"
	"    mediump vec3 yuv;\n"
	"    mediump vec3 rgb;\n"
	"    yuv.x = texture2D(SamplerY, TexCoordOut).r - 0.0625;\n"
	"    yuv.y = texture2D(SamplerA, TexCoordOut).r - 0.5;   \n"
	"    yuv.z = texture2D(SamplerA, TexCoordOut).a - 0.5;   \n"
	"    rgb = mat3(1.164,  1.164, 1.164,   \n"
	"               0,     -0.391, 2.018,   \n"
	"               1.596, -0.813, 0) * yuv;\n"
	"    gl_FragColor = vec4(rgb, 1);\n"
	"}";

const char *fShaderNV21=
	"varying highp vec2 TexCoordOut;\n"
	"uniform sampler2D SamplerY;\n"
	"uniform sampler2D SamplerA;\n"
	"void main(void)\n"
	"{\n"
	"    mediump vec3 yuv;\n"
	"    mediump vec3 rgb;\n"
	"    yuv.x = texture2D(SamplerY, TexCoordOut).r - 0.0625;\n"
	"    yuv.y = texture2D(SamplerA, TexCoordOut).a - 0.5;   \n"
	"    yuv.z = texture2D(SamplerA, TexCoordOut).r - 0.5;   \n"
	"    rgb = mat3(1.164,  1.164, 1.164,   \n"
	"               0,     -0.391, 2.018,   \n"
	"               1.596, -0.813, 0) * yuv;\n"
	"    gl_FragColor = vec4(rgb, 1);\n"
	"}";

const char *fShaderBGRP=
	"varying highp vec2 TexCoordOut;\n"
	"uniform sampler2D SamplerP;\n"
	"void main(void)\n"
	"{\n"
	"    mediump vec4 p;\n"
	"    p = texture2D(SamplerP, TexCoordOut);\n"
	"    if (p.a != 0.0) {\n"
	"        p.r /= p.a;\n"
	"        p.g /= p.a;\n"
	"        p.b /= p.a;\n"
	"        gl_FragColor = vec4(p.b, p.g, p.r, p.a);\n"
	"    } else {\n"
	"        gl_FragColor = vec4(0.0, 0.0, 0.0, 0.0);\n"
	"    }\n"
	"}";
}

class OpenGLRenderPrivateBase:public RenderPrivate,public QOpenGLFunctions
{
public:
	QOpenGLShaderProgram program[5];
	GLfloat vtx[8];
	GLfloat tex[8];

	void initialize()
	{
		initializeOpenGLFunctions();
		glPixelStorei(GL_UNPACK_ALIGNMENT,1);
		for(int i=0;i<5;++i){
			const char *fShaderCode=nullptr;
			switch(i){
			case 0:
			case 1:
				fShaderCode=fShaderI420;
				break;
			case 2:
				fShaderCode=fShaderNV12;
				break;
			case 3:
				fShaderCode=fShaderNV21;
				break;
			case 4:
				fShaderCode=fShaderBGRP;
				break;
			}
			QOpenGLShaderProgram &p=program[i];
			p.addShaderFromSourceCode(QOpenGLShader::Vertex  ,vShaderCode);
			p.addShaderFromSourceCode(QOpenGLShader::Fragment,fShaderCode);
			p.bindAttributeLocation("VtxCoord",0);
			p.bindAttributeLocation("TexCoord",1);
			p.bind();
			switch(i){
			case 0:
				p.setUniformValue("SamplerY",0);
				p.setUniformValue("SamplerU",1);
				p.setUniformValue("SamplerV",2);
				break;
			case 1:
				p.setUniformValue("SamplerY",0);
				p.setUniformValue("SamplerV",1);
				p.setUniformValue("SamplerU",2);
				break;
			case 2:
			case 3:
				p.setUniformValue("SamplerY",0);
				p.setUniformValue("SamplerA",1);
				break;
			case 4:
				p.setUniformValue("SamplerP",0);
				break;
			}
		}
		tex[0]=0;tex[1]=0;
		tex[2]=1;tex[3]=0;
		tex[4]=0;tex[5]=1;
		tex[6]=1;tex[7]=1;
	}
	
	void uploadTexture(GLuint t,int c,int w,int h,quint8 *d)
	{
		int f;
		switch(c){
		case 1:
			f=GL_LUMINANCE;
			break;
		case 2:
			f=GL_LUMINANCE_ALPHA;
			break;
		case 3:
			f=GL_RGB;
			break;
		case 4:
			f=GL_RGBA;
			break;
		default:
			return;
		}
		glBindTexture(GL_TEXTURE_2D,t);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
		glTexImage2D(GL_TEXTURE_2D,0,f,w,h,0,f,GL_UNSIGNED_BYTE,d);
	}

	void drawTexture(GLuint *texture,int format,QRectF dest,QRectF rect)
	{
		QOpenGLShaderProgram &p=program[format];
		p.bind();
		GLfloat h=2/rect.width(),v=2/rect.height();
		GLfloat l=dest.left()*h-1,r=dest.right()*h-1,t=1-dest.top()*v,b=1-dest.bottom()*v;
		vtx[0]=l;vtx[1]=t;
		vtx[2]=r;vtx[3]=t;
		vtx[4]=l;vtx[5]=b;
		vtx[6]=r;vtx[7]=b;
		p.setAttributeArray(0,vtx,2);
		p.setAttributeArray(1,tex,2);
		p.enableAttributeArray(0);
		p.enableAttributeArray(1);
		switch(format){
		case 0:
		case 1:
			glActiveTexture(GL_TEXTURE2);
			glBindTexture(GL_TEXTURE_2D,texture[2]);
		case 2:
		case 3:
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D,texture[1]);
		case 4:
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D,texture[0]);
			break;
		}
		glDrawArrays(GL_TRIANGLE_STRIP,0,4);
	}
};

class OpenGLRenderPrivate:public OpenGLRenderPrivateBase
{
public:
	QSize inner;
	int format;
	GLuint frame[3];
	QMutex dataLock;
	QList<quint8 *> buffer;
	
	void initialize()
	{
		OpenGLRenderPrivateBase::initialize();
		glGenTextures(3,frame);
	}

	void uploadTexture(int i,int c,int w,int h)
	{
		OpenGLRenderPrivateBase::uploadTexture(frame[i],c,w,h,buffer[i]);
	}

	void drawData(QPainter *painter,QRect rect)
	{
		if (inner.isEmpty()){
			return;
		}
		painter->beginNativePainting();
		if (dirty){
			int w=inner.width(),h=inner.height();
			dataLock.lock();
			switch(format){
			case 0:
			case 1:
				uploadTexture(0,1,w,h);
				uploadTexture(1,1,w/2,h/2);
				uploadTexture(2,1,w/2,h/2);
				break;
			case 2:
			case 3:
				uploadTexture(0,1,w,h);
				uploadTexture(1,2,w/2,h/2);
				break;
			case 4:
				uploadTexture(0,4,w,h);
				break;
			}
			dirty=false;
			dataLock.unlock();
		}
		QRect dest=fitRect(Render::instance()->getPreferSize(),rect);
		drawTexture(frame,format,dest,rect);
		painter->endNativePainting();
	}

	void paint(QPaintDevice *device)
	{
		QPainter painter(device);
		painter.setRenderHints(QPainter::SmoothPixmapTransform);
		QRect rect(QPoint(0,0),Render::instance()->getActualSize());
		if(APlayer::instance()->getState()==APlayer::Stop){
			drawStop(&painter,rect);
		}
		else{
			drawPlay(&painter,rect);
			drawTime(&painter,rect);
		}
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
		if     (chroma=="YV12"){
			format=1;
		}
		else if(chroma=="NV12"){
			format=2;
		}
		else if(chroma=="NV21"){
			format=3;
		}
		else{
			format=0;
			chroma="I420";
		}
		inner=size;
		int s=size.width()*size.height();
		quint8 *alloc=nullptr;
		QList<QSize> plane;
		switch(format){
		case 0:
		case 1:
			alloc=new quint8[s*3/2];
			plane.append(size);
			size/=2;
			plane.append(size);
			plane.append(size);
			break;
		case 2:
		case 3:
			alloc=new quint8[s*3/2];
			plane.append(size);
			size.rheight()/=2;
			plane.append(size);
			break;
		}
		if(!buffer.isEmpty()){
			delete []buffer[0];
		}
		buffer.clear();
		for(const QSize &s:plane){
			buffer.append(alloc);
			alloc+=s.width()*s.height();
		}
		if (bufferSize)
			bufferSize->swap(plane);
	}

	virtual quintptr getHandle()=0;
	virtual void resize(QSize)=0;
	virtual QSize getActualSize()=0;
	virtual void draw(QRect)=0;

	~OpenGLRenderPrivate()
	{
		if(!buffer.isEmpty()){
			delete []buffer[0];
		}
	}
};

namespace
{
class OWidget:public QOpenGLWidget
{
public:
	explicit OWidget(OpenGLRenderPrivate *render):
		QOpenGLWidget(lApp->mainWidget()),render(render)
	{
		setAttribute(Qt::WA_TransparentForMouseEvents);
		lower();
		connect(this,&OWidget::frameSwapped,[this](){
			if (isVisible()&&APlayer::instance()->getState()==APlayer::Play){
				QTimer::singleShot(2,Render::instance(),SLOT(draw()));
			}
		});
	}
	
private:
	OpenGLRenderPrivate *const render;

	void initializeGL()
	{
		render->initialize();
	}

	void paintGL()
	{
		render->paint(this);
	}
};
}

class OpenGLWidgetRenderPrivate:public OpenGLRenderPrivate
{
public:
	OpenGLWidgetRenderPrivate()
	{
		widget=new OWidget(this);
	}

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
	
	void draw(QRect rect)
	{
		widget->update(rect);
	}

private:
	OWidget *widget;
};

namespace
{
class OWindow:public QOpenGLWindow
{
public:
	explicit OWindow(OpenGLRenderPrivate *render):
		render(render)
	{
		window=nullptr;
		QTimer::singleShot(0,[this](){
			window=lApp->mainWidget()->backingStore()->window();
		});
		connect(this,&OWindow::frameSwapped,[this](){
			if (isVisible()&&APlayer::instance()->getState()==APlayer::Play){
				QTimer::singleShot(2,Render::instance(),SLOT(draw()));
			}
		});
	}
	
private:
	QWindow *window;
	OpenGLRenderPrivate *const render;

	void initializeGL()
	{
		render->initialize();
	}

	void paintGL()
	{
		render->paint(this);
	}

	bool event(QEvent *e)
	{
		switch(e->type()){
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
		case QEvent::Drop:
		case QEvent::ContextMenu:
			return window?qApp->sendEvent(window,e):false;
		default:
			return QOpenGLWindow::event(e);
		}
	}

};

/*	only QMdiSubWindow & QAbstractScrollArea
	will make windowcontainer to create native widgets.
*/
class FParent:public QAbstractScrollArea
{
public:
	explicit FParent(QWidget *parent):
		QAbstractScrollArea(parent)
	{
	}
};
}

class OpenGLWindowRenderPrivate:public OpenGLRenderPrivate
{
public:
	OpenGLWindowRenderPrivate()
	{
		widget=lApp->mainWidget();
		middle=new FParent(widget);
		middle->lower();
		window=new OWindow(this);
		widget=QWidget::createWindowContainer(window,middle);
		middle->setAcceptDrops(false);
		widget->setAcceptDrops(false);
	}

	quintptr getHandle()
	{
		return (quintptr)window;
	}

	void resize(QSize size)
	{
		middle->resize(size);
		widget->resize(size);
	}

	QSize getActualSize()
	{
		return widget->size();
	}

	void draw(QRect rect)
	{
		window->update(rect);
	}

private:
	QWidget *widget;
	FParent *middle;
	OWindow *window;
};

namespace
{
class STCache:public Render::ICache
{
public:
	GLuint texture;
	QImage source;
	OpenGLRenderPrivateBase *render;

	STCache(const QImage &image,OpenGLRenderPrivateBase *render):
		texture(0),source(image),render(render)
	{
	}

	~STCache()
	{
		if (texture)
			render->glDeleteTextures(1,&texture);
	}

	void draw(QPainter *painter,QRectF dest)
	{
		if(!texture){
			render->glGenTextures(1,&texture);
			render->uploadTexture(texture,4,source.width(),source.height(),(quint8 *)source.bits());
			source=QImage();
		}
		painter->beginNativePainting();
		QRect rect(QPoint(0,0),Render::instance()->getActualSize());
		render->glEnable(GL_BLEND);
		render->glBlendFuncSeparate(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA,GL_ONE,GL_ONE_MINUS_SRC_ALPHA);
		render->drawTexture(&texture,4,dest,rect);
		painter->endNativePainting();
	}
};
}

class OpenGLRender:public Render
{
public:
	explicit OpenGLRender(QObject *parent=0):
		Render(choose(),parent)
	{
		ins=this;
		setObjectName("ORender");
		connect(APlayer::instance(),SIGNAL(stateChanged(int)),this,SLOT(draw()));
	}

private:
	Q_DECLARE_PRIVATE(OpenGLRender);

	static OpenGLRenderPrivate *choose()
	{
		if (Config::getValue("/Performance/Option/OpenGL/FBO",true))
		{
			return new OpenGLWidgetRenderPrivate;
		}
		else
		{
			return new OpenGLWindowRenderPrivate;
		}
	}

public slots:
	ICache *getCache(const QImage &i)
	{
		Q_D(OpenGLRender);
		return new STCache(i,d);
	}

	quintptr getHandle()
	{
		Q_D(OpenGLRender);
		return d->getHandle();
	}

	void resize(QSize size)
	{
		Q_D(OpenGLRender);
		d->resize(size);
	}

	QSize getActualSize()
	{
		Q_D(OpenGLRender);
		return d->getActualSize();
	}

	QSize getBufferSize()
	{
		Q_D(OpenGLRender);
		return d->inner;
	}
	
	void draw(QRect rect=QRect())
	{
		Q_D(OpenGLRender);
		d->draw(rect.isValid()?rect:QRect(QPoint(0,0),getActualSize()));
	}
};

#endif

#ifdef RENDER_DETACH
class DetachRenderPrivate:public OpenGLRenderPrivateBase
{
public:
	void drawData(QPainter *,QRect)
	{
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

namespace
{
class DWindow:public QOpenGLWindow
{
public:
	explicit DWindow(DetachRenderPrivate *render):
		render(render)
	{
		QSurfaceFormat f=format();
		f.setAlphaBufferSize(8);
		setFormat(f);
		setFlags(flags()|Qt::Tool|Qt::FramelessWindowHint|Qt::WindowTransparentForInput|Qt::WindowStaysOnTopHint);
		setGeometry(qApp->desktop()->screenGeometry());
		connect(this,&DWindow::frameSwapped,[this](){
			if (isVisible()&&APlayer::instance()->getState()==APlayer::Play){
				QTimer::singleShot(2,Render::instance(),SLOT(draw()));
			}
		});
	}

private:
	DetachRenderPrivate *const render;
	
	void initializeGL()
	{
		render->initialize();
	}

	void paintGL()
	{
		render->glClearColor(0,0,0,0);
		render->glClear(GL_COLOR_BUFFER_BIT);
		QPainter painter(this);
		painter.setRenderHints(QPainter::SmoothPixmapTransform);
		QRect rect(QPoint(0,0),Render::instance()->getActualSize());
		render->drawDanm(&painter,rect);
	}
};
}

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
		window=new DWindow(d);
		window->create();
		connect(APlayer::instance(),&APlayer::begin,window,&QWindow::show);
		connect(APlayer::instance(),&APlayer::reach,window,&QWindow::hide);
	}
	
	~DetachRender()
	{
		if (window){
			delete window;
		}
	}
	
private:
	DWindow *window;
	Q_DECLARE_PRIVATE(DetachRender);

public slots:
	ICache *getCache(const QImage &i)
	{
		Q_D(DetachRender);
		return new STCache(i,d);
	}

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
	
	void draw(QRect rect=QRect())
	{
		window->update(rect.isValid()?rect:QRect(QPoint(0,0),getActualSize()));
	}
};
#endif

Render *Render::instance()
{
	if (ins){
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
		r=Config::getValue("/Performance/Render",l[0]);
		r=l.contains(r)?r:l[0];
		break;
	}
#ifdef RENDER_OPENGL
	if (r=="OpenGL"){
		return new OpenGLRender(qApp);
	}
#endif
#ifdef RENDER_RASTER
	if (r=="Raster"){
		return new RasterRender(qApp);
	}
#endif
#ifdef RENDER_DETACH
	if (r=="Detach"){
		return new DetachRender(qApp);
	}
#endif
	return nullptr;
}

Render::Render(RenderPrivate *data,QObject *parent):
	QObject(parent),d_ptr(data)
{
	Q_D(Render);
	d->time=0;
	connect(lApp,&Local::aboutToQuit,this,&Render::deleteLater);
	if(Config::getValue("/Interface/Version",true)){
		d->tv.setFileName(":/Picture/tv.gif");
		d->tv.setCacheMode(QMovie::CacheAll);
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

void Render::setBackground(QString path)
{
	Q_D(Render);
	d->background=QImage(path);
	Config::setValue("/Interface/Background",path);
}

void Render::setMusic(bool music)
{
	Q_D(Render);
	d->music=music;
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

QSize Render::getPreferSize()
{
	Q_D(Render);
	if(d->music){
		return QSize();
	}
	QSize s=APlayer::instance()->getSize();
	s=s.isValid()?s:getBufferSize();
	d->pixelAspectRatio>1?(s.rwidth()*=d->pixelAspectRatio):(s.rheight()/=d->pixelAspectRatio);
	return s;
}
