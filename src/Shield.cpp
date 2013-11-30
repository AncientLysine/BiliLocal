/*=======================================================================
*
*   Copyright (C) 2013 Lysine.
*
*   Filename:    Shield.cpp
*   Time:        2013/05/20
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

#include "Shield.h"
#include "Utils.h"

bool Shield::block[8];
QList<QString> Shield::shieldU;
QList<QRegExp> Shield::shieldR;

void Shield::init()
{
	QJsonArray u=Utils::getConfig<QJsonArray>("/Shield/User");
	QJsonArray r=Utils::getConfig<QJsonArray>("/Shield/Regexp");
	for(const QJsonValue &item:u){
		shieldU.append(item.toString());
	}
	for(const QJsonValue &item:r){
		shieldR.append(QRegExp(item.toString()));
	}
	int group=Utils::getConfig("/Shield/Group",0);
	for(int i=7;i>=0;--i){
		block[i]=group&1;
		group=group>>1;
	}
}

void Shield::free()
{
	QJsonArray u,r;
	for(auto &item:shieldU){
		u.append(item);
	}
	Utils::setConfig("/Shield/User",u);
	for(auto &item:shieldR){
		r.append(item.pattern());
	}
	Utils::setConfig("/Shield/Regexp",r);
	int g=0;
	for(int i=0;i<8;++i){
		g=(g<<1)+block[i];
	}
	Utils::setConfig("/Shield/Group",g);
}

bool Shield::isBlocked(const Comment &comment)
{
	if(block[Whole]||comment.mode==8
			||(comment.mode==1&&block[Slide])
			||(comment.mode==4&&block[Bottom])
			||(comment.mode==5&&block[Top])
			||(comment.mode==6&&block[Reverse])
			||(comment.mode==7&&block[Advanced])
			||(comment.color!=0xFFFFFF&&block[Color])){
		return true;
	}
	if(block[Guest]){
		if(comment.sender.length()==14&&comment.sender[3]=='k'){
			return true;
		}
		if(comment.sender.startsWith('D',Qt::CaseInsensitive)){
			return true;
		}
	}
	if(shieldU.contains(comment.sender)){
		return true;
	}
	for(const QRegExp &r:shieldR){
		if(r.indexIn(comment.string)!=-1){
			return true;
		}
	}
	return false;
}
