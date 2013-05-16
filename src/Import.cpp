//Import Module for BiliLocal

#include <QtCore>

int main(int argc,char *argv[])
{
	QCoreApplication a(argc,argv);
	QDir::setCurrent(a.applicationDirPath());
	QFile conf("./Config.txt");
	conf.open(QIODevice::ReadOnly|QIODevice::Text);
	QJsonObject config=QJsonDocument::fromJson(conf.readAll()).object();
	conf.close();
	QJsonObject shield=config["Shield"].toObject();
	QJsonArray u=shield["User"].toArray(),r=shield["Regex"].toArray();
	QTextStream in(stdin);
	QTextStream out(stdout);
	auto trans=[&](QString path){
		QFile file(path);
		if(file.exists()){
			file.open(QIODevice::ReadOnly|QIODevice::Text);
			QTextStream stream(&file);
			stream.setCodec("UTF-8");
			QString all=stream.readAll();
			file.close();
			QRegExp del(QString("[")+QChar(0)+"-"+QChar(31)+"]");
			all.replace(del," ");
			all=all.simplified();
			QRegExp fix("[tu]\\=\\S*");
			int cur=0;
			while((cur=fix.indexIn(all,cur))!=-1){
				int len=fix.matchedLength();
				QString item=all.mid(cur,len);
				QString text=item.mid(2);
				cur+=len;
				if(item.startsWith("u=")&&!u.contains(text)){
					u.append(text);
				}
				if(item.startsWith("t=")&&!r.contains(text)){
					r.append(text);
				}
				out<<item<<endl;
			}
		}
	};
	int n;
	for(n=1;n<argc;++n){
		trans(argv[n]);
	}
	if(n<2){
		QString path;
		do{
			path=in.readLine();
			path.remove(QChar('\''));
			path.remove(QChar('\"'));
			trans(path);
		}while(!path.isNull());
	}
	shield["User"]=u;
	shield["Regex"]=r;
	config["Shield"]=shield;
	conf.open(QIODevice::WriteOnly|QIODevice::Text);
	conf.write(QJsonDocument(config).toJson());
	conf.close();
	return 0;
}
