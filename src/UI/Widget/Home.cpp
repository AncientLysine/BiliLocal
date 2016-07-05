#include "Common.h"
#include "Home.h"
#include <QtGui>
#include <QtNetwork>
#include <functional>

using namespace UI;

namespace
{
	void setSingle(QMenu *menu)
	{
		QActionGroup *g = new QActionGroup(menu);
		for (auto *a : menu->actions()) {
			g->addAction(a)->setCheckable(true);
		}
	}

	QAction *addScale(QMenu *menu, double src, double dst)
	{
		QAction *s = menu->addAction(QString("%1:%2").arg(src).arg(dst));
		s->setEnabled(false);
		s->setData(dst / src);
		return s;
	}

	QAction *addRatio(QMenu *menu, double num, double den)
	{
		QAction *r = menu->addAction(QString("%1:%2").arg(num).arg(den));
		r->setData(num / den);
		return r;
	}

	void addTrack(QMenu *menu, int type)
	{
		QMenu *sub = new QMenu(Home::tr("Track"), menu);
		auto group = new QActionGroup(sub);
		auto track = lApp->findObject<APlayer>()->getTracks(type);
		for (int i = 0; i < track.size(); ++i) {
			auto action = group->addAction(track[i]);
			action->setCheckable(true);
			action->setChecked(lApp->findObject<APlayer>()->getTrack(type) == i);
			QObject::connect(action, &QAction::triggered, [=]() {
				lApp->findObject<APlayer>()->setTrack(type, i);
			});
		}
		sub->addActions(group->actions());
		sub->setEnabled(!sub->isEmpty());
		menu->addMenu(sub);
	}

	void addDelay(QMenu *menu, int type, qint64 step)
	{
		QMenu *sub = new QMenu(Home::tr("Sync"), menu);
		auto fake = sub->addAction(QString());
		fake->setEnabled(false);
		auto edit = new QLineEdit(sub);
		edit->setFrame(false);
		edit->setText(Home::tr("Delay: %1s").arg(lApp->findObject<APlayer>()->getDelay(type) / 1000.0));
		edit->setGeometry(sub->actionGeometry(fake).adjusted(24, 1, -1, -1));
		QObject::connect(edit, &QLineEdit::editingFinished, [=]() {
			QString e = QRegularExpression("[\\d\\.\\+\\-\\(][\\s\\d\\.\\:\\+\\-\\*\\/\\(\\)]*").match(edit->text()).captured();
			lApp->findObject<APlayer>()->setDelay(type, e.isEmpty() ? 0 : Utils::evaluate(e) * 1000);
		});
		QObject::connect(sub->addAction(Home::tr("%1s Faster").arg(step / 1000.0)), &QAction::triggered, [=]() {
			lApp->findObject<APlayer>()->setDelay(type, lApp->findObject<APlayer>()->getDelay(type) + step);
		});
		QObject::connect(sub->addAction(Home::tr("%1s Slower").arg(step / 1000.0)), &QAction::triggered, [=]() {
			lApp->findObject<APlayer>()->setDelay(type, lApp->findObject<APlayer>()->getDelay(type) - step);
		});
		sub->setEnabled(lApp->findObject<APlayer>()->getState() != APlayer::Stop);
		menu->addMenu(sub);
	}
}

