#pragma once

#include <QtNetwork>

class NetworkPrivate;

class Network :public QObject
{
	Q_OBJECT
public:
	static Network *instance();

private:
	static Network *ins;
	NetworkPrivate *const d_ptr;
	Q_DECLARE_PRIVATE(Network);

	explicit Network(QObject *parent = nullptr);
	virtual ~Network();

public slots:
	qint64 cacheSize();
	void clear();
	void setManager(QNetworkAccessManager *manager);
};
