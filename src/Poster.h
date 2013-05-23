#ifndef POSTER_H
#define POSTER_H

#include <QWidget>
#include <QtNetwork>
#include "Utils.h"
#include "Danmaku.h"
#include "Info.h"

class Poster : public QWidget
{
    Q_OBJECT
public:
    explicit Poster(QWidget *parent = 0);
    void setDanmaku(Danmaku *value);
    void setInfo(Info *value);

public slots:
    void postComment();

private:
    QLineEdit *commentL;
    QPushButton *commentB;
    QAction *commentA;
    QNetworkAccessManager *manager;
    Danmaku *danmaku;
    Info *info;
    int rnd;
    void resizeEvent(QResizeEvent *e);
};

#endif // POSTER_H