Home::Home(QWidget *parent)
	: QWidget(parent)
{
	QPalette palette = qApp->setStyle("Fusion")->standardPalette();
	palette.setColor(QPalette::Inactive, QPalette::ToolTipBase, Qt::white);
	qApp->setPalette(palette);
	QToolTip::setPalette(palette);

	setMouseTracking(true);
	setAcceptDrops(true);
	setWindowIcon(QIcon(":/Picture/icon.png"));
	setMinimumSize(360 * logicalDpiX() / 72, 270 * logicalDpiY() / 72);

	aplayer = lApp->findObject<APlayer>();
	danmaku = lApp->findObject<Danmaku>();
	running = lApp->findObject<Running>();
	arender = lApp->findObject<ARender>();
	menu = new Menu(this);
	info = new Info(this);
	jump = new Jump(this);
	type = new Type(this);
	list = lApp->findObject<List>();
	load = lApp->findObject<Load>();
	post = lApp->findObject<Post>();
	seek = lApp->findObject<Seek>();
	sign = lApp->findObject<Sign>();

	timer = new QTimer(this);
	delay = new QTimer(this);
	connect(timer, &QTimer::timeout, [this]() {
		QPoint cur = mapFromGlobal(QCursor::pos());
		int x = cur.x(), y = cur.y(), w = width(), h = height();
		if (isActiveWindow()) {
			if (y<-60 || y>h + 60 || x<-100 || x>w + 100) {
				menu->push();
				info->push();
			}
		}
	});
	delay->setSingleShot(true);
	connect(delay, &QTimer::timeout, lApp->findObject<APlayer>(), [this]() {
		if (aplayer->getState() == APlayer::Play && !menu->isVisible() && !info->isVisible()) {
			setCursor(QCursor(Qt::BlankCursor));
		}
		if (!sliding) {
			showprg = false;
			arender->setDisplayTime(0);
		}
	});

	connect(danmaku, &Danmaku::modelAppend, this, [this]() {
		if (danmaku->getPool().size() >= 2 && load->getHead() == nullptr) {
			UI::Editor::exec(this);
		}
	}, Qt::QueuedConnection);

	connect(aplayer, &APlayer::begin, this, [this]() {
		rat->defaultAction()->setChecked(true);
		spd->defaultAction()->setChecked(true);
		for (auto iter : sca->actions()) {
			if (iter->data().type() == QVariant::Double) {
				iter->setEnabled(true);
			}
		}
		if (!isFullScreen()) {
			if (geo.isEmpty()) {
				geo = saveGeometry();
				setGeometry(arender->getPreferSize(), false);
			}
		}
		rat->setEnabled(true);
		spd->setEnabled(true);
		arender->setDisplayTime(0);
		setWindowFilePath(aplayer->getMedia());
	});

	connect(aplayer, &APlayer::reach, this, [this](bool m) {
		running->clear();
		running->jumpTime(0);
		rat->setEnabled(false);
		spd->setEnabled(false);
		for (auto iter : sca->actions()) {
			if (iter->data().type() == QVariant::Double) {
				iter->setEnabled(false);
			}
		}
		arender->setVideoAspectRatio(0);
		if (!geo.isEmpty() && (list->finished() || m)) {
			fullA->setChecked(false);
			restoreGeometry(geo);
			geo.clear();
		}
		arender->setDisplayTime(0);
		setWindowFilePath(QString());
	});

	connect(aplayer, &APlayer::stateChanged, this, &Home::setWindowFlags);

	connect(aplayer, &APlayer::errorOccurred, [this](int code) {
		QString text;
		switch (code) {
		case APlayer::ResourceError:
			text = tr("A media resource couldn't be resolved.");
			break;
		case APlayer::FormatError:
			text = tr("The format of a media resource isn't (fully) supported. "
				"Playback may still be possible, "
				"but without an audio or video component.");
			break;
		case APlayer::NetworkError:
			text = tr("A network error occurred.");
			break;
		case APlayer::AccessDeniedError:
			text = tr("There are not the appropriate permissions to play a media resource.");
			break;
		case APlayer::ServiceMissingError:
			text = tr("A valid playback service was not found, playback cannot proceed.");
			break;
		default:
			text = tr("An error occurred.");
			break;
		}
		warning(tr("Warning"), text);
	});

	connect(load, &Load::progressChanged, this, &Home::percent);

	auto alertNetworkError = [this](int code) {
		QString info = tr("Network error occurred, error code: %1").arg(code);
		QString sugg;
		switch (code) {
		case 3:
		case 4:
			sugg = tr("check your network connection");
			break;
		case 203:
		case 403:
		case -8:
			sugg = tr("access denied, try login");
			break;
		default:
			break;
		}
		warning(tr("Network Error"), sugg.isEmpty() ? info : (info + '\n' + sugg));
	};
	connect(load, &Load::errorOccured, this, alertNetworkError);
	connect(post, &Post::errorOccured, this, alertNetworkError);
	connect(seek, &Seek::errorOccured, this, alertNetworkError);
	connect(sign, &Sign::errorOccured, this, alertNetworkError);

	showprg = sliding = false;
	connect(aplayer, &APlayer::timeChanged, [this](qint64 t) {
		if (!sliding&&aplayer->getState() != APlayer::Stop) {
			arender->setDisplayTime(showprg ? t / (double)aplayer->getDuration() : -1);
		}
	});

	addActions(menu->actions());
	addActions(info->actions());

	quitA = new QAction(tr("Quit"), this);
	quitA->setObjectName("Quit");
	quitA->setShortcut(Config::getValue("/Shortcut/Quit", QString("Ctrl+Q")));
	addAction(quitA);
	connect(quitA, &QAction::triggered, this, &Home::close);

	fullA = new QAction(tr("Full Screen"), this);
	fullA->setObjectName("Full");
	fullA->setCheckable(true);
	fullA->setShortcut(Config::getValue("/Shortcut/Full", QString("F")));
	addAction(fullA);
	connect(fullA, &QAction::toggled, [this](bool b) {
		b ? showFullScreen() : showNormal();
	});

	confA = new QAction(tr("Config"), this);
	confA->setObjectName("Conf");
	confA->setShortcut(Config::getValue("/Shortcut/Conf", QString("Ctrl+I")));
	addAction(confA);
	connect(confA, &QAction::triggered, [this]() {
		Prefer::exec(this);
	});

	toggA = new QAction(tr("Block All"), this);
	toggA->setObjectName("Togg");
	toggA->setCheckable(true);
	QString wholeShield = QString("m=%1").arg(Shield::Whole);
	toggA->setChecked(lApp->findObject<Shield>()->contains(wholeShield));
	toggA->setShortcut(Config::getValue("/Shortcut/Togg", QString("Ctrl+T")));
	addAction(toggA);
	connect(toggA, &QAction::triggered, [=](bool b) {
		if (b) {
			lApp->findObject<Shield>()->insert(wholeShield);
		}
		else {
			lApp->findObject<Shield>()->remove(wholeShield);
		}
		danmaku->parse(0x2);
	});
	connect(lApp->findObject<Shield>(), &Shield::shieldChanged, [=]() {
		toggA->setChecked(lApp->findObject<Shield>()->contains(wholeShield));
	});

	listA = new QAction(tr("Playlist"), this);
	listA->setObjectName("List");
	listA->setShortcut(Config::getValue("/Shortcut/List", QString("Ctrl+L")));
	addAction(listA);
	connect(listA, &QAction::triggered, [this]() {
		Editor::exec(this);
	});

	postA = new QAction(tr("Post Danmaku"), this);
	postA->setObjectName("Post");
	postA->setEnabled(false);
	postA->setShortcut(Config::getValue("/Shortcut/Post", QString("Ctrl+P")));
	addAction(postA);
	connect(postA, &QAction::triggered, [this]() {
		menu->push();
		info->push();
		type->show();
	});
	connect(danmaku, &Danmaku::modelReset, [this]() {
		bool allow = false;
		for (const Record &r : danmaku->getPool()) {
			if (post->canPost(r.access)) {
				allow = true;
				break;
			}
		}
		postA->setEnabled(allow);
	});

	QAction *fwdA = new QAction(tr("Forward"), this);
	fwdA->setObjectName("Fowd");
	fwdA->setShortcut(Config::getValue("/Shortcut/Fowd", QString("Right")));
	connect(fwdA, &QAction::triggered, [this]() {
		aplayer->setTime(aplayer->getTime() + Config::getValue("/Interface/Interval", 10) * 1000);
	});
	QAction *bwdA = new QAction(tr("Backward"), this);
	bwdA->setObjectName("Bkwd");
	bwdA->setShortcut(Config::getValue("/Shortcut/Bkwd", QString("Left")));
	connect(bwdA, &QAction::triggered, [this]() {
		aplayer->setTime(aplayer->getTime() - Config::getValue("/Interface/Interval", 10) * 1000);
	});
	addAction(fwdA);
	addAction(bwdA);

	QAction *delA = new QAction(tr("Delay"), this);
	delA->setObjectName("Dely");
	delA->setShortcut(Config::getValue("/Shortcut/Dely", QString("Ctrl+Right")));
	connect(delA, &QAction::triggered, std::bind(&Danmaku::delayAll, lApp->findObject<Danmaku>(), +1000));
	QAction *ahdA = new QAction(tr("Ahead"), this);
	ahdA->setObjectName("Ahed");
	ahdA->setShortcut(Config::getValue("/Shortcut/Ahed", QString("Ctrl+Left")));
	connect(ahdA, &QAction::triggered, std::bind(&Danmaku::delayAll, lApp->findObject<Danmaku>(), -1000));
	addAction(delA);
	addAction(ahdA);

	QAction *escA = new QAction(this);
	escA->setShortcut(Qt::Key_Escape);
	connect(escA, &QAction::triggered, [this]() {
		fullA->setChecked(false);
	});
	addAction(escA);

	QAction *vouA = new QAction(tr("VolUp"), this);
	vouA->setObjectName("VoUp");
	vouA->setShortcut(Config::getValue("/Shortcut/VoUp", QString("Up")));
	connect(vouA, &QAction::triggered, [this]() {aplayer->setVolume(aplayer->getVolume() + 5); });
	QAction *vodA = new QAction(tr("VolDn"), this);
	vodA->setObjectName("VoDn");
	vodA->setShortcut(Config::getValue("/Shortcut/VoDn", QString("Down")));
	connect(vodA, &QAction::triggered, [this]() {aplayer->setVolume(aplayer->getVolume() - 5); });
	addAction(vouA);
	addAction(vodA);

	QAction *pguA = new QAction(tr("Last Media"), this);
	pguA->setObjectName("PgUp");
	pguA->setShortcut(Config::getValue("/Shortcut/PgUp", QString("PgUp")));
	connect(pguA, &QAction::triggered, [this]() {list->jumpToLast(); });
	QAction *pgdA = new QAction(tr("Next Media"), this);
	pgdA->setObjectName("PgDn");
	pgdA->setShortcut(Config::getValue("/Shortcut/PgDn", QString("PgDown")));
	connect(pgdA, &QAction::triggered, [this]() {list->jumpToNext(); });
	addAction(pguA);
	addAction(pgdA);

	QAction *spuA = new QAction(tr("SpeedUp"), this);
	spuA->setObjectName("SpUp");
	spuA->setShortcut(Config::getValue("/Shortcut/SpUp", QString("Ctrl+Up")));
	connect(spuA, &QAction::triggered, [this]() {aplayer->setRate(aplayer->getRate() + 0.1); });
	QAction *spdA = new QAction(tr("SpeedDn"), this);
	spdA->setObjectName("SpDn");
	spdA->setShortcut(Config::getValue("/Shortcut/SpDn", QString("Ctrl+Down")));
	connect(spdA, &QAction::triggered, [this]() {aplayer->setRate(aplayer->getRate() - 0.1); });
	addAction(spuA);
	addAction(spdA);

	QAction *rstA = new QAction(tr("Reset"), this);
	rstA->setObjectName("Rest");
	rstA->setShortcut(Config::getValue("/Shortcut/Rest", QString("Home")));
	connect(rstA, &QAction::triggered, [this]() {
		aplayer->setRate(1.0);
		rat->defaultAction()->trigger();
		sca->defaultAction()->trigger();
		spd->defaultAction()->trigger();
	});
	addAction(rstA);

	rat = new QMenu(tr("Ratio"), this);
	rat->setEnabled(false);
	rat->setDefaultAction(rat->addAction(tr("Default")));
	addRatio(rat, 4, 3);
	addRatio(rat, 16, 10);
	addRatio(rat, 16, 9);
	addRatio(rat, 1.85, 1);
	addRatio(rat, 2.35, 1);
	connect(rat, &QMenu::triggered, [this](QAction *action) {
		arender->setVideoAspectRatio(action->data().toDouble());
		if (aplayer->getState() == APlayer::Pause) {
			arender->draw();
		}
	});
	setSingle(rat);

	sca = new QMenu(tr("Scale"), this);
	connect(sca->addAction(fullA->text()), &QAction::toggled, fullA, &QAction::setChecked);
	sca->addAction(tr("Init Size"))->setData(QSize());
	sca->addAction(QStringLiteral("541×384"))->setData(QSize(540, 383));
	sca->addAction(QStringLiteral("672×438"))->setData(QSize(671, 437));
	sca->addSeparator();
	addScale(sca, 1, 4);
	addScale(sca, 1, 2);
	sca->setDefaultAction(addScale(sca, 1, 1));
	addScale(sca, 2, 1);
	addScale(sca, 4, 1);
	connect(sca, &QMenu::triggered, [this](QAction *action) {
		QVariant d = action->data();
		switch (d.type()) {
		case QVariant::Double:
			setGeometry(arender->getPreferSize() * d.toDouble(), false);
			break;
		case QVariant::Size:
			setGeometry(d.toSize(), false);
			break;
		default:
			break;
		}
	});
	setSingle(sca);

	spd = new QMenu(tr("Speed"), this);
	spd->setEnabled(false);
	spd->addAction(QStringLiteral("×5.0"))->setData(5.0);
	spd->addAction(QStringLiteral("×2.0"))->setData(2.0);
	spd->addAction(QStringLiteral("×1.5"))->setData(1.5);
	spd->setDefaultAction(spd->addAction(QStringLiteral("×1.0")));
	spd->addAction(QStringLiteral("×0.8"))->setData(0.8);
	spd->addAction(QStringLiteral("×0.5"))->setData(0.5);
	spd->addAction(QStringLiteral("×0.2"))->setData(0.2);
	connect(spd, &QMenu::triggered, [this](QAction *action) {
		auto data = action->data();
		aplayer->setRate(data.isValid() ? data.toDouble() : 1.0);
	});
	setSingle(spd);

	setWindowFlags();
	checkForUpdate();
	setGeometry(QSize(), true);
}

