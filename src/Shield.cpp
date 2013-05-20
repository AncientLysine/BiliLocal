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

Shield *Shield::instance=NULL;

Shield::Shield()
{
	QJsonObject shield=Utils::getConfig("Shield");
	for(const auto &item:shield["User"].toArray()){
		shieldU.append(item.toString());
	}
	for(const auto &item:shield["Regex"].toArray()){
		shieldR.append(QRegExp(item.toString()));
	}
	instance=this;
}

Shield::~Shield()
{
	QJsonObject shield;
	QJsonArray u,r;
	for(auto &item:shieldU){
		u.append(item);
	}
	for(auto &item:shieldR){
		r.append(item.pattern());
	}
	shield.insert("User",u);
	shield.insert("Regex",r);
	Utils::setConfig(shield,"Shield");
	instance=NULL;
}

bool Shield::isBlocked(const Comment &comment)
{
	if(instance){
		bool flag=false;
		for(const QRegExp &reg:instance->shieldR){
			if(reg.indexIn(comment.content)!=-1){
				flag=true;
				break;
			}
		}
		for(const QString &name:instance->shieldU){
			if(name==comment.sender){
				flag=true;
				break;
			}
		}
		return flag;
	}
	else{
		return false;
	}
}
