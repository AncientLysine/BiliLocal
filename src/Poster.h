#ifndef POSTER_H
#define POSTER_H

#include <QtWidgets>
#include <QtNetwork>
#include "Utils.h"
#include "Danmaku.h"
#include "VPlayer.h"

class Poster : public QWidget
{
	Q_OBJECT
public:
	explicit Poster(QWidget *parent = 0);
	void setDanmaku(Danmaku *value);
	void setVplayer(VPlayer *value);

public slots:
	void postComment();

private:
	QLineEdit *commentL;
	QPushButton *commentB;
	QAction *commentA;
	QNetworkAccessManager *manager;
	Danmaku *danmaku;
	VPlayer *vplayer;
	int rnd;
	void resizeEvent(QResizeEvent *e);
};

#endif // POSTER_H
