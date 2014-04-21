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

	~Load();
	static Load *instance();
	void getReply(QNetworkRequest request,QString string=QString());
	QString getString(){return current?current->request().attribute(QNetworkRequest::User).toString():QString();}
	QList<QStandardItem *> getParts(){return parts;}

private:
	QList<QStandardItem *> parts;
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
