//Import Module for BiliLocal

#include <QtCore>

int main(int argc,char *argv[])
{
	QCoreApplication(argc, argv);
	QDir::setCurrent(QCoreApplication::applicationDirPath());
	QFile shield("./Shield.txt");
	int cur=-1;
	QList<QString> list[3];
	if(shield.exists()){
		shield.open(QIODevice::ReadOnly|QIODevice::Text);
		QTextStream stream(&shield);
		QString line;
		while(!stream.atEnd()){
			line=stream.readLine();
			if(line=="[User]"){
				cur=0;
				continue;
			}
			if(line=="[Regex]"){
				cur=1;
				continue;
			}
			if(line=="[String]"){
				cur=2;
				continue;
			}
			if(!line.isEmpty()&&cur>=0){
				list[cur].append(line);
			}
		}
		shield.close();
	}
	for(int n=1;n<argc;++n){
		QFile file(argv[n]);
		if(file.exists()){
			file.open(QIODevice::ReadOnly|QIODevice::Text);
			QTextStream stream(&file);
			stream.setCodec("UTF-8");
			QString all=stream.readAll();
			QRegExp del(QString("[")+QChar(0)+"-"+QChar(31)+"]");
			all.replace(del," ");
			QRegExp fix("\\s{4,}.(?=[tu]\\=)");
			all.replace(fix,"\n");
			all.remove(0,all.indexOf(QRegExp("[tu]=")));
			all.truncate(all.lastIndexOf(QRegExp("\\s{2,}attr")));
			QStringList items=all.split("\n");
			for(QString &item:items){
				if(item.startsWith("u=")){
					list[0].append(item.mid(2));
				}
				if(item.startsWith("t=")){
					list[2].append(item.mid(2));
				}
			}
		}
	}
	shield.open(QIODevice::WriteOnly|QIODevice::Text);
	QTextStream stream(&shield);
	for(int i=0;i<3;++i){
		switch(i){
		case 0:
			stream<<"[User]"<<endl;
			break;
		case 1:
			stream<<"[Regex]"<<endl;
			break;
		case 2:
			stream<<"[String]"<<endl;
			break;
		}
		if(i!=2&&list[i].isEmpty()){
			stream<<endl;
		}
		else{
			for(QString item:list[i]){
				stream<<item<<endl;
			}
		}
	}
	shield.close();
	return 0;
}
