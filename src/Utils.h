#ifndef UTILS_H
#define UTILS_H

#include <QtCore>

namespace Utils
{
template<class Func>
void delayExec(QObject *parent,int time,Func func)
{
	QTimer *delay=new QTimer(parent);
	delay->setSingleShot(true);
	delay->start(time);
	parent->connect(delay,&QTimer::timeout,func);
}
}

#endif // UTILS_H