void Home::percent(double degree)
{
	auto p = msg ? qobject_cast<QProgressDialog *>(msg) : nullptr;
	QWidget *c = qApp->activeWindow();
	if (!p || (p != c && p->parent() != c && c)) {
		delete msg;
		p = new QProgressDialog(c);
		p->setMaximum(1000);
		p->setWindowTitle(tr("Loading"));
		p->setFixedSize(p->sizeHint());
		p->show();
		connect(p, &QProgressDialog::canceled, lApp->findObject<Load>(), &Load::dequeue);
		msg = p;
	}
	p->setValue(1000 * degree);
}

void Home::warning(QString title, QString text)
{
	auto w = msg ? qobject_cast<QMessageBox *>(msg) : nullptr;
	QWidget *c = qApp->activeWindow();
	if (!w || (w != c && w->parent() != c && c)) {
		delete msg;
		w = new QMessageBox(c);
		w->setIcon(QMessageBox::Warning);
		msg = w;
	}
	w->setText(text);
	w->setWindowTitle(title);
	w->show();
}

void Home::closeEvent(QCloseEvent * e)
{
	if (aplayer->getState() == APlayer::Stop && !isFullScreen() && !isMaximized()) {
		QString conf = Config::getValue("/Interface/Size", QString("720,405"));
		QString size = QString("%1,%2").arg(width() * 72 / logicalDpiX()).arg(height() * 72 / logicalDpiY());
		Config::setValue("/Interface/Size", conf.endsWith(' ') ? conf.trimmed() : size);
	}
	QWidget::closeEvent(e);
	qApp->exit();
}

