#include "Common.h"
#include "VPlayer.h"
#include "../Config.h"
#include "../Utils.h"
#include "../Render/ARender.h"

QMutex VPlayer::time;

namespace
{
	class Buffer
	{
	public:
		explicit Buffer(const QList<QSize> &planeSize)
		{
			size = 0;
			QList<int> planeLength;
			for (const QSize &s : planeSize){
				int length = s.width()*s.height();
				planeLength.append(length);
				size += length;
			}
			quint8 *alloc = new quint8[size];
			for (int length : planeLength){
				data.append(alloc);
				alloc += length;
			}
		}

		~Buffer()
		{
			delete[]data[0];
		}

		void flush()
		{
			memcpy(ARender::instance()->getBuffer()[0], data[0], size);
			ARender::instance()->releaseBuffer();
		}

		QList<quint8 *> getBuffer()
		{
			return data;
		}

	private:
		QList<quint8 *> data;
		int size;
	};

	unsigned fmt(void **opaque, char *chroma,
		unsigned *width, unsigned *height,
		unsigned *p, unsigned *l)
	{
		QString c(chroma);
		QList<QSize> b;
		ARender::instance()->setBuffer(c, QSize(*width, *height), 8, &b);
		if (b.isEmpty()){
			return 0;
		}
		memcpy(chroma, c.toUtf8(), 4);
		for (int i = 0; i < b.size(); ++i){
			const QSize &s = b[i];
			p[i] = s.width(); l[i] = s.height();
		}
		*opaque = (void *)new Buffer(b);
		return 1;
	}

	void *lck(void *opaque, void **planes)
	{
		int i = 0;
		for (quint8 *p : ((Buffer *)opaque)->getBuffer()){
			planes[i++] = (void *)p;
		}
		return nullptr;
	}

	void dsp(void *opaque, void *)
	{
		((Buffer *)opaque)->flush();
		emit APlayer::instance()->decode();
	}

	void clr(void *opaque)
	{
		delete (Buffer *)opaque;
	}

	void sta(const libvlc_event_t *, void *)
	{
		QMetaObject::invokeMethod(APlayer::instance(), "event", Q_ARG(int, VPlayer::Init));
	}

	void mid(const libvlc_event_t *, void *)
	{
		if (VPlayer::time.tryLock()) {
			QMetaObject::invokeMethod(APlayer::instance(),
				"timeChanged",
				Q_ARG(qint64, APlayer::instance()->getTime()));
			VPlayer::time.unlock();
		}
	}

	void hal(const libvlc_event_t *, void *)
	{
		QMetaObject::invokeMethod(APlayer::instance(), "event", Q_ARG(int, VPlayer::Wait));
	}

	void end(const libvlc_event_t *, void *)
	{
		QMetaObject::invokeMethod(APlayer::instance(), "event", Q_ARG(int, VPlayer::Free));
	}

	void err(const libvlc_event_t *, void *)
	{
		QMetaObject::invokeMethod(APlayer::instance(), "event", Q_ARG(int, VPlayer::Fail));
	}
}

VPlayer::VPlayer(QObject *parent) :
APlayer(parent)
{
	ins = this;
	setObjectName("VPlayer");
	QList<QByteArray> args;
	for (QJsonValue arg : Config::getValue<QJsonArray>("/Playing/Arguments")){
		args.append(arg.toString().toUtf8());
	}
	QVector<const char *> argv(args.size());
	for (int i = 0; i < args.size(); ++i){
		argv[i] = args[i].data();
	}
	vlc = libvlc_new(argv.size(), argv.data());
#ifdef Q_OS_WIN
	libvlc_add_intf(vlc, "bililocal");
#endif
	mp = nullptr;
	state = Stop;
	for (auto &iter : tracks){
		iter = new QActionGroup(this);
		iter->setExclusive(true);
	}
}

VPlayer::~VPlayer()
{
	if (mp){
		libvlc_media_player_release(mp);
	}
	libvlc_release(vlc);
}

