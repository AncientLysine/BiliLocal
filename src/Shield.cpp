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
QList<QString> Shield::shieldC;
QCache<Comment,bool> Shield::cacheS;

uint qHash(const Comment &key,uint seed=0)
{
	uint h=0;
	h+=qHash(key.mode,seed);
	h+=qHash(key.color,seed);
	h+=qHash(key.sender,seed);
	h+=qHash(key.string,seed);
	return h;
}

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
	for(int i=5;i>=0;--i){
		block[i]=group&1;
		group=group>>1;
	}
	cacheS.setMaxCost(2000);
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

#define RET(b) cacheS.insert(comment,new bool(b));return b

bool Shield::isBlocked(const Comment &comment)
{
	bool *blocked=cacheS.object(comment);
	if(blocked==NULL){
		if(block[Whole]||comment.mode>5
				||(comment.mode==1&&block[Slide])
				||(comment.mode==4&&block[Bottom])
				||(comment.mode==5&&block[Top])
				||(comment.color!=Qt::white&&block[Color])){
			RET(true);
		}
		if(block[Guest]){
			if(comment.sender.length()==14&&comment.sender[3]=='k'){
				RET(true);
			}
			if(comment.sender.startsWith('D',Qt::CaseInsensitive)){
				RET(true);
			}
		}
		for(const QString &n:shieldU){
			if(n==comment.sender){
				RET(true);
			}
		}
		for(const QRegExp &r:shieldR){
			if(r.indexIn(comment.string)!=-1){
				RET(true);
			}
		}
		for(const QString &c:shieldC){
			QString clean=comment.string;
			clean.remove(QRegExp("\\W"));
			if(clean.indexOf(c)!=-1){
				RET(true);
			}
		}
		RET(false);
	}
	else{
		return *blocked;
	}
}