void Home::dragEnterEvent(QDragEnterEvent * e)
{
	if (e->mimeData()->hasFormat("text/uri-list")) {
		e->acceptProposedAction();
	}
	QWidget::dragEnterEvent(e);
}

void Home::dropEvent(QDropEvent * e)
{
	if (e->mimeData()->hasFormat("text/uri-list")) {
		for (const QString &item : QString(e->mimeData()->data("text/uri-list")).split('\n', QString::SkipEmptyParts)) {
			lApp->tryLocal(QUrl(item).toLocalFile().trimmed());
		}
	}
}

void Home::mouseDoubleClickEvent(QMouseEvent * e)
{
	if (!menu->isShown() && !info->isShown()) {
		fullA->toggle();
	}
	QWidget::mouseDoubleClickEvent(e);
}

void Home::mouseMoveEvent(QMouseEvent * e)
{
	QPoint cur = e->pos();
	int x = cur.x(), y = cur.y(), w = width(), h = height();
	if (isActiveWindow()) {
		if (x > 250) {
			menu->push();
		}
		if (x < w - 250) {
			info->push();
		}
		if (y <= h - 50) {
			if (x >= 0 && x<50) {
				menu->pop();
			}
			if (x>w - 50 && x <= w) {
				info->pop();
			}
		}
	}
	if (!showprg&&aplayer->getState() != APlayer::Stop) {
		arender->setDisplayTime(aplayer->getTime() / (double)aplayer->getDuration());
	}
	showprg = true;
	if (cursor().shape() == Qt::BlankCursor) {
		unsetCursor();
	}
	delay->start(4000);
	if (sliding) {
		arender->setDisplayTime(x / (double)w);
	}
	else if (!sta.isNull() && (windowFlags()&Qt::CustomizeWindowHint) != 0 && !isFullScreen()) {
		move(wgd + e->globalPos() - sta);
	}
	QWidget::mouseMoveEvent(e);
}

