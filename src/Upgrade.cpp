//Update Module for BiliLocal

#include <QtCore>
#include <QtNetwork>

int count=0;
int archn=2;
QTextStream out(stdout);
QNetworkAccessManager *manager=NULL;

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
		if(name=="."||name==".."||name.indexOf("Config")!=-1||name.indexOf("Upgrade")!=-1){
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

static void loadFile(QJsonObject obj,QDir dir,QString root)
{
	for(QString item:obj.keys()){
		if(item.indexOf("Config")!=-1||item.indexOf("Upgrade")!=-1){
			continue;
		}
		if(obj[item].isObject()){
			if(!dir.exists(item)){
				dir.mkdir(item);
			}
			loadFile(obj[item].toObject(),QDir(dir.filePath(item)),root);
		}
		else{
			QString load=root;
			QString path=QDir::current().relativeFilePath(dir.filePath(item));
			if(calcHash(path)!=obj[item].toString()){
				load+=path;
				manager->get(QNetworkRequest(QUrl(load)));
				++count;
			}
			else{
				out<<"=> "<<path<<endl;
			}
		}
	}
}

int main(int argc,char *argv[])
{
	QCoreApplication a(argc,argv);
	QDir::setCurrent(a.applicationDirPath());
	QStringList args=QCoreApplication::arguments();
	QFile conf("./Config.txt");
	conf.open(QIODevice::ReadOnly|QIODevice::Text);
	QJsonObject config=QJsonDocument::fromJson(conf.readAll()).object();
	conf.close();
	if(args.size()>=2&&args[1].startsWith("-g")){
		conf.open(QIODevice::WriteOnly|QIODevice::Text);
		config["Hash"]=getFileHash(QDir::current());
		conf.write(QJsonDocument(config).toJson());
		conf.close();
		return 0;
	}
	else{
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
		QNetworkAccessManager man;
		manager=&man;
		if(!archi.isEmpty()){
			out<<"Bulid  \""<<archi<<"\""<<endl<<"Branch \""<<branch<<"\""<<endl;
			man.connect(manager,&QNetworkAccessManager::finished,[](QNetworkReply *reply){
				QString path=reply->url().url();
				bool flag=true;
				if(reply->error()==QNetworkReply::NoError){
					if(path.endsWith("Config.txt")){
						flag=false;
						--archn;
						QJsonObject hash=QJsonDocument::fromJson(reply->readAll()).object();
						hash=hash["Hash"].toObject();
						if(!hash.isEmpty()){
							QString root=path.mid(0,path.lastIndexOf("Config.txt"));
							loadFile(hash,QDir::current(),root);
						}
						else{
							out<<"Hash File is EMPTY"<<endl;
						}
					}
					else{
						path=path.mid(path.indexOf('/',path.indexOf("bin")+4)+1);
						out<<"=> "+path<<endl;
						QFile file(path);
						file.open(QIODevice::WriteOnly);
						file.write(reply->readAll());
						file.close();
					}
				}
				else{
					out<<QString("NetworkError %1").arg(reply->error())<<endl;
				}
				if(flag){
					--count;
				}
				if(!archn&&count<=0){
					out<<"Upgrade Finished"<<endl;
					qApp->quit();
				}
			});
			QString url("https://raw.github.com/AncientLysine/BiliLocal/%1/bin/%2/Config.txt");
			man.get(QNetworkRequest(QUrl(url.arg(branch).arg("Any"))));
			man.get(QNetworkRequest(QUrl(url.arg(branch).arg(archi))));
		}
		return a.exec();
	}
}
