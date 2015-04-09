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

#ifndef SEARCH_H
#define SEARCH_H

#include <QtCore>
#include <QtWidgets>
#include <QtNetwork>

class Search:public QDialog
{
	Q_OBJECT
public:
	explicit Search(QWidget *parent=0);

private:
	QLabel *statusL;
	QLabel *pageT;
	QLabel *pageL;
	QLineEdit *keywE;
	QLineEdit *pageE;
	QComboBox *orderC;
	QComboBox *sitesC;
	QPushButton *okB;
	QPushButton *ccB;
	QPushButton *searchB;
	QPushButton *pageUpB;
	QPushButton *pageDnB;
	QPushButton *pageGoB;
	QTreeWidget *resultW;
	QNetworkAccessManager *manager;
	QSet<QNetworkReply *> remain;

	QString key;
	QString aid;

	int pageNum;
	int pageCur;
	bool isWaiting;

	void getData(int pageNum);
	QList<const char *> getOrder(int site);

public slots:
	void setKey(QString key);
	QString getKey(){return key;}
	QString getAid(){return aid;}
	void setSite();
	void startSearch();
	void clearSearch();
	void accept();
};

#endif // SEARCH_H