void Home::mousePressEvent(QMouseEvent * e)
{
	if (e->button() == Qt::LeftButton) {
		if (sta.isNull()) {
			sta = e->globalPos();
			wgd = pos();
		}
		QPoint p = e->pos();
		if (!info->geometry().contains(p) && !menu->geometry().contains(p)) {
			if (height() - p.y() <= 25 && height() >= p.y()) {
				sliding = true;
				arender->setDisplayTime(e->x() / (double)width());
			}
			else if (Config::getValue("/Interface/Sensitive", false)) {
				aplayer->play();
			}
		}
	}
	QWidget::mousePressEvent(e);
}

void Home::mouseReleaseEvent(QMouseEvent * e)
{
	if (!menu->geometry().contains(e->pos()) && !info->geometry().contains(e->pos())) {
		menu->push(true);
		info->push(true);
		type->hide();
		jump->hide();
	}
	sta = wgd = QPoint();
	if (sliding&&e->button() == Qt::LeftButton) {
		sliding = false;
		if (aplayer->getState() != APlayer::Stop) {
			aplayer->setTime(e->x()*aplayer->getDuration() / (width() - 1));
		}
	}
	if (e->button() == Qt::RightButton) {
		showContextMenu(e->pos());
	}
	QWidget::mouseReleaseEvent(e);
}

