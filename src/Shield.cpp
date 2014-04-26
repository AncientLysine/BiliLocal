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
#include "Config.h"

bool Shield::shieldG[8];
QSet<QString> Shield::shieldS;
QList<QRegularExpression> Shield::shieldR;

void Shield::load()
{
	QJsonArray s=Config::getValue<QJsonArray>("/Shield/Sender");
	QJsonArray r=Config::getValue<QJsonArray>("/Shield/Regexp");
	for(const QJsonValue &item:s){
		shieldS.insert(item.toString());
	}
	for(const QJsonValue &item:r){
		shieldR.append(QRegularExpression(item.toString()));
	}
	int group=Config::getValue("/Shield/Group",0);
	for(int i=7;i>=0;--i){
		shieldG[i]=group&1;
		group=group>>1;
	}
}

void Shield::save()
{
	QJsonArray s,r;
	for(auto &item:shieldS){
		s.append(item);
	}
	Config::setValue("/Shield/Sender",s);
	for(auto &item:shieldR){
		r.append(item.pattern());
	}
	Config::setValue("/Shield/Regexp",r);
	int g=0;
	for(int i=0;i<8;++i){
		g=(g<<1)+shieldG[i];
	}
	Config::setValue("/Shield/Group",g);
}

bool Shield::isBlocked(const Comment &comment)
{
	if(shieldG[Whole]||comment.mode==8
			||(comment.mode==1&&shieldG[Slide])
			||(comment.mode==4&&shieldG[Bottom])
			||(comment.mode==5&&shieldG[Top])
			||(comment.mode==6&&shieldG[Reverse])
			||(comment.mode==7&&shieldG[Advanced])
			||(comment.color!=0xFFFFFF&&shieldG[Color])){
		return true;
	}
	if(shieldG[Guest]){
		if(comment.sender.length()==14&&comment.sender[3]=='k'){
			return true;
		}
		if(comment.sender.startsWith('D',Qt::CaseInsensitive)){
			return true;
		}
		if(comment.sender=="0"){
			return true;
		}
	}
	if(shieldS.contains(comment.sender)){
		return true;
	}
	for(const QRegularExpression &r:shieldR){
		if(r.match(comment.string).hasMatch()){
			return true;
		}
	}
	return false;
}
