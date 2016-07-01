#include "Common.h"
#include "VPlayer.h"
#include "../Config.h"
#include "../Local.h"
#include "../Utils.h"
#include "../Render/ARender.h"
#include "../Render/ABuffer.h"
#include "../Render/PFormat.h"
#include <functional>

namespace
{
	QMutex time;

	class PixelBuffer : public ABuffer
	{
	public:
		typedef std::function<void(PixelBuffer *)> Reuser;

		explicit PixelBuffer(int size, const QSharedPointer<Reuser> &reuse)
			: data(size), reuse(reuse)
		{
		}

		virtual void release() override
		{
			QSharedPointer<Reuser> f = reuse.toStrongRef();
			if (f.isNull()) {
				delete this;
			}
			else {
				(*f)(this);
			}
		}

		virtual bool map() override
		{
			return true;
		}

		virtual uint mappedBytes() const override
		{
			return data.size();
		}

		uchar *bits()
		{
			return data.data();
		}

		virtual const uchar *bits() const override
		{
			return data.constData();
		}

		virtual void unmap() override
		{
		}

		virtual HandleType handleType() const override
		{
			return NoHandle;
		}

		virtual QVariant handle() const override
		{
			return QVariant();
		}

	private:
		QVector<uchar> data;
		QWeakPointer<Reuser> reuse;
	};

	class PixelBufferMgr
	{
	public:
		int length;
		int planes;
		int offset[4];

		explicit PixelBufferMgr(const QList<QSize> &size)
			: length(0)
			, planes(0)
		{
			for (const QSize &iter : size) {
				offset[planes++] = length;
				length += iter.width() * iter.height();
			}
			auto f = std::bind(&PixelBufferMgr::reuse, this, std::placeholders::_1);
			func = QSharedPointer<PixelBuffer::Reuser>::create(f);
		}

		~PixelBufferMgr()
		{
			qDeleteAll(data);
		}

		PixelBuffer *fetch()
		{
			QMutexLocker locker(&lock);
			if (data.isEmpty()) {
				return new PixelBuffer(length, func);
			}
			else {
				return data.dequeue();
			}
		}

		void reuse(PixelBuffer *buffer)
		{
			QMutexLocker locker(&lock);
			data.enqueue(buffer);
		}

	private:
		QMutex lock;
		QQueue<PixelBuffer *> data;
		QSharedPointer<PixelBuffer::Reuser> func;
	};

	unsigned fmt(void **opaque, char *chroma,
		unsigned *width, unsigned *height,
		unsigned *pitches, unsigned *lines)
	{
		PFormat f;
		f.chroma = chroma;
		f.size = QSize(*width, *height);
		f.alignment = 8;
		lApp->findObject<ARender>()->setFormat(&f);
		if (f.chroma == "NONE" || f.alloc.isEmpty()){
			return 0;
		}
		memcpy(chroma, f.chroma.toUtf8(), 4);
		*width = f.size.width(); *height = f.size.height();
		for (int i = 0; i < f.alloc.size() && i < 4; ++i){
			pitches[i] = f.alloc[i].width(); lines[i] = f.alloc[i].height();
		}
		*opaque = new PixelBufferMgr(f.alloc);
		return 2;
	}

	void *lck(void *opaque, void **planes)
	{
		PixelBufferMgr *m = (PixelBufferMgr *)opaque;
		auto buffer = m->fetch();
		for (int i = 0; i < m->planes; ++i) {
			planes[i] = buffer->bits() + m->offset[i];
		}
		return buffer;
	}

	void dsp(void *opaque, void *picture)
	{
		Q_UNUSED(opaque);
		lApp->findObject<ARender>()->setBuffer((ABuffer *)picture);
		emit lApp->findObject<APlayer>()->decode();
	}

	void clr(void *opaque)
	{
		PixelBufferMgr *m = (PixelBufferMgr *)opaque;
		delete m;
	}

	void sta(const libvlc_event_t *, void *)
	{
		QMetaObject::invokeMethod(lApp->findObject<APlayer>(), "event", Q_ARG(int, VPlayer::Init));
	}

	void mid(const libvlc_event_t *, void *)
	{
		if (::time.tryLock()) {
			QMetaObject::invokeMethod(lApp->findObject<APlayer>(),
				"timeChanged",
				Q_ARG(qint64, lApp->findObject<APlayer>()->getTime()));
			::time.unlock();
		}
	}

	void hal(const libvlc_event_t *, void *)
	{
		QMetaObject::invokeMethod(lApp->findObject<APlayer>(), "event", Q_ARG(int, VPlayer::Wait));
	}

	void end(const libvlc_event_t *, void *)
	{
		QMetaObject::invokeMethod(lApp->findObject<APlayer>(), "event", Q_ARG(int, VPlayer::Free));
	}

	void err(const libvlc_event_t *, void *)
	{
		QMetaObject::invokeMethod(lApp->findObject<APlayer>(), "event", Q_ARG(int, VPlayer::Fail));
	}
}

