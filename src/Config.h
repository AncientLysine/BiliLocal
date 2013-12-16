/*=======================================================================
*
*   Copyright (C) 2013 Lysine.
*
*   Filename:    Config.h
*   Time:        2013/06/17
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

#ifndef CONFIG_H
#define CONFIG_H

#include <QtCore>
#include <QtWidgets>
#include <QtNetwork>

class Config:public QDialog
{
	Q_OBJECT
public:
	explicit Config(QWidget *parent=0,int index=0);

private:
	QTabWidget *tab;
	QWidget *widget[5];

	//Playing
	QGroupBox *box[7];
	QCheckBox *danm[2];
	QCheckBox *scale[2];
	QCheckBox *bold;
	QComboBox *dmfont;
	QComboBox *effect;
	QLineEdit *play[3];

	//Interface
	QGroupBox *ui[5];
	QComboBox *font;
	QCheckBox *stay;
	QCheckBox *less;
	QLineEdit *size;
	QLineEdit *back;
	QPushButton *open;
	QLabel *info;
	QLineEdit *input[3];
	QPushButton *click;
	QNetworkAccessManager *manager;

	//Shiled
	QLineEdit *edit;
	QCheckBox *check[8];
	QComboBox *type;
	QListView *regexp;
	QListView *sender;
	QStringListModel *rm;
	QStringListModel *sm;
	QAction *action[4];
	QPushButton *button[2];
	QLineEdit *limit[2];
	QGroupBox *label[2];

	//Thanks
	QTextEdit *thanks;

	//License
	QTextEdit *license;
};

#endif // CONFIG_H
