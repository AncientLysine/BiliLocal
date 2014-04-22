#ifndef LOAD_H
#define LOAD_H

#include <QtGui>
#include <QtCore>
#include <QtNetwork>

class Load:public QObject
{
	Q_OBJECT
public:
	enum State
	{
		None=0,
		Page=381,
		Part=407,
		Code=379,
		File=384
	};

	static Load *instance();
	void getReply(QNetworkRequest request,QString string=QString());
	QString getString();
	QStandardItemModel *getModel();

private:
	QStandardItemModel *model;
	QNetworkAccessManager *manager;
	QPointer<QNetworkReply> current;
	static Load *ins;

	Load(QObject *parent=0);

signals:
	void stateChanged(int state);

public slots:
	void loadDanmaku(QString _code);

};

#endif // LOAD_H
