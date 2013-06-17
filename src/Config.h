#ifndef CONFIG_H
#define CONFIG_H

#include <QtCore>
#include <QtWidgets>
#include "Shield.h"

class Config:public QDialog
{
	Q_OBJECT
public:
	explicit Config(QWidget *parent=0,int index=0);
	~Config();
	void importShield(QString path);

private:
	QTabWidget *tab;
	QWidget *widget[4];

	//Playing
	QGroupBox *box[4];
	QCheckBox *danm[2];
	QLineEdit *play[3];

	//Shiled
	QLineEdit *edit;
	QCheckBox *check[6];
	QListView *regexp;
	QListView *sender;
	QStringListModel *rm;
	QStringListModel *sm;
	QAction *action[3];
	QPushButton *button[2];

	//Thanks
	QTextEdit *thanks;

	//License
	QTextEdit *license;
};

#endif // CONFIG_H
