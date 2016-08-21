#include "JPlayer.h"
#include "../Local.h"
#include "../Render/ABuffer.h"
#include "../Render/ARender.h"
#include "../Render/PFormat.h"
#include <atomic>

namespace
{
	void onError(JNIEnv *env, jobject mp, jint what, jint extra)
	{
		Q_UNUSED(env);
		Q_UNUSED(mp);
		qDebug() << "JPlayer: error" << what << extra;
		auto p = lApp->findObject<APlayer>();
		p->event(JPlayer::Fail, what);
	}

	void onInfo(JNIEnv *env, jobject mp, jint what, jint extra)
	{
		Q_UNUSED(env);
		Q_UNUSED(mp);
		qDebug() << "JPlayer: info " << what << extra;
		auto p = lApp->findObject<APlayer>();
		p->event(JPlayer::Info, what);
	}

	void onVideoSize(JNIEnv *env, jobject mp, jint width, jint height)
	{
		Q_UNUSED(env);
		Q_UNUSED(mp);
		auto p = lApp->findObject<APlayer>();
		p->event(JPlayer::Size, QSize(width, height));
	}

	void onPrepared(JNIEnv *env, jobject mp)
	{
		Q_UNUSED(env);
		Q_UNUSED(mp);
		auto p = lApp->findObject<APlayer>();
		p->event(JPlayer::Prep);
	}

	void onComplete(JNIEnv *env, jobject mp)
	{
		Q_UNUSED(env);
		Q_UNUSED(mp);
		auto p = lApp->findObject<APlayer>();
		p->event(JPlayer::Comp);
	}

	void onAvailable(JNIEnv *env, jobject mp)
	{
		Q_UNUSED(env);
		Q_UNUSED(mp);
		auto p = lApp->findObject<APlayer>();
		p->event(JPlayer::Avai);
	}
}

JPlayer::JPlayer(QObject *parent)
	: APlayer(parent)
	, mp("tv/danmaku/local/Player")
	, st("tv/danmaku/local/Output")
	, tn(QSharedPointer<GLuint>::create(0))
	, state(Stop)
{
	QAndroidJniEnvironment env;
	jclass pc = env->GetObjectClass(mp.object());
	static const JNINativeMethod pm[] = {
		{"onError", "(II)V", reinterpret_cast<void *>(onError)},
		{"onInfo" , "(II)V", reinterpret_cast<void *>(onInfo )},
		{"onVideoSize", "(II)V", reinterpret_cast<void *>(onVideoSize)},
		{"onPrepared", "()V", reinterpret_cast<void *>(onPrepared)},
		{"onComplete", "()V", reinterpret_cast<void *>(onComplete)},
	};
	env->RegisterNatives(pc, pm, sizeof(pm) / sizeof(*pm));
	jclass tc = env->GetObjectClass(st.object());
	static const JNINativeMethod tm[] = {
		{"onAvailable", "()V", reinterpret_cast<void *>(onAvailable)},
	};
	env->RegisterNatives(tc, tm, sizeof(tm) / sizeof(*tm));

	QAndroidJniObject sh("tv/danmaku/local/Holder", "(Landroid/graphics/SurfaceTexture;)V", st.object());
	mp.callMethod<void>("setDisplay", "(Landroid/view/SurfaceHolder;)V", sh.object());

	auto timer = new QTimer(this);
	timer->setSingleShot(false);
	timer->start(100);
	connect(timer, &QTimer::timeout, [this]() {
		if (getState() == Play) {
			emit timeChanged(getTime());
		}
	});

	setLoop(APlayer::getLoop());
}

namespace
{
	class FrameBuffer : public ABuffer
	{
	public:
		explicit FrameBuffer(const QAndroidJniObject &st, const QSharedPointer<GLuint> &tn)
			: st(st)
			, flag(ATOMIC_FLAG_INIT)
			, tn(tn)
		{
			QObject::connect(lApp->findObject<APlayer>(), &APlayer::decode, [this]() {
				flag.clear();
			});
		}

		virtual bool map() override
		{
			return false;
		}

		virtual const uchar *bits() const override
		{
			return nullptr;
		}

		virtual QList<QSize> size() const override
		{
			return QList<QSize>();
		}

		virtual void unmap() override
		{
		}

		virtual HandleType handleType() const override
		{
			return GLTextureExHandle;
		}