QList<QAction *> VPlayer::getTracks(int type)
{
	QList<QAction *> track;
	if (type&Utils::Video){
		track += tracks[0]->actions();
	}
	if (type&Utils::Audio){
		track += tracks[1]->actions();
	}
	if (type&Utils::Subtitle){
		track += tracks[2]->actions();
	}
	return track;
}

namespace
{
	void copyTracks(libvlc_track_description_t *head, QActionGroup *group)
	{
		qDeleteAll(group->actions());
		libvlc_track_description_t *iter = head;
		while (iter){
			QAction *action = group->addAction(iter->psz_name);
			action->setCheckable(true);
			action->setData(iter->i_id);
			iter = iter->p_next;
		}
		libvlc_track_description_list_release(head);
	}
}

void VPlayer::init()
{
	if (mp){
		auto connection = QSharedPointer<QMetaObject::Connection>::create();
		*connection = connect(this, &VPlayer::timeChanged, this, [=](){
			int last = state;
			emit stateChanged(state = Play);
			disconnect(*connection);
			switch (last){
			case Stop:
			{
				ARender::instance()->setMusic(libvlc_video_get_track_count(mp) <= 0);
				if (!Config::getValue("/Playing/Subtitle", true)){
					libvlc_video_set_spu(mp, -1);
				}
				copyTracks(libvlc_video_get_spu_description(mp), tracks[2]);
				copyTracks(libvlc_video_get_track_description(mp), tracks[0]);
				copyTracks(libvlc_audio_get_track_description(mp), tracks[1]);
				for (QAction *i : tracks[0]->actions()){
					int t = i->data().toInt();
					connect(i, &QAction::triggered, [=](){
						libvlc_video_set_track(mp, t);
						ARender::instance()->setMusic(t == -1);
					});
					i->setChecked(t == libvlc_video_get_track(mp));
				}
				for (QAction *i : tracks[1]->actions()){
					connect(i, &QAction::triggered, [=](){libvlc_audio_set_track(mp, i->data().toInt()); });
					i->setChecked(i->data().toInt() == libvlc_audio_get_track(mp));
				}
				for (QAction *i : tracks[2]->actions()){
					connect(i, &QAction::triggered, [=](){libvlc_video_set_spu(mp, i->data().toInt()); });
					i->setChecked(i->data().toInt() == libvlc_video_get_spu(mp));
				}
				emit begin();
				break;
			}
			case Loop:
			{
				for (auto *g : tracks){
					for (QAction *i : g->actions()){
						if (i->isChecked()){
							i->trigger();
						}
					}
				}
				break;
			}
			default:
				return;
			}
			setVolume(Config::getValue("/Playing/Volume", 50));
		});
	}
}

void VPlayer::wait()
{
	emit stateChanged(state = Pause);
}

void VPlayer::free()
{
	if (state == Play&&Config::getValue("/Playing/Loop", false)){
		libvlc_media_player_stop(mp);
		emit stateChanged(state = Loop);
		libvlc_media_player_play(mp);
		emit jumped(0);
	}
	else{
		stop(false);
	}
}

void VPlayer::play()
{
	if (mp){
		if (state == Stop){
			libvlc_video_set_format_callbacks(mp, fmt, clr);
			libvlc_video_set_callbacks(mp, lck, nullptr, dsp, nullptr);
			libvlc_media_player_play(mp);
		}
		else{
			libvlc_media_player_pause(mp);
		}
	}
}

void VPlayer::stop(bool manually)
{
	if (mp&&state != Stop){
		libvlc_media_player_stop(mp);
		emit stateChanged(state = Stop);
		for (auto g : tracks){
			qDeleteAll(g->actions());
		}
		emit reach(manually);
	}
}

