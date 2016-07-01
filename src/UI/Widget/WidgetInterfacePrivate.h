#pragma once

#include "Editor.h"
#include "Info.h"
#include "Jump.h"
#include "Menu.h"
#include "Prefer.h"
#include "Type.h"
#include "../Interface.h"
#include "../InterfacePrivate.h"
#include "../../Config.h"
#include "../../Local.h"
#include "../../Access/Load.h"
#include "../../Access/Post.h"
#include "../../Access/Seek.h"
#include "../../Access/Sign.h"
#include "../../Model/Danmaku.h"
#include "../../Model/Running.h"
#include "../../Model/List.h"
#include "../../Model/Shield.h"
#include "../../Player/APlayer.h"
#include "../../Render/ARender.h"
#include <QtCore>
#include <QtGui>
#include <QtNetwork>
#include <QtWidgets>
#include <functional>

class WidgetInterfacePrivate : public InterfacePrivate, public QWidget
{
public:
	WidgetInterfacePrivate();

	void setWindowFlags();

	virtual void percent(double degree) override;
	virtual void warning(QString title, QString text) override;

	virtual void show() override;
	virtual void hide() override;

	virtual QWidget *widget() override;
	virtual QWindow *window() override;

private:
	QTimer *timer;
	QTimer *delay;

	QAction *quitA;
	QAction *fullA;
	QAction *confA;
	QAction *toggA;
	QAction *listA;
	QAction *postA;
	QMenu *rat;
	QMenu *sca;
	QMenu *spd;
	QPointer<QDialog> msg;

	UI::Menu *menu;
	UI::Info *info;
	UI::Jump *jump;
	UI::Type *type;
	List *list;
	Load *load;
	Post *post;
	Seek *seek;
	Sign *sign;
	APlayer *aplayer;
	Danmaku *danmaku;
	Running *running;
	ARender *arender;

	QPoint sta;
	QPoint wgd;
	QByteArray geo;

	bool showprg;
	bool sliding;

	virtual void closeEvent(QCloseEvent *e) override;
	virtual void dragEnterEvent(QDragEnterEvent *e) override;
	virtual void dropEvent(QDropEvent *e) override;
	virtual void mouseDoubleClickEvent(QMouseEvent *e) override;
	virtual void mouseMoveEvent(QMouseEvent *e) override;
	virtual void mousePressEvent(QMouseEvent *e) override;
	virtual void mouseReleaseEvent(QMouseEvent *e) override;
	virtual void resizeEvent(QResizeEvent *e) override;

	QSize parseSize(QString string);
	void setGeometry(QSize size, bool center);

	void checkForUpdate();
	void showContextMenu(QPoint p);
};
