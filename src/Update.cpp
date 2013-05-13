//Update Module for BiliLocal

#include <QtCore>
#include <QtNetwork>

static QJsonObject getFileHash(QDir dir)
{
	QJsonObject hash;
	for(QString name:dir.entryList()){
		if(name=="."||name==".."){
			continue;
		}
		QString path=dir.filePath(name);
		if(QFileInfo(path).isFile()){
			QFile file(path);
			file.open(QIODevice::ReadOnly);
			QString md5=QCryptographicHash::hash(file.readAll(),QCryptographicHash::Md5).toHex();
			hash.insert(name,md5);
			file.close();
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
		if(obj[item].isObject()){
			if(!dir.exists(item)){
				dir.mkdir(item);
			}
			loadFile(obj[item].toObject(),QDir(dir.filePath(item)),root,manager);
		}
		else{
			QString load=root;
			QDir::current().relativeFilePath(dir.filePath(item));
			manager->get(QNetworkRequest(QUrl(load)));
		}
	}
}

int main(int argc,char *argv[])
{
	QCoreApplication a(argc,argv);
	QDir::setCurrent(a.applicationDirPath());
	QStringList args=QCoreApplication::arguments();
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
		QString arch;
		if(config.contains("Info")){
			config=config["Info"].toObject();
			if(config.contains("Architecture")){
				arch=config["Architecture"].toString();
			}
		}
		QNetworkAccessManager manager;
		if(!arch.isEmpty()){
			QString root("https://github.com/AncientLysine/BiliLocal/raw/master/bin/%1/");
			root.arg(arch);
			manager.connect(&manager,&QNetworkAccessManager::finished,[root,arch](QNetworkReply *reply){
				if (reply->error()==QNetworkReply::NoError) {
					if(reply->url().url().endsWith("Hash.txt")){
						QJsonObject hash=QJsonDocument::fromJson(reply->readAll()).object();
						loadFile(hash,QDir::current(),root,reply->manager());
					}
					else{
						QString path=reply->url().url();
						path="."+path.mid(path.indexOf(arch)+arch.length());
						QFile file(path);
						file.open(QIODevice::WriteOnly);
						file.write(reply->readAll());
						file.close();
					}
				}
			});
			manager.get(QNetworkRequest(QUrl(root+"Hash.txt")));
		}
		return a.exec();
	}
}