void Home::resizeEvent(QResizeEvent * e)
{
	const QSize size = e->size();
	arender->resize(size);
	int w = size.width(), h = size.height();
	menu->terminate();
	info->terminate();
	int l = Config::getValue("/Interface/Popup/Width", 16 * font().pointSizeF())*logicalDpiX() / 72;
	menu->setGeometry(menu->isShown() ? 0 : 0 - l, 0, l, h);
	info->setGeometry(info->isShown() ? w - l : w, 0, l, h);
	type->move(w / 2 - type->width() / 2, h - type->height() - 2);
	jump->move(w / 2 - jump->width() / 2, 2);
	for (QAction *action : sca->actions()) {
		if (action->isSeparator() || !action->isEnabled()) {
			continue;
		}
		QVariant d = action->data();
		switch (d.type()) {
		case QVariant::Double:
		{
			QSize will = arender->getPreferSize() * d.toDouble();
			action->setChecked(will == size && !fullA->isChecked());
			break;
		}
		case QVariant::Size:
		{
			QSize will = d.toSize();
			if (will.isEmpty()) {
				will = parseSize(Config::getValue("/Interface/Size", QString("720,405")));
			}
			action->setChecked(will == size && !fullA->isChecked());
			break;
		}
		default:
			action->setChecked(fullA->isChecked());
			break;
		}
	}
	QWidget::resizeEvent(e);
}