		virtual QVariant handle() const override
		{
			if(*tn == 0){
				QOpenGLContext *ctx = QOpenGLContext::currentContext();
				st.callMethod<void>("detachFromGLContext");
				ctx->functions()->glGenTextures(1, tn.data());
				st.callMethod<void>("attachToGLContext", "(I)V", (jint)*tn);
				QObject::connect(ctx, &QOpenGLContext::aboutToBeDestroyed, [=]() {
					ctx->functions()->glDeleteTextures(1, tn.data());
				});
			}
			if (flag.test_and_set() == false) {
				st.callMethod<void>("updateTexImage");
			}
			return *tn;
		}

		virtual QVariant argument(QByteArray name) const override
		{
			if (name != "Transform") {
				return QVariant();
			}

			//(0, 0) is buttom left of texture, map it to top left.
			QMatrix4x4 map(1,  0,  0,  0,
						   0, -1,  0,  1,
						   0,  0,  1,  0,
						   0,  0,  0,  1);

			QAndroidJniEnvironment env;
			auto data = env->NewFloatArray(16);
			st.callMethod<void>("getTransformMatrix", "([F)V", data);
			QMatrix4x4 mat;
			env->GetFloatArrayRegion(data, 0, 16, mat.data());
			env->DeleteLocalRef(data);

			return map * mat;
		}

	private:
		QAndroidJniObject st;
		mutable std::atomic_flag flag;
		QSharedPointer<GLuint> tn;
	};
}

JPlayer::~JPlayer()
{
	mp.callMethod<void>("release");
}

void JPlayer::play()
{
	switch(state) {
	case Initialized:
		mp.callMethod<void>("prepareAsync");
		state = Preparing;
		break;
	case Started:
		mp.callMethod<void>("pause");
		state = Paused;
		break;
	case Prepared:
	case Paused:
	case Complete:
		mp.callMethod<void>("start");
		state = Started;
		break;
	}
	emit stateChanged(getState());
}

void JPlayer::stop(bool manually)
{
	mp.callMethod<void>("stop");
	state = Stopped;
	emit stateChanged(getState());
	emit reach(manually);
}

int JPlayer::getState()
{
	switch(state) {
	case Started:
		return Play;
	case Paused:
		return Pause;
	default:
		return Stop;
	}
}

void JPlayer::setTime(qint64 time)
{
	mp.callMethod<void>("seekTo", "(I)V", (jint)time);
}

qint64 JPlayer::getTime()
{
	return mp.callMethod<jint>("getCurrentPosition", "()I");
}

void JPlayer::setMedia(QString file)
{
	mp.callMethod<void>("reset");
	auto arg = QAndroidJniObject::fromString(file);
	mp.callMethod<void>("setDataSource", "(Ljava/lang/String;)V", arg.object());
	emit mediaChanged(meida = file);
	state = Initialized;
	emit stateChanged(getState());
}

QString JPlayer::getMedia()
{
	return meida;
}

qint64 JPlayer::getDuration()
{
	return mp.callMethod<jint>("getDuration", "()I");
}

void JPlayer::setVolume(int volume)
{
	mp.callMethod<void>("setVolume", "(F)V", (jfloat)(volume / 100.0f));
}

int JPlayer::getVolume()
{
	return 0;
}

void JPlayer::setLoop(bool loop)
{
	mp.callMethod<void>("setLooping", "(Z)V", (jboolean)loop);
	APlayer::setLoop(loop);
}

bool JPlayer::getLoop()
{
	return mp.callMethod<jboolean>("isLooping", "()Z");
}

void JPlayer::event(int type, QVariant args)
{
	switch(type) {
	case Fail:
		state = Error;
		emit errorOccurred(args.toInt());
		break;
	case Prep:
		state = Prepared;
		play();
		break;
	case Comp:
		state = Complete;
		emit reach(false);
		break;
	case Size:
	{
		QSize size = args.toSize();
		auto r = lApp->findObject<ARender>();
		r->setMusic(size.isEmpty());
		PFormat f;
		f.chroma = "GLEX";
		f.size = size;
		r->setFormat(&f);
		r->setBuffer(new FrameBuffer(st, tn));
		return;
	}
	case Avai:
		emit decode();
		return;
	default:
		return;
	}
	emit stateChanged(getState());
}
