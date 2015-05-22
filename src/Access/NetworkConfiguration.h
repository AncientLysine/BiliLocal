#pragma once

#include <QtNetwork>

class NetworkConfigurationPrivate;

class NetworkConfiguration :public QObject
{
private:
	static NetworkConfiguration *ins;
	QScopedPointer<NetworkConfigurationPrivate> const d_ptr;
	Q_DECLARE_PRIVATE(NetworkConfiguration);

	explicit NetworkConfiguration(QObject *parent = 0);

public:
	static NetworkConfiguration *instance();
	qint64 cacheSize();
	void clear();
	void setManager(QNetworkAccessManager *manager);
};