QSize Home::parseSize(QString string)
{
	QStringList l = string.trimmed().split(',', QString::SkipEmptyParts);
	double x = logicalDpiX() / 72.0, y = logicalDpiY() / 72.0;
	return l.size() >= 2 ? QSize(l[0].toInt() * x, l[1].toInt() * y) : QSize(720 * x, 405 * y);
}

void Home::setGeometry(QSize size, bool center)
{
	if (size.isEmpty()) {
		size = parseSize(Config::getValue("/Interface/Size", QString("720,405")));
	}
	QSize m = minimumSize();
	QRect r;
	r.setSize(QSize(qMax(m.width(), size.width()), qMax(m.height(), size.height())));
	QRect s = QApplication::desktop()->screenGeometry(this);
	QRect t = center ? s : geometry();
	if ((windowFlags()&Qt::CustomizeWindowHint) == 0) {
		s.setTop(s.top() + style()->pixelMetric(QStyle::PM_TitleBarHeight));
	}
	if (r.width() >= s.width() || r.height() >= s.height()) {
		if (isVisible()) {
			fullA->setChecked(true);
			return;
		}
		else {
			r.setSize(parseSize("720,405"));
		}
	}
	r.moveCenter(t.center());
	if (r.top() < s.top()) {
		r.moveTop(s.top());
	}
	if (r.bottom() > s.bottom()) {
		r.moveBottom(s.bottom());
	}
	if (r.left() < s.left()) {
		r.moveLeft(s.left());
	}
	if (r.right() > s.right()) {
		r.moveRight(s.right());
	}
	QWidget::setGeometry(r);
}

void Home::setWindowFlags()
{
	Qt::WindowFlags flags = windowFlags();
	if (Config::getValue("/Interface/Frameless", false)) {
		flags = Qt::CustomizeWindowHint;
	}
	if ((Config::getValue("/Interface/Top", 0) + (aplayer->getState() == APlayer::Play)) >= 2) {
		flags |= Qt::WindowStaysOnTopHint;
	}
	else {
		flags &= ~Qt::WindowStaysOnTopHint;
	}
	if (!testAttribute(Qt::WA_WState_Created)) {
		QWidget::setWindowFlags(flags);
	}
	else {
		emit lApp->findObject<Interface>()->flagsChanged(flags);
	}
}

void Home::checkForUpdate()
{
	QString url("https://raw.githubusercontent.com/AncientLysine/BiliLocal/master/res/INFO");
	auto update = Config::getValue("/Interface/Update", QVariant(url));
	if (QVariant::Bool == update.type()) {
		if (!update.toBool()) {
			return;
		}
	}
	else {
		url = update.toString();
	}
	QNetworkAccessManager *manager = new QNetworkAccessManager(this);
	manager->get(QNetworkRequest(url));
	connect(manager, &QNetworkAccessManager::finished, [=](QNetworkReply *info) {
		QUrl redirect = info->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
		if (redirect.isValid()) {
			manager->get(QNetworkRequest(redirect));
			return;
		}
		manager->deleteLater();
		if (info->error() != QNetworkReply::NoError) {
			return;
		}
		QFile local(":/Text/DATA");
		local.open(QIODevice::ReadOnly);
		QJsonObject l = QJsonDocument::fromJson(local.readAll()).object();
		QJsonObject r = QJsonDocument::fromJson(info->readAll()).object();
		if (!r.contains("Version") || l["Version"].toString() >= r["Version"].toString()) {
			return;
		}
		QMessageBox::StandardButton button;
		QString information = r["String"].toString();
		button = QMessageBox::information(this,
			tr("Update"),
			information,
			QMessageBox::Ok | QMessageBox::Cancel);
		if (button == QMessageBox::Ok) {
			QDesktopServices::openUrl(r["Url"].toString());
		}
	});
}

