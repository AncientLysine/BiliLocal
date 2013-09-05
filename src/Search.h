/*=======================================================================
*
*   Copyright (C) 2013 Lysine.
*
*   Filename:    Search.h
*   Time:        2013/04/18
*   Author:      Chaserhkj
*   Contributor: Lysine
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

#ifndef _SEARCH_H_
#define _SEARCH_H_

#include <QtCore>
#include <QtWidgets>
#include <QtNetwork>
#include "Utils.h"
#include "Config.h"
#include "Danmaku.h"

class Search:public QDialog
{
	Q_OBJECT
public:
	explicit Search(QWidget *parent=0);
	inline QString getKey(){return key;}
	inline QString getAid(){return aid;}

private:
	QLabel *statusL;
	QLabel *pageTxL;
	QLabel *pageNuL;
	QLineEdit *keywE;
	QLineEdit *pageE;
	QComboBox *orderC;
	QPushButton *okB;
	QPushButton *ccB;
	QPushButton *searchB;
	QPushButton *pageUpB;
	QPushButton *pageDnB;
	QPushButton *pageGoB;
	QTreeWidget *resultW;
	QNetworkDiskCache *cache;
	QNetworkAccessManager *manager;

	QString key;
	QString aid;
	int pageNum=-1;
	int pageCur=-1;
	bool isWaiting=false;

	void getData(int pageNum);

public slots:
	void setKey(QString _key);
	void startSearch();
	void clearSearch();

};

#endif /* _SEARCH_H_ */
