/*=======================================================================
*
*   Copyright (C) 2013 Lysine.
*
*   Filename:    Utils.cpp
*   Time:        2013/05/10
*   Author:      Lysine
*
*   Lysine is a student majoring in Software Engineering
*   from the School of Software, SUN YAT-SEN UNIVERSITY.
*
*   This program is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option) any later version.
*
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.

*   You should have received a copy of the GNU General Public License
*   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
=========================================================================*/

#include "Utils.h"
#ifdef Q_OS_LINUX
#include <sys/utsname.h>
#endif
#ifdef Q_OS_WIN
#include "windows.h"
#endif

Config Utils::config;

Config::Config()
{
	QJsonObject probe;
#ifdef Q_OS_LINUX
	QString platform;
	probe.insert("Architecture",QString("Linux64"));
	struct utsname v;
	if(uname(&v)>=0){
		platform="%1 %2 %3";
		platform=platform.arg(v.sysname,v.release,v.machine);
	}
#endif
#ifdef Q_OS_WIN
	QString platform;
	probe.insert("Architecture",QString("Win32"));
	SYSTEM_INFO info;
	GetSystemInfo(&info);
	OSVERSIONINFOEX os;
	os.dwOSVersionInfoSize=sizeof(OSVERSIONINFOEX);
	if(GetVersionEx((OSVERSIONINFO *)&os))
	{
		switch(os.dwMajorVersion){
		case 5:
			switch(os.dwMinorVersion){
			case 0:
				platform="Windows 2000";
				break;
			case 1:
				platform="Windows XP";
				break;
			case 2:
				if(GetSystemMetrics(SM_SERVERR2)==0){
					platform="Windows Server 2003";
				}
				else{
					platform="Windows Server 2003 R2";
				}
				break;
			}
			break;
		case 6:
			switch(os.dwMinorVersion){
			case 0:
				if(os.wProductType==VER_NT_WORKSTATION){
					platform="Windows Vista";
				}
				else{
					platform="Windows Server 2008";
				}
				break;
			case 1:
				if(os.wProductType==VER_NT_WORKSTATION){
					platform="Windows 7";
				}
				else{
					platform="Windows Server 2008 R2";
				}
				break;
			case 2:
				if(os.wProductType==VER_NT_WORKSTATION){
					platform="Windows 8";
				}
				else{
					platform="Windows Server 2012";
				}
				break;
			}
			break;
		}
	}
#endif
	platform=platform.isEmpty()?"Unknown":platform;
	probe.insert("Platform",platform);
	insert("Info",probe);
}

Config::~Config()
{
	QFile conf("./Config.txt");
	conf.open(QIODevice::WriteOnly|QIODevice::Text);
	conf.write(QJsonDocument(*this).toJson());
	conf.close();
}

QJsonObject Utils::getConfig(QString area)
{
	if(area.isEmpty()){
		return config;
	}
	else if(config.contains(area)){
		return config[area].toObject();
	}
	else{
		return QJsonObject();
	}
}

void Utils::setConfig(QJsonObject _config,QString area)
{
	if(area.isEmpty()){
		static_cast<QJsonObject &>(config)=_config;
	}
	else{
		config[area]=_config;
	}
}

void Utils::loadConfig()
{
	QFile conf("./Config.txt");
	conf.open(QIODevice::ReadOnly|QIODevice::Text);
	auto read=QJsonDocument::fromJson(conf.readAll()).object();
	conf.close();
	for(auto iter=read.begin();iter!=read.end();++iter){
		if(!config.contains(iter.key())){
			config.insert(iter.key(),iter.value());
		}
	}
}
