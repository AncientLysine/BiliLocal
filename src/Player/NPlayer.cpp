#include "Common.h"
#include "NPlayer.h"
#include "../Local.h"
#include "../Render/ARender.h"

NPlayer::NPlayer(QObject *parent) :
APlayer(parent)
{
	setObjectName("NPlayer");

	state = Stop;
	startTimer(100);
}

void NPlayer::timerEvent(QTimerEvent *)
{
	if (state == Play){
		emit timeChanged(getTime());
	}
}

QList<QAction *> NPlayer::getTracks(int)
{
	return QList<QAction *>();
}

void NPlayer::play()
{
	if (state != Stop){
		return;
	}
	lApp->findObject<ARender>()->setMusic(true);
	emit stateChanged(state = Play);
	emit begin();
	start = QDateTime::currentMSecsSinceEpoch();
}

void NPlayer::stop(bool)
{
	emit reach(true);
	emit stateChanged(state = Stop);
}

void NPlayer::setTime(qint64)
{
}

qint64 NPlayer::getTime()
{
	return state == Stop ? -1 : (QDateTime::currentMSecsSinceEpoch() - start);
}

void NPlayer::setMedia(QString, bool)
{
}

QString NPlayer::getMedia()
{
	return QString();
}

qint64 NPlayer::getDuration()
{
	return -1;
}

void NPlayer::setVolume(int)
{
}

int NPlayer::getVolume()
{
	return 0;
}