VPlayer::VPlayer(QObject *parent)
	: APlayer(parent)
{
	setObjectName("VPlayer");

	QList<QByteArray> args;
	for (QJsonValue arg : Config::getValue<QJsonArray>("/Player/Arguments")){
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
}

VPlayer::~VPlayer()
{
	if (mp){
		libvlc_media_player_release(mp);
	}
	libvlc_release(vlc);
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
				lApp->findObject<ARender>()->setMusic(libvlc_video_get_track_count(mp) <= 0);
				if (!Config::getValue("/Player/Subtitle", true)){
					libvlc_video_set_spu(mp, -1);
				}
				parseTracks(Utils::Video);
				parseTracks(Utils::Audio);
				parseTracks(Utils::Subtitle);
				emit begin();
				break;
			}
			case Loop:
			{
				setTrack(Utils::Video, tracks[0].current);
				setTrack(Utils::Audio, tracks[1].current);
				setTrack(Utils::Subtitle, tracks[2].current);
				break;
			}
			default:
				return;
			}
			setVolume(Config::getValue("/Player/Volume", 50));
		});
	}
}

void VPlayer::wait()
{
	emit stateChanged(state = Pause);
}

void VPlayer::free()
{
	if (state == Play&&Config::getValue("/Player/Loop", false)){
		libvlc_media_player_stop(mp);
		emit stateChanged(state = Loop);
		libvlc_media_player_play(mp);
		emit jumped(0);
	}
	else{
		stop(false);
	}
}

void VPlayer::parseTracks(Utils::Type type)
{
	int curr = -1;
	libvlc_track_description_t *head = nullptr;
	TrackSlot *slot = nullptr;
	switch (type) {
	case Utils::Video:
		curr = libvlc_video_get_track(mp);
		head = libvlc_video_get_track_description(mp);
		slot = &tracks[0];
		break;
	case Utils::Audio:
		curr = libvlc_audio_get_track(mp);
		head = libvlc_audio_get_track_description(mp);
		slot = &tracks[1];
		break;
	case Utils::Subtitle:
		curr = libvlc_video_get_spu(mp);
		head = libvlc_video_get_spu_description(mp);
		slot = &tracks[2];
		break;
	default:
		return;
	}
	slot->list.clear();
	libvlc_track_description_t *iter = head;
	while (iter) {
		auto fake = slot->list.size();
		auto real = iter->i_id;
		auto name = iter->psz_name;
		iter = iter->p_next;
		std::function<void()> func;
		switch (type) {
		case Utils::Video:
			func = [=]() {
				libvlc_video_set_track(mp, real);
				slot->current = fake;
				emit trackChanged(type, fake);
				lApp->findObject<ARender>()->setMusic(real == -1);
			};
			break;
		case Utils::Audio:
			func = [=]() {
				libvlc_audio_set_track(mp, real);
				slot->current = fake;
				emit trackChanged(type, fake);
			};
			break;
		case Utils::Subtitle:
			func = [=]() {
				libvlc_video_set_spu(mp, real);
				slot->current = fake;
				emit trackChanged(type, fake);
			};
			break;
		default:
			continue;
		}
		if (curr == real) {
			slot->current = fake;
		}
		slot->list.append({ name , func });
	}
	libvlc_track_description_list_release(head);
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
		emit reach(manually);
	}
}

void VPlayer::setTime(qint64 _time)
{
	if (mp&&state != Stop){
		if (getDuration() == _time){
			if (Config::getValue("/Player/Loop", false)){
				setTime(0);
			}
			else{
				stop();
			}
		}
		else{
			::time.lock();
			qApp->processEvents();
			emit jumped(_time);
			libvlc_media_player_set_time(mp, qBound<qint64>(0, _time, getDuration()));
			::time.unlock();
		}
	}
}

qint64 VPlayer::getTime()
{
	return state == Stop ? -1 : libvlc_media_player_get_time(mp);
}

void VPlayer::setMedia(QString file)
{
	file = QUrl::fromUserInput(file).toLocalFile();
	file = QDir::toNativeSeparators(file);
	libvlc_media_t *m = libvlc_media_new_path(vlc, file.toUtf8());
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
	if (Config::getValue("/Player/Immediate", false)){
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
	Config::setValue("/Player/Volume", volume);
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

int VPlayer::getTrack(int type)
{
	switch (type) {
	case Utils::Video:
		return tracks[0].current;
	case Utils::Audio:
		return tracks[1].current;
	case Utils::Subtitle:
		return tracks[2].current;
	default:
		return -1;
	}
}

void VPlayer::setTrack(int type, int index)
{
	QList<Track> *list = nullptr;
	switch (type) {
	case Utils::Video:
		list = &tracks[0].list;
		break;
	case Utils::Audio:
		list = &tracks[1].list;
		break;
	case Utils::Subtitle:
		list = &tracks[2].list;
		break;
	default:
		return;
	}
	if (index < 0 || index >= list->size()) {
		index = 0;
	}
	(*list)[index].set();
}

QStringList VPlayer::getTracks(int type)
{
	QStringList result;
	QList<Track> *list = nullptr;
	switch (type) {
	case Utils::Video:
		list = &tracks[0].list;
		break;
	case Utils::Audio:
		list = &tracks[1].list;
		break;
	case Utils::Subtitle:
		list = &tracks[2].list;
		break;
	default:
		return result;
	}
	for (const Track &iter : *list) {
		result.append(iter.name);
	}
	return result;
}

void VPlayer::addSubtitle(QString file)
{
	if (mp){
		QList<Track> &list = tracks[2].list;
		if (list.isEmpty()) {
			list.prepend({
				APlayer::tr("Disable"),
				[this]() { libvlc_video_set_spu(mp, -1); }
			});
		}
		QFileInfo info(file);
		auto n = info.fileName();
		auto f = QDir::toNativeSeparators(info.absoluteFilePath()).toUtf8();
		std::function<void()> s = [=]() {
			libvlc_video_set_subtitle_file(mp, f);
		};
		list.append({ n, s });
		s();
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
