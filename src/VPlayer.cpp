/*=======================================================================
*
*   Copyright (C) 2013 Lysine.
*
*   Filename:    VPlayer.cpp
*   Time:        2013/03/18
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

#include "VPlayer.h"
#include "Local.h"
#include "Utils.h"
#include "Config.h"

#ifdef Q_OS_WIN32
#include <windows.h>
#endif

QMutex VPlayer::data;
QMutex VPlayer::time;
VPlayer *VPlayer::ins=NULL;

static AVPixelFormat getFormat(char *chroma)
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
	QString c=QString(chroma).toUpper();
	if(!f.contains(c)){
		if(c=="NV61"){
			strcpy(chroma,"NV16");
		}
		else if(c=="YV12"||
				c=="IYUV"){
			strcpy(chroma,"I420");
		}
		else if(c=="UYNV"||
				c=="UYNY"||
				c=="Y422"||
				c=="HDYC"||
				c=="AVUI"||
				c=="UYV1"||
				c=="2VUY"||
				c=="2VU1"){
			strcpy(chroma,"UYVY");
		}
		else if(c=="VYUY"||
				c=="YUYV"||
				c=="YUNV"||
				c=="V422"||
				c=="YVYU"||
				c=="Y211"||
				c=="CYUV"){
			strcpy(chroma,"YUY2");
		}
		else{
			strcpy(chroma,"RV32");
		}
		c=chroma;
	}
	return f[c];
}

class RasterPlayer:public VPlayer
{
public:
	explicit RasterPlayer(QObject *parent=NULL):
		VPlayer(parent)
	{
		swsctx=NULL;
		srcFrame=NULL;
		dstFrame=NULL;
		dstFormat=AV_PIX_FMT_RGB32;
		ins=this;
	}

	~RasterPlayer()
	{
		release();
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

	void getBuffer(void **planes)
	{
		for(int i=0;i<8;++i){
			if(srcFrame->linesize[i]==0){
				break;
			}
			planes[i]=srcFrame->data[i];
		}
		start=true;
	}

	void setBuffer(char *chroma,unsigned *width,unsigned *height,unsigned *pitches,unsigned *lines)
	{
		srcSize=QSize(*width,*height);
		if(srcFrame){
			avpicture_free(srcFrame);
		}
		else{
			srcFrame=new AVPicture;
		}
		srcFormat=getFormat(chroma);
		avpicture_alloc(srcFrame,srcFormat,srcSize.width(),srcSize.height());
		for(int i=0;i<3;++i){
			int l;
			if((l=srcFrame->linesize[i])==0){
				break;
			}
			pitches[i]=l;
			lines[i]=srcSize.height();
		}
	}

	void draw(QPainter *painter,QRect rect)
	{
		if(getState()!=Stop){
			painter->fillRect(rect,Qt::black);
			if(music){
				painter->drawImage(rect.center()-QRect(QPoint(0,0),sound.size()).center(),sound);
			}
			else if(start){
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
		}
	}

private:
	SwsContext *swsctx;
	QSize srcSize;
	QSize dstSize;
	AVPixelFormat srcFormat;
	AVPixelFormat dstFormat;
	AVPicture *srcFrame;
	AVPicture *dstFrame;
	QImage frame;
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
class OpenGLPlayer:public VPlayer,protected QOpenGLFunctions
{
public:
	explicit OpenGLPlayer(QObject *parent=NULL):
		VPlayer(parent)
	{
		for(auto &iter:buffer){
			iter=NULL;
		}
		initialize=true;
		vShader=fShader=program=0;
		ins=this;
	}

	~OpenGLPlayer()
	{
		release();
		if(!initialize){
			glDeleteShader(vShader);
			glDeleteShader(fShader);
			glDeleteProgram(program);
			glDeleteTextures(3,frame);
		}
		for(auto *iter:buffer){
			if(iter){
				delete iter;
			}
		}
	}

	void getBuffer(void **planes)
	{
		for(int i=0;i<3;++i){
			planes[i]=buffer[i];
		}
		start=true;
	}

	void setBuffer(char *chroma,unsigned *width,unsigned *height,unsigned *pitches,unsigned *lines)
	{
		strcpy(chroma,"I420");
		int w=*width,h=*height;
		inner=QSize(w,h);
		for(auto *iter:buffer){
			if(iter){
				delete iter;
			}
		}
		buffer[0]=new uchar[w*h];
		buffer[1]=new uchar[w*h/4];
		buffer[2]=new uchar[w*h/4];
		pitches[0]=w;
		pitches[1]=pitches[2]=w/2;
		lines[0]=h;
		lines[1]=lines[2]=h/2;
	}

	void draw(QPainter *painter,QRect rect)
	{
		if(getState()!=Stop){
			painter->fillRect(rect,Qt::black);
			if(music){
				painter->drawImage(rect.center()-QRect(QPoint(0,0),sound.size()).center(),sound);
			}
			else if(start){
				QRect dest=getRect(rect);
				data.lock();
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
				data.unlock();
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
				glBindTexture(GL_TEXTURE_2D,frame[1]);
				glUniform1i(glGetUniformLocation(program,"SamplerU"),1);
				glActiveTexture(GL_TEXTURE2);
				glBindTexture(GL_TEXTURE_2D,frame[2]);
				glUniform1i(glGetUniformLocation(program,"SamplerV"),2);
				glDrawArrays(GL_TRIANGLE_STRIP,0,4);
				painter->endNativePainting();
			}
		}
	}

private:
	QSize inner;
	bool initialize;
	GLuint vShader;
	GLuint fShader;
	GLuint program;
	GLuint frame[3];
	uchar *buffer[3];

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
};

VPlayer *VPlayer::instance()
{
	if(ins){
		return ins;
	}
	if(Config::getValue("/Interface/Accelerated",false)){
		return new OpenGLPlayer(Local::mainWidget());
	}
	else{
		return new RasterPlayer(Local::mainWidget());
	}
}

static unsigned fmt(void **,char *chroma,
					unsigned *width,unsigned *height,
					unsigned *pitches, unsigned *lines)
{
	VPlayer::instance()->setBuffer(chroma,width,height,pitches,lines);
	return 1;
}

static void *lck(void *,void **planes)
{
	VPlayer::data.lock();
	VPlayer::instance()->getBuffer(planes);
	return NULL;
}

static void dsp(void *,void *)
{
	VPlayer::instance()->setDirty();
	VPlayer::data.unlock();
}

static void sta(const libvlc_event_t *,void *)
{
	QMetaObject::invokeMethod(VPlayer::instance(),"init");
}

static void mid(const libvlc_event_t *,void *)
{
	if (VPlayer::time.tryLock()) {
		QMetaObject::invokeMethod(VPlayer::instance(),
								  "timeChanged",
								  Q_ARG(qint64,VPlayer::instance()->getTime()));
		VPlayer::time.unlock();
	}
}

static void end(const libvlc_event_t *,void *)
{
	QMetaObject::invokeMethod(VPlayer::instance(),"free");
}

VPlayer::VPlayer(QObject *parent):
	QObject(parent)
{
	QList<QByteArray> args;
	for(QJsonValue arg:Config::getValue<QJsonArray>("/Playing/Arguments")){
		args.append(arg.toString().toUtf8());
	}
	const char *argv[args.size()];
	for(int i=0;i<args.size();++i){
		argv[i]=args[i];
	}
	vlc=libvlc_new(args.size(),argv);
#ifdef Q_OS_WIN
	libvlc_add_intf(vlc,"bililocal");
#endif
	m=NULL;
	mp=NULL;
	state=Stop;
	ratio=0;
	start=false;
	music=false;
	dirty=false;
	setObjectName("VPlayer");
	sound=QImage(":/Picture/sound.png");
	for(auto &iter:tracks){
		iter=new QActionGroup(this);
		iter->setExclusive(true);
	}
	fake=new QTimer(this);
	fake->setInterval(33);
	fake->setTimerType(Qt::PreciseTimer);
	connect(fake,&QTimer::timeout,this,&VPlayer::decode);
}

VPlayer::~VPlayer()
{
}

qint64 VPlayer::getTime()
{
	return state==Stop?-1:libvlc_media_player_get_time(mp);
}

qint64 VPlayer::getDuration()
{
	return mp?libvlc_media_player_get_length(mp):-1;
}

QString VPlayer::getFile()
{
	if(m){
		char *s=libvlc_media_get_mrl(m);
		QUrl u(s);
		libvlc_free(s);
		if(u.isLocalFile()){
			return u.toLocalFile();
		}
	}
	return QString();
}

QList<QAction *> VPlayer::getTracks(int type)
{
	QList<QAction *> track;
	if(type&Utils::Video){
		track+=tracks[0]->actions();
	}
	if(type&Utils::Audio){
		track+=tracks[1]->actions();
	}
	if(type&Utils::Subtitle){
		track+=tracks[2]->actions();
	}
	return track;
}

QRect VPlayer::getRect(QRect rect)
{
	QRect dest;
	dest.setSize((ratio>0?QSizeF(ratio,1):QSizeF(size)).scaled(rect.size(),Qt::KeepAspectRatio).toSize()/4*4);
	dest.moveCenter(rect.center());
	return dest;
}

static void copyTracks(libvlc_track_description_t *head,QActionGroup *group)
{
	qDeleteAll(group->actions());
	libvlc_track_description_t *iter=head;
	while(iter){
		QAction *action=group->addAction(iter->psz_name);
		action->setCheckable(true);
		action->setData(iter->i_id);
		iter=iter->p_next;
	}
	libvlc_track_description_list_release(head);
}

void VPlayer::init()
{
	if(mp){
		auto *connection=new QMetaObject::Connection;
		*connection=QObject::connect(this,&VPlayer::timeChanged,[=](){
			if(state==Stop){
				setState(Play);
				libvlc_media_track_t **info;
				int n=libvlc_media_tracks_get(m,&info);
				music=true;
				size=QSize();
				for(int i=0;i<n;++i){
					if(info[i]->i_type==libvlc_track_video){
						libvlc_video_track_t *v=info[i]->video;
						music=false;
						double r=v->i_sar_den==0?1:(double)v->i_sar_num/v->i_sar_den;
						size=QSize(v->i_width*r,v->i_height);
						break;
					}
				}
				libvlc_media_tracks_release(info,n);
				if(music){
					fake->start();
				}
				if(!Config::getValue("/Playing/Subtitle",true)){
					libvlc_video_set_spu(mp,-1);
				}
				copyTracks(libvlc_video_get_spu_description(mp),tracks[2]);
				copyTracks(libvlc_video_get_track_description(mp),tracks[0]);
				copyTracks(libvlc_audio_get_track_description(mp),tracks[1]);
				for(QAction *i:tracks[0]->actions()){
					int t=i->data().toInt();
					connect(i,&QAction::triggered,[=](){
						libvlc_video_set_track(mp,t);
						if(t==-1){
							fake->start();
							music=1;
						}
						else{
							fake->stop();
							music=0;
						}
					});
					i->setChecked(t==libvlc_video_get_track(mp));
				}
				for(QAction *i:tracks[1]->actions()){
					connect(i,&QAction::triggered,[=](){libvlc_audio_set_track(mp,i->data().toInt());});
					i->setChecked(i->data().toInt()==libvlc_audio_get_track(mp));
				}
				for(QAction *i:tracks[2]->actions()){
					connect(i,&QAction::triggered,[=](){libvlc_video_set_spu(mp,i->data().toInt());});
					i->setChecked(i->data().toInt()==libvlc_video_get_spu(mp));
				}
				emit begin();
			}
			if(state==Loop){
				setState(Play);
				for(auto *g:tracks){
					for(QAction *i:g->actions()){
						if(i->isChecked()){
							i->trigger();
						}
					}
				}
			}
			setVolume(Config::getValue("/Playing/Volume",50));
			QObject::disconnect(*connection);
			delete connection;
		});
	}
}

void VPlayer::free()
{
	if(state==Play&&Config::getValue("/Playing/Loop",false)){
		libvlc_media_player_stop(mp);
		setState(Loop);
		libvlc_media_player_play(mp);
		emit jumped(0);
	}
	else{
		stop();
	}
}

void VPlayer::release()
{
	QMutex exit;
	libvlc_set_exit_handler(vlc,[](void *opaque){
		((QMutex *)opaque)->unlock();
	},&exit);
	exit.lock();
	if(mp){
		libvlc_media_player_release(mp);
	}
	if(m){
		libvlc_media_release(m);
	}
	libvlc_release(vlc);
	exit.lock();
	exit.unlock();
}

void VPlayer::setState(State _state)
{
#ifdef Q_OS_WIN32
	switch(_state){
	case Play:
	case Loop:
		SetThreadExecutionState(ES_DISPLAY_REQUIRED|ES_SYSTEM_REQUIRED|ES_CONTINUOUS);
		break;
	default:
		SetThreadExecutionState(ES_CONTINUOUS);
		break;
	}
#endif
	state=_state;
	emit stateChanged(state);
}

void VPlayer::play()
{
	if(mp){
		if(state==Stop){
			libvlc_video_set_format_callbacks(mp,fmt,NULL);
			libvlc_video_set_callbacks(mp,lck,NULL,dsp,NULL);
			libvlc_media_player_play(mp);
		}
		else{
			libvlc_media_player_pause(mp);
			setState(state==Play?Pause:Play);
		}
	}
}

void VPlayer::stop()
{
	if(mp&&state!=Stop){
		libvlc_media_player_stop(mp);
		size=QSize();
		ratio=0;
		setState(Stop);
		if(music){
			fake->stop();
		}
		start=false;
		for(auto g:tracks){
			qDeleteAll(g->actions());
		}
		emit reach();
	}
}

void VPlayer::setDirty()
{
	if(state!=Pause){
		dirty=true;
		emit decode();
	}
}

void VPlayer::setTime(qint64 _time)
{
	if(mp&&state!=Stop){
		if(getDuration()==_time){
			if(Config::getValue("/Playing/Loop",false)){
				setTime(0);
			}
			else{
				stop();
			}
		}
		else{
			time.lock();
			qApp->processEvents();
			emit jumped(_time);
			libvlc_media_player_set_time(mp,qBound<qint64>(0,_time,getDuration()));
			time.unlock();
		}
	}
}

void VPlayer::setMedia(QString _file)
{
	stop();
	if(m){
		libvlc_media_release(m);
	}
	if(mp){
		libvlc_media_player_release(mp);
	}
	m=libvlc_media_new_path(vlc,QDir::toNativeSeparators(_file).toUtf8());
	emit mediaChanged(m?getFile():QString());
	if(m){
		mp=libvlc_media_player_new_from_media(m);
		if(mp){
			Config::setValue("/Playing/Path",QFileInfo(_file).absolutePath());
			libvlc_event_manager_t *man=libvlc_media_player_event_manager(mp);
			libvlc_event_attach(man,
								libvlc_MediaPlayerPlaying,
								sta,NULL);
			libvlc_event_attach(man,
								libvlc_MediaPlayerTimeChanged,
								mid,NULL);
			libvlc_event_attach(man,
								libvlc_MediaPlayerEndReached,
								end,NULL);
			if(Config::getValue("/Playing/Immediate",false)){
				play();
			}
		}
	}
}

void VPlayer::setRatio(double _ratio)
{
	ratio=_ratio;
}

void VPlayer::setVolume(int _volume)
{
	if(mp){
		_volume=qBound(0,_volume,100);
		libvlc_audio_set_volume(mp,_volume);
		emit volumeChanged(_volume);
	}
}

void VPlayer::addSubtitle(QString _file)
{
	if(mp){
		if(tracks[2]->actions().isEmpty()){
			QAction *action=tracks[2]->addAction(tr("Disable"));
			action->setCheckable(true);
			connect(action,&QAction::triggered,[this](){
				libvlc_video_set_spu(mp,-1);
			});
		}
		QAction *outside=new QAction(tracks[2]);
		QFileInfo info(_file);
		outside->setCheckable(true);
		outside->setText(qApp->fontMetrics().elidedText(info.fileName(),Qt::ElideMiddle,200));
		outside->setData(QDir::toNativeSeparators(info.absoluteFilePath()));
		connect(outside,&QAction::triggered,[=](){
			libvlc_video_set_subtitle_file(mp,outside->data().toString().toUtf8());
		});
		outside->trigger();
	}
}
