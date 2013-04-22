/*=======================================================================
*
*   Copyright (C) 2013 Lysine.
*
*   Filename:    Search.h
*   Time:        2013/04/18
*   Author:      Chaserhkj
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

class Search : public QDialog
{
	Q_OBJECT
public:
	explicit Search(QWidget *parent=0);
	QString keyword();
	QString selectedId();

public slots:
	void setKeyword(const QString & key);
	void startSearch();
	void getData(int pagenum);
	void clearSearch();
	
private:
	QLineEdit *keywordE;
	QPushButton *searchB;
	QTreeWidget *resultW;
	QLabel *pageL;
	QLineEdit *pageE;
	QLabel *pageNumL;
	QPushButton *pagegotoB;
	QPushButton *pageupB;
	QPushButton *pagedownB;
	QPushButton *okB;
	QPushButton *cancelB;

	int pageCount = -1;
	int pageNum = -1;
	QString key;
	QString id;
	bool isRequesting = false;
	
	QNetworkAccessManager *manager;
private slots:
	void dataProcessor(QNetworkReply *reply);
};

#endif /* _SEARCH_H_ */
