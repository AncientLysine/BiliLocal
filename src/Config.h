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

private:
	QTabWidget *tab;
	QWidget *widget[5];

	//Playing
	QGroupBox *box[5];
	QCheckBox *danm[2];
	QComboBox *effect;
	QLineEdit *play[3];

	//Interface
	QGroupBox *ui[4];
	QComboBox *font;
	QComboBox *stay;
	QLineEdit *size;
	QLineEdit *jump;

	//Shiled
	QLineEdit *edit;
	QCheckBox *check[6];
	QListView *regexp;
	QListView *sender;
	QStringListModel *rm;
	QStringListModel *sm;
	QAction *action[3];
	QPushButton *button[2];
	QLineEdit *limit[2];
	QGroupBox *label[2];

	//Thanks
	QTextEdit *thanks;

	//License
	QTextEdit *license;
};

#endif // CONFIG_H