void Home::showContextMenu(QPoint p)
{
	bool flag = true;
	flag = flag && !(menu->isVisible() && menu->geometry().contains(p));
	flag = flag && !(info->isVisible() && info->geometry().contains(p));
	if (flag) {
		QMenu top(this);
		const Comment *cur = running->commentAt(p);
		if (cur) {
			QAction *text = new QAction(&top);
			top.addAction(text);
			int w = top.actionGeometry(text).width() - 24;
			text->setText(top.fontMetrics().elidedText(cur->string, Qt::ElideRight, w));
			top.addSeparator();
			connect(top.addAction(tr("Copy")), &QAction::triggered, [=]() {
				qApp->clipboard()->setText(cur->string);
			});
			connect(top.addAction(tr("Eliminate The Sender")), &QAction::triggered, [=]() {
				QString sender = cur->sender;
				if (!sender.isEmpty()) {
					lApp->findObject<Shield>()->insert("u=" + sender);
					danmaku->parse(0x2);
				}
			});
			top.addSeparator();
		}
		top.addActions(info->actions());
		top.addAction(menu->findChild<QAction *>("File"));
		top.addAction(listA);
		QMenu vid(tr("Video"), this);
		vid.setFixedWidth(top.sizeHint().width());
		addTrack(&vid, Utils::Video);
		vid.addMenu(sca);
		vid.addMenu(rat);
		vid.addMenu(spd);
		vid.addAction(findChild<QAction *>("Rest"));
		top.addMenu(&vid);
		QMenu aud(tr("Audio"), this);
		aud.setFixedWidth(top.sizeHint().width());
		addTrack(&aud, Utils::Audio);
		addDelay(&aud, Utils::Audio, 100);
		aud.addAction(findChild<QAction *>("VoUp"));
		aud.addAction(findChild<QAction *>("VoDn"));
		top.addMenu(&aud);
		QMenu sub(tr("Subtitle"), this);
		sub.setFixedWidth(top.sizeHint().width());
		addTrack(&sub, Utils::Subtitle);
		addDelay(&sub, Utils::Subtitle, 500);
		connect(sub.addAction(tr("Load Subtitle")), &QAction::triggered, [this]() {
			QFileInfo info(aplayer->getMedia());
			QString file = QFileDialog::getOpenFileName(this,
				tr("Open File"),
				info.absolutePath(),
				tr("Subtitle files (%1);;All files (*.*)").arg(Utils::getSuffix(Utils::Subtitle, "*.%1").join(' ')));
			if (!file.isEmpty()) {
				aplayer->addSubtitle(file);
			}
		});
		top.addMenu(&sub);
		QMenu dmk(tr("Danmaku"), this);
		dmk.setFixedWidth(top.sizeHint().width());
		QMenu dts(tr("Track"), &dmk);
		for (const Record r : danmaku->getPool()) {
			dts.addAction(r.string);
		}
		dts.setEnabled(!dts.isEmpty());
		dmk.addMenu(&dts);
		connect(dmk.addAction(tr("Sync")), &QAction::triggered, [this]() {
			Editor::exec(this, 2);
		});
		QAction *loadA = menu->findChild<QAction *>("Danm");
		QAction *seekA = menu->findChild<QAction *>("Sech");
		dmk.addAction(toggA);
		dmk.addAction(tr("Load"), loadA, SLOT(trigger()), loadA->shortcut())->setEnabled(loadA->isEnabled());
		dmk.addAction(tr("Post"), postA, SLOT(trigger()), postA->shortcut())->setEnabled(postA->isEnabled());
		dmk.addAction(tr("Seek"), seekA, SLOT(trigger()), seekA->shortcut())->setEnabled(seekA->isEnabled());
		top.addMenu(&dmk);
		top.addAction(confA);
		top.addAction(quitA);
		top.exec(mapToGlobal(p));
	}
}
