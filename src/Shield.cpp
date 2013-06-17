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

bool Shield::block[6];
QList<QString> Shield::shieldU;
QList<QRegExp> Shield::shieldR;

void Shield::init()
{
	QJsonArray u=Utils::getConfig<QJsonArray>("/Shield/User");
	QJsonArray r=Utils::getConfig<QJsonArray>("/Shield/Regexp");
	for(const auto &item:u){
		shieldU.append(item.toString());
	}
	for(const auto &item:r){
		shieldR.append(QRegExp(item.toString()));
	}
	int group=Utils::getConfig("/Shield/Group",0);
	for(int i=5;i>=0;--i){
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
	for(int i=0;i<6;++i){
		g=(g<<1)+block[i];
	}
	Utils::setConfig("/Shield/Group",g);
}

bool Shield::isBlocked(const Comment &comment)
{
	if(block[Whole]||comment.mode>5
			||(comment.mode==1&&block[Slide])
			||(comment.mode==4&&block[Bottom])
			||(comment.mode==5&&block[Top])
			||(comment.sender.startsWith('D',Qt::CaseInsensitive)&&block[Guest])
			||(comment.color!=Qt::white&&block[Color])){
		return true;
	}
	else{
		bool flag=false;
		for(const QRegExp &reg:shieldR){
			if(reg.indexIn(comment.content)!=-1){
				flag=true;
				break;
			}
		}
		for(const QString &name:shieldU){
			if(name==comment.sender){
				flag=true;
				break;
			}
		}
		return flag;
	}
}
