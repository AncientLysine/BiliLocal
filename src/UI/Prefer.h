/*=======================================================================
*
*   Copyright (C) 2013-2015 Lysine.
*
*   Filename:    Preference.h
*   Time:        2015/05/13
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

#pragma once

#include <QtCore>
#include <QtNetwork>
#include <QtWidgets>

namespace UI
{
	class Prefer :public QDialog
	{
		Q_OBJECT
	public:
		static void exec(QWidget *parent = 0, int index = 0);

	private:
		QTabWidget *tab;
		QWidget *widget[9];

		//Playing
		QGroupBox *box[7];
		QCheckBox *load[4];
		QCheckBox *fitted[2];
		QLineEdit *factor;
		QCheckBox *bold;
		QComboBox *dmfont;
		QComboBox *effect;
		QLineEdit *play[2];

		//Interface
		QGroupBox *ui[8];
		QComboBox *font;
		QComboBox *reop;
		QCheckBox *vers;
		QCheckBox *sens;
		QCheckBox *less;
		QCheckBox *upda;
		QComboBox *loca;
		QComboBox *stay;
		QLineEdit *jump;
		QLineEdit *size;
		QLineEdit *back;
		QPushButton *open;

		//Performance
		QGroupBox *opt[2];
		QComboBox *render;
		QComboBox *decode;
		QMultiMap<QString, QWidget *> option;
		QLabel *retext;
		QLabel *detext;
		QList<QLabel *> relogo;
		QList<QLabel *> delogo;

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
		QSlider *same;
		QLineEdit *limit;
		QGroupBox *label[2];

		//Network
		QComboBox *sites;
		QLineEdit *sheet[3];
		QPushButton *click;
		QLabel *info;
		QGroupBox *login;
		QLabel *text;
		QPushButton *clear;
		QGroupBox *cache;
		QComboBox *arg;
		QLineEdit *input[4];
		QGroupBox *proxy;

		//Shortcut
		QTreeWidget *hotkey;

		//Plugin
		QTreeWidget *plugin;

		//Thanks
		QTextEdit *thanks;

		//License
		QTextEdit *license;

		//Shared
		QNetworkAccessManager *manager;
		QHash<QString, QVariant> restart;
		QHash<QString, QVariant> reparse;

		explicit Prefer(QWidget *parent = 0);
		QHash<QString, QVariant> getRestart();
		QHash<QString, QVariant> getReparse();
		void fillPicture(QLabel *label, QString url, QString error, QSize limit);
		QString getLogo(QString name);
	};
}
