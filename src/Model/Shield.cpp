/*=======================================================================
*
*   Copyright (C) 2013-2015 Lysine.
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
#include "../Config.h"
#include "../Utils.h"
#include <algorithm>

Shield *Shield::ins = nullptr;

Shield *Shield::instance()
{
	return ins ? ins : new Shield(qApp);
}

class ShieldPrivate
{
public:
	bool shieldG[8];
	QSet<QString> shieldS;
	QList<QRegularExpression> shieldR;

	bool contains(const QString &shield)
	{
		if (shield.length() <= 2){
			return false;
		}
		QString content = shield.mid(2);
		switch (shield[0].unicode()){
		case 'm':
		{
			bool ok;
			int i = content.toInt(&ok);
			return ok&&i >= 0 && i < 8 && shieldG[i];
		}
		case 't':
			for (const auto &iter : shieldR){
				if (content == iter.pattern()){
					return true;
				}
			}
			return false;
		case 'u':
			return shieldS.contains(content);
		default:
			return false;
		}
	}

	void insert(const QString &shield)
	{
		if (shield.length() <= 2){
			return;
		}
		QString content = shield.mid(2);
		switch (shield[0].unicode()){
		case 'm':
		{
			bool ok;
			int i = content.toInt(&ok);
			if (ok&&i >= 0 && i < 8){
				shieldG[i] = true;
			}
			break;
		}
		case 't':
		{
			QRegularExpression r(content);
			if (r.isValid()){
				shieldR.append(r);
			}
			break;
		}
		case 'u':
			shieldS.insert(content);
			break;
		default:
			break;
		}
	}

	void remove(const QString &shield)
	{
		if (shield.length() <= 2){
			return;
		}
		QString content = shield.mid(2);
		switch (shield[0].unicode()){
		case 'm':
		{
			bool ok;
			int i = content.toInt(&ok);
			if (ok&&i >= 0 && i < 8){
				shieldG[i] = false;
			}
			break;
		}
		case 't':
			for (auto iter = shieldR.begin(); iter != shieldR.end(); ++iter){
				if (iter->pattern() == content){
					shieldR.erase(iter);
					break;
				}
			}
			break;
		case 'u':
			shieldS.remove(content);
			break;
		default:
			break;
		}
	}
};

Shield::Shield(QObject *parent) :
QObject(parent), d_ptr(new ShieldPrivate)
{
	Q_D(Shield);
	ins = this;
	QJsonArray s = Config::getValue<QJsonArray>("/Shield/Sender");
	QJsonArray r = Config::getValue<QJsonArray>("/Shield/Regexp");
	for (const QJsonValue &item : s){
		d->shieldS.insert(item.toString());
	}
	for (const QJsonValue &item : r){
		d->shieldR.append(QRegularExpression(item.toString()));
	}
	int group = Config::getValue("/Shield/Group", 0);
	for (int i = 7; i >= 0; --i){
		d->shieldG[i] = group & 1;
		group = group >> 1;
	}
	connect(Config::instance(), &Config::aboutToSave, [d]{
		QJsonArray s, r;
		for (auto &item : d->shieldS){
			s.append(item);
		}
		Config::setValue("/Shield/Sender", s);
		for (auto &item : d->shieldR){
			r.append(item.pattern());
		}
		Config::setValue("/Shield/Regexp", r);
		int g = 0;
		for (int i = 0; i < 8; ++i){
			g = (g << 1) + d->shieldG[i];
		}
		Config::setValue("/Shield/Group", g);
	});
}

Shield::~Shield()
{
	delete d_ptr;
}

void Shield::setAllShields(const QStringList &shields)
{
	Q_D(Shield);
	d->shieldR.clear();
	d->shieldS.clear();
	for (auto &iter : d->shieldG){
		iter = false;
	}
	for (const QString &iter : shields){
		d->insert(iter);
	}
	emit shieldChanged();
}

QStringList Shield::getAllShields()
{
	Q_D(Shield);
	QStringList shields;
	for (int i = 0; i < 8;++i){
		if (d->shieldG[i]){
			shields.append(QString("m=%1").arg(i));
		}
	}
	for (const auto &iter : d->shieldS){
		shields.append("u=" + iter);
	}
	for (const auto &iter : d->shieldR){
		shields.append("t=" + iter.pattern());
	}
	std::sort(shields.begin(), shields.end());
	return shields;
}

bool Shield::contains(const QString &shield)
{
	Q_D(Shield);
	return d->contains(shield);
}

void Shield::insert(const QString &shield)
{
	Q_D(Shield);
	d->insert(shield);
	emit shieldChanged();
}

void Shield::remove(const QString &shield)
{
	Q_D(Shield);
	d->remove(shield);
	emit shieldChanged();
}

bool Shield::isBlocked(const Comment &comment)
{
	Q_D(Shield);
	if (d->shieldG[Whole]
		|| (comment.mode == 1 && d->shieldG[Slide])
		|| (comment.mode == 4 && d->shieldG[Bottom])
		|| (comment.mode == 5 && d->shieldG[Top])
		|| (comment.mode == 6 && d->shieldG[Reverse])
		|| (d->shieldG[Advanced] && (comment.mode == 7 || comment.mode == 8))
		|| (comment.color != 0xFFFFFF && d->shieldG[Color])){
		return true;
	}
	if (d->shieldG[Guest]){
		if (comment.sender.length() == 14 && comment.sender[3] == 'k'){
			return true;
		}
		if (comment.sender.startsWith('D', Qt::CaseInsensitive)){
			return true;
		}
		if (comment.sender == "0"){
			return true;
		}
	}
	if (d->shieldS.contains(comment.sender)){
		return true;
	}
	for (const auto &r : d->shieldR){
		if (r.match(comment.string).hasMatch()){
			return true;
		}
	}
	return false;
}
