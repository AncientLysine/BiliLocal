/*=======================================================================
*
*   Copyright (C) 2013 Lysine.
*
*   Filename:    Local.cpp
*   Time:        2013/03/18
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

#include "Interface.h"
#include <QApplication>
#ifdef Q_OS_WIN
#include "windows.h"
#endif

QString Utils::platform=QString();

int main(int argc, char *argv[])
{
	QString &platform=Utils::platform;
#ifdef Q_OS_LINUX
	platform="Linux";
#endif
#ifdef Q_OS_MAC
	platform="Mac OS";
#endif
#ifdef Q_OS_WIN
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
	if(platform!="Windows 8"){
		QApplication::setStyle("Fusion");
	}
	QApplication a(argc, argv);
	QDir::setCurrent(QApplication::applicationDirPath());
	auto locale=QLocale::system().name();
	QTranslator myTrans;
	myTrans.load(locale+".qm","./translations");
	QTranslator qtTrans;
	qtTrans.load(locale+".qt.qm","./translations");
	a.installTranslator(&myTrans);
	a.installTranslator(&qtTrans);
	Interface w;
	w.show();
	return a.exec();
}
