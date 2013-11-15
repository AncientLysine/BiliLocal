/*=======================================================================
*
*   Copyright (C) 2013 Lysine.
*
*   Filename:    Cookie.cpp
*   Time:        2013/10/17
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

#include "Cookie.h"
#include "Utils.h"

Cookie Cookie::data;

void Cookie::init()
{
	QByteArray buff;
	buff=Utils::getConfig("/Interface/Cookie",QString()).toUtf8();
	buff=buff.isEmpty()?buff:qUncompress(QByteArray::fromBase64(buff));
	QDataStream read(buff);
	QList<QNetworkCookie> all;
	int n,l;
	read>>n;
	for(int i=0;i<n;++i){
		read>>l;
		char d[l];
		read.readRawData(d,l);
		all.append(QNetworkCookie::parseCookies(QByteArray(d,l)));
	}
	data.setAllCookies(all);
}

void Cookie::free()
{
	QByteArray buff;
	QDataStream save(&buff,QIODevice::WriteOnly);
	const QList<QNetworkCookie> &all=data.allCookies();
	save<<all.count();
	for(const QNetworkCookie &iter:all){
		QByteArray d=iter.toRawForm();
		save<<d.size();
		save.writeRawData(d.data(),d.size());
	}
	Utils::setConfig("/Interface/Cookie",QString(qCompress(buff).toBase64()));
}