void VPlayer::setTime(qint64 _time)
{
	if (mp&&state != Stop){
		if (getDuration() == _time){
			if (Config::getValue("/Playing/Loop", false)){
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
			libvlc_media_player_set_time(mp, qBound<qint64>(0, _time, getDuration()));
			time.unlock();
		}
	}
}

qint64 VPlayer::getTime()
{
	return state == Stop ? -1 : libvlc_media_player_get_time(mp);
}

void VPlayer::setMedia(QString file, bool manually)
{
	stop(manually);
	libvlc_media_t *m = libvlc_media_new_path(vlc, QDir::toNativeSeparators(file).toUtf8());
	if (!m){
		return;
	}
	if (mp){
		libvlc_media_player_release(mp);
	}
	mp = libvlc_media_player_new_from_media(m);
	libvlc_media_release(m);
	if (!mp){
		return;
	}
	libvlc_event_manager_t *man = libvlc_media_player_event_manager(mp);
	libvlc_event_attach(man,
		libvlc_MediaPlayerPlaying,
		sta, nullptr);
	libvlc_event_attach(man,
		libvlc_MediaPlayerTimeChanged,
		mid, nullptr);
	libvlc_event_attach(man,
		libvlc_MediaPlayerPaused,
		hal, nullptr);
	libvlc_event_attach(man,
		libvlc_MediaPlayerEndReached,
		end, nullptr);
	libvlc_event_attach(man,
		libvlc_MediaPlayerEncounteredError,
		err, nullptr);
	emit mediaChanged(getMedia());
	if (Config::getValue("/Playing/Immediate", false)){
		play();
	}
}

QString VPlayer::getMedia()
{
	if (mp){
		libvlc_media_t *m = libvlc_media_player_get_media(mp);
		char *s = libvlc_media_get_mrl(m);
		QUrl u(s);
		libvlc_free(s);
		libvlc_media_release(m);
		return u.isLocalFile() ? u.toLocalFile() : u.url();
	}
	return QString();
}

qint64 VPlayer::getDuration()
{
	return mp ? libvlc_media_player_get_length(mp) : -1;
}

void VPlayer::setVolume(int volume)
{
	volume = qBound(0, volume, 100);
	Config::setValue("/Playing/Volume", volume);
	if (mp){
		libvlc_audio_set_volume(mp, volume);
	}
	emit volumeChanged(volume);
}

int VPlayer::getVolume()
{
	return mp ? libvlc_audio_get_volume(mp) : 0;
}

void VPlayer::setRate(double rate)
{
	if (mp){
		libvlc_media_player_set_rate(mp, rate);
		emit rateChanged(rate);
	}
}

double VPlayer::getRate()
{
	return mp ? libvlc_media_player_get_rate(mp) : 0;
}

qint64 VPlayer::getDelay(int type)
{
	if (mp){
		switch (type){
		case Utils::Audio:
			return libvlc_audio_get_delay(mp);
		case Utils::Subtitle:
			return libvlc_video_get_spu_delay(mp);
		}
	}
	return 0;
}

void VPlayer::setDelay(int type, qint64 delay)
{
	if (mp){
		int code = -1;
		switch (type){
		case Utils::Audio:
			code = libvlc_audio_set_delay(mp, delay);
			break;
		case Utils::Subtitle:
			code = libvlc_video_set_spu_delay(mp, delay);
			break;
		}
		if (code == 0){
			emit delayChanged(type, delay);
		}
	}
}

void VPlayer::addSubtitle(QString file)
{
	if (mp){
		if (tracks[2]->actions().isEmpty()){
			QAction *action = tracks[2]->addAction(APlayer::tr("Disable"));
			action->setCheckable(true);
			connect(action, &QAction::triggered, [this](){
				libvlc_video_set_spu(mp, -1);
			});
		}
		QFileInfo info(file);
		QAction *outside = new QAction(tracks[2]);
		outside->setCheckable(true);
		outside->setText(info.fileName());
		outside->setData(QDir::toNativeSeparators(info.absoluteFilePath().toUtf8()));
		connect(outside, &QAction::triggered, [=](){
			libvlc_video_set_subtitle_file(mp, outside->data().toByteArray());
		});
		outside->trigger();
	}
}

void VPlayer::event(int type)
{
	switch (type){
	case Init:
		init();
		break;
	case Wait:
		wait();
		break;
	case Free:
		free();
		break;
	case Fail:
		emit errorOccurred(UnknownError);
		break;
	}
}
