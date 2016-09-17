#pragma once

#include "APlayer.h"
#include "../Utility/Text.h"
#include <functional>

extern "C"
{
#include <vlc/vlc.h>
}

class VPlayer : public APlayer
{
public:
	enum Event
	{
		Init,
		Wait,
		Free,
		Fail
	};

	explicit VPlayer(QObject *parent = nullptr);
	virtual ~VPlayer();

private:
	enum State
	{
		Loop = InternalState
	};

	struct Track
	{
		QString name;
		std::function<void()> set;
	};

	struct TrackSlot
	{
		QList<Track> list;
		int current;
	};

	int state;
	libvlc_instance_t *vlc;
	libvlc_media_player_t *mp;
	TrackSlot tracks[3];

	void    init();
	void    wait();
	void    free();
	void    parseTracks(Utils::Type type);

public slots:
	void    play() override;
	void    stop(bool manually = true) override;
	int     getState() override { return state; }

	void    setTime(qint64 time) override;
	qint64  getTime() override;

	void    setMedia(QString file) override;
	QString getMedia() override;

	qint64  getDuration() override;

	void    setVolume(int volume) override;
	int     getVolume() override;

	void    setRate(double rate) override;
	double  getRate() override;

	qint64  getDelay(int type) override;
	void    setDelay(int type, qint64 delay) override;

	int     getTrack(int type) override;
	void    setTrack(int type, int index) override;
	QStringList getTracks(int type) override;

	void    addSubtitle(QString file) override;

	void    event(int type, QVariant args = QVariant()) override;
};
