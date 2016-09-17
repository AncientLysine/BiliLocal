#include "Common.h"
#include "Network.h"
#include "../Config.h"
#include "../Local.h"
#include "../Utility/Path.h"
#include <functional>

Network *Network::ins = nullptr;

Network *Network::instance()
{
	return ins ? ins : new Network(qApp);
}

namespace
{
	class Cookie :public QNetworkCookieJar
	{
	public:
		Cookie(QObject *parent = 0) :
			QNetworkCookieJar(parent)
		{
		}

		void load()
		{
			QByteArray buff;
			buff = Config::getValue("/Network/Cookie", QString()).toUtf8();
			buff = buff.isEmpty() ? buff : qUncompress(QByteArray::fromBase64(buff));
			QDataStream read(buff);
			QList<QNetworkCookie> all;
			int n, l;
			read >> n;
			for (int i = 0; i < n; ++i){
				read >> l;
				char *d = new char[l];
				read.readRawData(d, l);
				all.append(QNetworkCookie::parseCookies(QByteArray(d, l)));
				delete[]d;
			}
			setAllCookies(all);
		}

		void save()
		{
			QByteArray buff;
			QDataStream save(&buff, QIODevice::WriteOnly);
			const QList<QNetworkCookie> &all = allCookies();
			save << all.count();
			for (const QNetworkCookie &iter : all){
				QByteArray d = iter.toRawForm();
				save << d.size();
				save.writeRawData(d.data(), d.size());
			}
			Config::setValue("/Network/Cookie", QString(qCompress(buff).toBase64()));
		}

		void clear()
		{
			setAllCookies(QList<QNetworkCookie>());
		}
	};

	class DCache :public QNetworkDiskCache
	{
	public:
		DCache(QObject *parent = 0) :
			QNetworkDiskCache(parent)
		{
		}

		void load()
		{
			setCacheDirectory(Utils::localPath(Utils::Cache));
			setMaximumCacheSize(Config::getValue("/Network/Cache/Maximum", 100 * 1024 * 1024));
		}
	};

	class APorxy
	{
	public:
		void load()
		{
			QNetworkProxy proxy;
			proxy.setType((QNetworkProxy::ProxyType)Config::getValue<int>("/Network/Proxy/Type", QNetworkProxy::NoProxy));
			proxy.setHostName(Config::getValue<QString>("/Network/Proxy/HostName"));
			proxy.setPort(Config::getValue<quint16>("/Network/Proxy/Port"));
			proxy.setUser(Config::getValue<QString>("/Network/Proxy/User"));
			proxy.setPassword(Config::getValue<QString>("/Network/Proxy/Password"));
			QNetworkProxy::setApplicationProxy(proxy);
		}

		void save()
		{
			QNetworkProxy proxy = QNetworkProxy::applicationProxy();
			Config::setValue("/Network/Proxy/Type", proxy.type());
			Config::setValue("/Network/Proxy/HostName", proxy.hostName());
			Config::setValue("/Network/Proxy/Port", proxy.port());
			Config::setValue("/Network/Proxy/User", proxy.user());
			Config::setValue("/Network/Proxy/Password", proxy.password());
		}
	};
}

class NetworkPrivate
{
public:
	DCache d;
	Cookie c;
	APorxy p;
};

Network::Network(QObject *parent) :
QObject(parent), d_ptr(new NetworkPrivate)
{
	Q_D(Network);
	ins = this;
	d->d.load();
	d->c.load();
	d->p.load();
	connect(lApp->findObject<Config>(), &Config::aboutToSave, [d](){
		d->c.save();
		d->p.save();
	});
}

Network::~Network()
{
	delete d_ptr;
}

qint64 Network::cacheSize()
{
	Q_D(Network);
	return d->d.cacheSize();
}

void Network::clear()
{
	Q_D(Network);
	d->d.clear();
	d->c.clear();
}

void Network::setManager(QNetworkAccessManager *manager)
{
	Q_D(Network);
	manager->setCache(&d->d);
	d->d.setParent(nullptr);
	manager->setCookieJar(&d->c);
	d->c.setParent(nullptr);
}
