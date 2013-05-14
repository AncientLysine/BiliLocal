//Update Module for BiliLocal

#include <QtCore>
#include <QtNetwork>

static QString calcHash(QString path)
{
	QFile file(path);
	file.open(QIODevice::ReadOnly);
	QString md5=QCryptographicHash::hash(file.readAll(),QCryptographicHash::Md5).toHex();
	file.close();
	return md5;
}

static QJsonObject getFileHash(QDir dir)
{
	QJsonObject hash;
	for(QString name:dir.entryList()){
		if(name=="."||name==".."){
			continue;
		}
		QString path=dir.filePath(name);
		if(QFileInfo(path).isFile()){
			hash.insert(name,calcHash(path));
		}
		else{
			hash.insert(name,getFileHash(path));
		}
	}
	return hash;
}

static void loadFile(QJsonObject obj,QDir dir,QString root,QNetworkAccessManager *manager)
{
	for(QString item:obj.keys()){
		if(item.indexOf("Hash")!=-1||item.indexOf("Upgrade")!=-1){
			continue;
		}
		if(obj[item].isObject()){
			if(!dir.exists(item)){
				dir.mkdir(item);
			}
			loadFile(obj[item].toObject(),QDir(dir.filePath(item)),root,manager);
		}
		else{
			QString load=root;
			QString path=dir.filePath(item);
			if(calcHash(path)!=obj[item].toString()){
				load+=QDir::current().relativeFilePath(path);
				manager->get(QNetworkRequest(QUrl(load)));
			}
		}
	}
}

int main(int argc,char *argv[])
{
	QCoreApplication a(argc,argv);
	QDir::setCurrent(a.applicationDirPath());
	QStringList args=QCoreApplication::arguments();
	QTextStream stream(stdout);
	auto out=[&stream](QString info){
		stream<<info<<endl;
		stream.flush();
	};
	if(args.size()>=2&&args[1].startsWith("-g")){
		QFile conf("./Hash.txt");
		conf.open(QIODevice::WriteOnly|QIODevice::Text);
		conf.write(QJsonDocument(getFileHash(QDir::current())).toJson());
		conf.close();
		return 0;
	}
	else{
		QFile conf("./Config.txt");
		conf.open(QIODevice::ReadOnly|QIODevice::Text);
		QJsonObject config=QJsonDocument::fromJson(conf.readAll()).object();
		QString archi,branch="master";
		if(config.contains("Info")){
			config=config["Info"].toObject();
			if(config.contains("Architecture")){
				archi=config["Architecture"].toString();
			}
			if(config.contains("Branch")){
				branch=config["Branch"].toString();
			}
		}
		QNetworkAccessManager manager;
		if(!archi.isEmpty()){
			manager.connect(&manager,&QNetworkAccessManager::finished,[out](QNetworkReply *reply){
				QString path=reply->url().url();
				out(path);
				if (reply->error()==QNetworkReply::NoError) {
					if(path.endsWith("Hash.txt")){
						QJsonObject hash=QJsonDocument::fromJson(reply->readAll()).object();
						if(!hash.isEmpty()){
							QString root=path.mid(0,path.lastIndexOf("Hash.txt"));
							loadFile(hash,QDir::current(),root,reply->manager());
						}
						else{
							out("Hash File is EMPTY");
						}
					}
					else{
						path="."+path.mid(path.indexOf('/',path.indexOf("bin")+4));
						out("=> "+path);
						QFile file(path);
						file.open(QIODevice::WriteOnly);
						file.write(reply->readAll());
						file.close();
					}
				}
				else{
					out(QString("NetworkError %1").arg(reply->error()));
				}
			});
			QString url("https://raw.github.com/AncientLysine/BiliLocal/%1/bin/%2/");
			manager.get(QNetworkRequest(QUrl(url.arg(branch).arg("Any")+"Hash.txt")));
			manager.get(QNetworkRequest(QUrl(url.arg(branch).arg(archi)+"Hash.txt")));
		}
		return a.exec();
	}
}
