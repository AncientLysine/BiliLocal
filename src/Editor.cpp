/*=======================================================================
*
*   Copyright (C) 2013 Lysine.
*
*   Filename:    Editor.cpp
*   Time:        2013/06/30
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

#include "Editor.h"
#include "APlayer.h"
#include "Config.h"
#include "Danmaku.h"
#include "List.h"
#include "Utils.h"

namespace{	
class ListEditor:public QListView
{
public:
	explicit ListEditor(QWidget *parent=0):
		QListView(parent)
	{
		setModel(List::instance());
		setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
		setSelectionMode(ExtendedSelection);setDragDropMode(InternalMove);
		setContextMenuPolicy(Qt::ActionsContextMenu);

		connect(this,&ListEditor::doubleClicked,[this](const QModelIndex &index){
			List::instance()->jumpToIndex(index);
			if (APlayer::instance()->getState()!=APlayer::Play){
				APlayer::instance()->play();
			}
		});

		QAction *delA=new QAction(tr("Delete"),this);
		delA->setShortcut(QString("Del"));
		connect(delA,&QAction::triggered,[this](){
			QList<int> rows;
			List *l=List::instance();
			for(const QModelIndex &i:selectionModel()->selectedRows()){
				if(l->itemFromIndex(i)!=l->getCurrent()){
					rows.append(i.row());
				}
			}
			std::sort(rows.begin(),rows.end());
			while(!rows.isEmpty()){
				int r=rows.takeFirst();
				model()->removeRow(r);
				for(int &i:rows){
					if(i>r){
						--i;
					}
				}
			}
		});
		addAction(delA);


	}

private:
	void currentChanged(const QModelIndex &c,
						const QModelIndex &p)
	{
		QListView::currentChanged(c,p);
		selectionModel()->setCurrentIndex(QModelIndex(),QItemSelectionModel::NoUpdate);
	}

	void paintEvent(QPaintEvent *e)
	{
		QListView::paintEvent(e);
	}

	QSize sizeHint() const
	{
		return QSize(150*logicalDpiX()/96,QListView::sizeHint().height());
	}
};

class Calendar:public QDialog
{
public:
	explicit Calendar(QWidget *parent=0):
		QDialog(parent,Qt::Popup)
	{
		auto layout=new QGridLayout(this);
		date=new QLabel(this);
		date->setAlignment(Qt::AlignCenter);
		layout->addWidget(date,0,1);
		prev=new QToolButton(this);
		next=new QToolButton(this);
		prev->setIcon(QIcon(":/Picture/previous.png"));
		next->setIcon(QIcon(":/Picture/next.png"));
		connect(prev,&QToolButton::clicked,[this](){
			setCurrentPage(page.addMonths(-1));
		});
		connect(next,&QToolButton::clicked,[this](){
			setCurrentPage(page.addMonths(+1));
		});
		layout->addWidget(prev,0,0);
		layout->addWidget(next,0,2);
		table=new QTableWidget(7,7,this);
		table->setShowGrid(false);
		table->setSelectionMode(QAbstractItemView::SingleSelection);
		table->verticalHeader()  ->hide();
		table->verticalHeader()  ->setSectionResizeMode(QHeaderView::Stretch);
		table->horizontalHeader()->hide();
		table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
		table->setVerticalScrollBarPolicy  (Qt::ScrollBarAlwaysOff);
		table->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
		connect(table,&QTableWidget::itemDoubleClicked,this,&QDialog::accept);
		layout->addWidget(table,1,0,1,3);
		resize(280*logicalDpiX()/96,250*logicalDpiY()/96);
		Utils::setCenter(this);
	}

	QDate selectedDate()
	{
		QTableWidgetItem *item=table->currentItem();
		if(item!=NULL){
			QDate selected=item->data(Qt::UserRole).toDate();
			if(selected<=QDate::currentDate()){
				return selected;
			}
		}
		return QDate();
	}

	void setCurrentDate(QDate _d)
	{
		curr=_d;
	}

	void setCurrentPage(QDate _d)
	{
		page.setDate(_d.year(),_d.month(),1);
		table->clear();
		for(int day=1;day<=7;++day){
			QTableWidgetItem *item=new QTableWidgetItem(QDate::shortDayName(day));
			item->setFlags(0);
			QFont f=item->font();
			f.setBold(true);
			item->setFont(f);
			item->setData(Qt::TextColorRole,QColor(Qt::black));
			table->setItem(0,day-1,item);
		}
		int row=1;
		for(QDate iter=page;iter.month()==page.month();iter=iter.addDays(1)){
			QTableWidgetItem *item=new QTableWidgetItem(QString::number(iter.day()));
			item->setData(Qt::UserRole,iter);
			item->setData(Qt::TextAlignmentRole,Qt::AlignCenter);
			if(!count.contains(iter)){
				item->setFlags(0);
			}
			else{
				item->setFlags(Qt::ItemIsEnabled|Qt::ItemIsSelectable);
			}
			if(iter==curr){
				item->setData(Qt::BackgroundRole,QColor(Qt::gray));
			}
			table->setItem(row,iter.dayOfWeek()-1,item);
			if(item->column()==6){
				++row;
			}
		}
		for(int r=0;r<7;++r){
			for(int c=0;c<7;++c){
				if(table->item(r,c)==0){
					QTableWidgetItem *item=new QTableWidgetItem;
					item->setFlags(0);
					table->setItem(r,c,item);
				}
			}
		}
		QDate last(page.year(),page.month(),page.daysInMonth());
		prev->setEnabled(page>count.firstKey());
		next->setEnabled(last<count.lastKey());
		date->setText(page.toString("MMM yyyy"));
	}

	void setCount(const QMap<QDate,int> &_c)
	{
		count=_c;
	}

private:
	QToolButton *prev;
	QToolButton *next;
	QTableWidget *table;
	QDate page;
	QDate curr;
	QLabel *date;
	QMap<QDate,int> count;
	QDate currentLimit();
};

class Track:public QWidget
{
public:
	explicit Track(QWidget *parent=0):
		QWidget(parent)
	{
		double x=100*logicalDpiX()/96,y=100*logicalDpiY()/96;
		resize(parent->width(),y);
		m_record=nullptr;
		m_wheel=0;
		m_magnet<<0<<0<<0;
		m_lable=new QLabel(this);
		m_lable->setGeometry(0, 0, x-2,y*0.75-2);
		m_lable->setWordWrap(true);
		m_lable->setAlignment(Qt::AlignCenter);
		m_delay=new QLineEdit(this);
		m_delay->setGeometry(0,y*0.75-2,x-2,y/4);
		m_delay->setFrame(false);
		m_delay->setAlignment(Qt::AlignCenter);
		connect(m_delay,&QLineEdit::editingFinished,[this](){
			QString expression=QRegularExpression("[\\s\\d\\.\\+\\-\\*\\/\\(\\)]+").match(m_delay->text()).captured();
			if(!expression.isEmpty()){
				delayRecord(Utils::evaluate(expression)*1000-m_record->delay);
			}
			else{
				m_delay->setText(m_prefix.arg(m_record->delay/1000));
			}
		});
	}

	Record &getRecord()
	{
		return *m_record;
	}

	void setRecord(Record *r)
	{
		m_record=r;
		m_lable->setText(r->string);
		m_delay->setText(m_prefix.arg(r->delay/1000));
	}

	void setPrefix(QString p){
		m_prefix=p;
	}

	void setCurrent(qint64 c)
	{
		m_current=c;
		m_magnet[1]=c;
	}

	void setDuration(qint64 d)
	{
		m_duration=d;
		m_magnet[2]=d;
	}

private:
	QLabel *m_lable;
	QLineEdit *m_delay;

	Record *m_record;
	QString m_prefix;
	qint64 m_current;
	qint64 m_duration;
	QPoint m_point;
	QList<int> m_magnet;
	int m_wheel;

	void paintEvent(QPaintEvent *e)
	{
		QPainter painter(this);
		double x=100*logicalDpiX()/96,y=100*logicalDpiY()/96;
		painter.fillRect(e->rect(),Qt::gray);
		painter.fillRect(0,0,x-2,y-2,Qt::white);
		if(x>=width()-5){
			return;
		}
		int w=width()-x,m=0,d=m_duration/(w/5)+1,t=0;
		QHash<int,int> c;
		for(const Comment &com:m_record->danmaku){
			if(com.blocked){
				continue;
			}
			if(com.time>=0&&com.time<=m_duration){
				++t;
			}
			int k=(com.time-m_record->delay)/d,v=c.value(k,0)+1;
			c.insert(k,v);
			m=v>m?v:m;
		}
		if(m!=0){
			int o=w*m_record->delay/m_duration;
			if(!m_point.isNull()){
				o+=mapFromGlobal(QCursor::pos()).x()-m_point.x();
				for(qint64 p:m_magnet){
					p=p*w/m_duration;
					if(qAbs(o-p)<5){
						o=p;
						break;
					}
				}
			}
			painter.setClipRect(x,0,w,y);
			for(int j=0;j<w;j+=5){
				int he=c[j/5]*(y-2)/m;
				painter.fillRect(o+j+x,y-2-he,5,he,Qt::white);
			}
			painter.setPen(Qt::white);
			QString count=QString("%1/%2").arg(t).arg(m_record->danmaku.size());
			QRect rect=painter.fontMetrics().boundingRect(x,0,w,y-2,Qt::AlignRight,count);
			rect.adjust(-5,0,0,0);
			painter.fillRect(rect,QColor(160,160,164,100));
			painter.drawText(rect,Qt::AlignCenter,count);
			if(m_current>0){
				painter.fillRect(m_current*w/m_duration+x,0,1,y,Qt::red);
			}
		}
	}

	void wheelEvent(QWheelEvent *e)
	{
		m_wheel+=e->angleDelta().y();
		if(qAbs(m_wheel)>=120){
			qint64 d=(m_record->delay/1000)*1000;
			d+=m_wheel>0?-1000:1000;
			d-=m_record->delay;
			delayRecord(d);
			m_wheel=0;
		}
		e->accept();
	}

	void mouseMoveEvent(QMouseEvent *e)
	{
		if(!m_point.isNull()){
			update();
		}
		else{
			m_point=e->pos();
		}
	}

	void mouseReleaseEvent(QMouseEvent *e)
	{
		if(!m_point.isNull()){
			int w=width()-100*logicalDpiX()/96;
			qint64 d=(e->x()-m_point.x())*m_duration/w;
			for(qint64 p:m_magnet){
				if(qAbs(d+m_record->delay-p)<m_duration/(w/5)+1){
					d=p-m_record->delay;
					break;
				}
			}
			delayRecord(d);
		}
		m_point=QPoint();
	}

	void delayRecord(qint64 delay)
	{
		m_record->delay+=delay;
		for(Comment &c:m_record->danmaku){
			c.time+=delay;
		}
		QMetaObject::invokeMethod(Danmaku::instance(),"parse",Qt::QueuedConnection,Q_ARG(int,0x1|0x2));
	}
};

class MScroll:public QScrollBar
{
public:
	explicit MScroll(QWidget *parent=0):
		QScrollBar(parent)
	{
	}

private:
	void paintEvent(QPaintEvent *e)
	{
		QScrollBar::paintEvent(e);
		QPainter painter(this);
		painter.setPen(Qt::gray);
		QPoint points[4]={rect().topLeft(),rect().topRight(),rect().bottomRight(),rect().bottomLeft()};
		painter.drawPolyline(points,4);
	}
};

class PoolEditor:public QWidget
{
public:
	explicit PoolEditor(QWidget *parent=0):
		QWidget(parent)
	{
		setMinimumWidth(100*logicalDpiX()/96);
		manager=new QNetworkAccessManager(this);
		Config::setManager(manager);
		
		scroll=new MScroll(this);
		scroll->setSingleStep(20);
		connect(scroll,&QScrollBar::valueChanged,[this](int value){
			value=-value;
			for(QObject *c:widget->children()){
				qobject_cast<QWidget *>(c)->move(0,value);
				value+=100*logicalDpiY()/96;
			}
		});
		
		widget=new QWidget(this);
		widget->setContextMenuPolicy(Qt::CustomContextMenu);
		connect(widget,&QWidget::customContextMenuRequested,[this](QPoint point){
			Track *c=dynamic_cast<Track *>(childAt(point));
			if(!c){
				return;
			}
			QMenu menu(this);
			auto &p=Danmaku::instance()->getPool();
			auto &r=c->getRecord();
			QAction *fullA=menu.addAction(Editor::tr("Full"));
			connect (fullA,&QAction::triggered,[this,&r](){
				QString cid=QFileInfo(r.source).baseName();
				QString api("http://comment.%1/rolldate,%2");
				api=api.arg(Utils::customUrl(Utils::Bilibili));
				QNetworkReply *reply=manager->get(QNetworkRequest(api.arg(cid)));
					connect(reply,&QNetworkReply::finished,[=,&r](){
					QMap<QDate,int> count=parseCount(reply->readAll());
					reply->deleteLater();
					if(count.isEmpty()){
						return;
					}
					QString url("http://comment.%1/dmroll,%3,%2");
					url=url.arg(Utils::customUrl(Utils::Bilibili)).arg(cid);
					QSet<Comment> set;
					QProgressDialog progress(this);
					progress.setFixedSize(progress.sizeHint());
					progress.setWindowTitle(Editor::tr("Loading"));
					auto getHistory=[&](QDate date){
						return manager->get(QNetworkRequest(url.arg(QDateTime(date).toTime_t())));
					};
					QSet<QNetworkReply *> remain;
					QNetworkReply *header=getHistory(count.firstKey());
					connect(header,&QNetworkReply::finished,[&](){
						QByteArray data=header->readAll();
						header->deleteLater();
						for(const Comment &c:Utils::parseComment(data,Utils::Bilibili)){
							set.insert(c);
						}
						int max=QRegularExpression("(?<=\\<max_count\\>).+(?=\\</max_count\\>)").match(data).captured().toInt();
						int now=0;
						if(count.size()>=2){
							for(auto iter=count.begin()+1;;++iter){
								now+=iter.value();
								if(iter+1==count.end()){
									remain+=getHistory(iter.key());
									break;
								}
								else if(now+(iter+1).value()>max){
									remain+=getHistory(iter.key());
									now=0;
								}
							}
							progress.setMaximum(remain.size()+1);
							for(QNetworkReply *re:remain){
								connect(re,&QNetworkReply::finished,[&,re](){
									for(const Comment &c:Utils::parseComment(re->readAll(),Utils::Bilibili)){
										set.insert(c);
									}
									re->deleteLater();
									remain.remove(re);
									int m=progress.maximum(),f=m-remain.size();
									if (f*100/m!=progress.value()*100/m) {
										progress.setValue(f);
									}
								});
							}
							progress.setValue(1);
						}
						else{
							progress.reject();
						}
					});
					progress.exec();
					for(QNetworkReply *r:remain){
							delete r;
					}
					Record load;
					load.full=true;
					load.danmaku=set.toList();
					load.source=r.source;
					Danmaku::instance()->appendToPool(load);
				});
			});
			fullA->setEnabled(!r.full);
			menu.addSeparator();
			connect(menu.addAction(Editor::tr("History")),&QAction::triggered,[this,&r](){
				Calendar history(window());
				QMap<QDate,int> count;
				QDate c=r.limit==0?QDate::currentDate().addDays(1):QDateTime::fromTime_t(r.limit).date();
				history.setCurrentDate(c);
				if(r.full){
					for(const Comment &c:r.danmaku){
						++count[QDateTime::fromTime_t(c.date).date()];
					}
					count[QDate::currentDate().addDays(1)]=0;
					count.remove(count.firstKey());
					history.setCount(count);
					history.setCurrentPage(c);
				}
				else{
					QString cid=QFileInfo(r.source).baseName();
					QString api("http://comment.%1/rolldate,%2");
					api=api.arg(Utils::customUrl(Utils::Bilibili));
					QNetworkReply *reply=manager->get(QNetworkRequest(api.arg(cid)));
					connect(reply,&QNetworkReply::finished,[&](){
						count=parseCount(reply->readAll());
						count[QDate::currentDate().addDays(1)]=0;
						history.setCount(count);
						history.setCurrentPage(c);
					});
				}
				if(history.exec()==QDialog::Accepted){
					QDate selected=history.selectedDate();
					r.limit=selected.isValid()?QDateTime(selected).toTime_t():0;
					if(r.full){
						Danmaku::instance()->parse(0x2);
						widget->update();
					}
					else{
						QString url,cid=QFileInfo(r.source).baseName();;
						QDate selected=history.selectedDate();
						if(selected.isValid()){
							url=QString("http://comment.%1/dmroll,%2,%3");
							url=url.arg(Utils::customUrl(Utils::Bilibili));
							url=url.arg(QDateTime(selected).toTime_t()).arg(cid);
						}
						else{
							url=QString("http://comment.%1/%2.xml").arg(Utils::customUrl(Utils::Bilibili));
							url=url.arg(cid);
						}
						QNetworkReply *reply=manager->get(QNetworkRequest(url));
						connect(reply,&QNetworkReply::finished,[reply,&r](){
							r.danmaku.clear();
							Record load;
							load.full=false;
							load.danmaku=Utils::parseComment(reply->readAll(),Utils::Bilibili);
							load.source=r.source;
							Danmaku::instance()->appendToPool(load);
						});
					}
				}
			});
			connect(menu.addAction(Editor::tr("Delete")),&QAction::triggered,[&p,&r](){
				for(auto i=p.begin();i!=p.end();++i){
					if(&r==&(*i)){
						p.erase(i);
						Danmaku::instance()->parse(0x1|0x2);
						break;
					}
				}
			});
			menu.exec(mapToGlobal(point));
		});
		
		remind=new QLabel(this);
		QFont f;
		f.setPointSize(55);
		f.setBold(true);
		remind->setFont(f);
		remind->setAlignment(Qt::AlignCenter);
		QPalette p=remind->palette();
		p.setColor(QPalette::Foreground,Qt::gray);
		remind->setPalette(p);
		remind->setText(QStringLiteral("_(:з」∠)_"));
		
		parseRecords();
		connect(Danmaku::instance(),&Danmaku::modelReset,this,&PoolEditor::parseRecords);
	}

private:
	QWidget *widget;
	QLabel  *remind;
	QScrollBar *scroll;
	QNetworkAccessManager *manager;

	QMap<QDate,int> parseCount(QByteArray data)
	{
		QMap<QDate,int> count;
		for(QJsonValue iter:QJsonDocument::fromJson(data).array()){
			QJsonObject obj=iter.toObject();
			QJsonValue time=obj["timestamp"],size=obj["new"];
			count[QDateTime::fromTime_t(time.toVariant().toInt()).date()]+=size.toVariant().toInt();
		}
		return count;
	}

	void parseRecords()
	{
		qDeleteAll(widget->children());
		QList<Record> &pool=Danmaku::instance()->getPool();
		if(!pool.isEmpty()){
			qint64 duration=APlayer::instance()->getDuration();
			for(Record &r:pool){
				if(APlayer::instance()->getDuration()<=0){
					for(const Comment &c:r.danmaku){
						duration=qMax(c.time-r.delay,duration);
					}
				}
			}
			int height=0;
			for(Record &r:pool){
				Track *t=new Track(widget);
					t->setPrefix(Editor::tr("Delay: %1s"));
				t->move(0,height);
				height+=100*logicalDpiY()/96;
				t->setCurrent(APlayer::instance()->getTime());
				t->setDuration(duration);
				t->setRecord(&r);
				t->show();
			}
			remind->hide();
		}
		else{
			remind->show();
		}
		parseLayouts();
	}
		
	void parseLayouts()
	{
		double y=100*logicalDpiY()/96;
		QRect r=rect();
		remind->setGeometry(r);
		if (widget->children().size()*y-2>r.height()){
			scroll->show();
			scroll->setGeometry(r.adjusted(r.width()-scroll->width(),0,0,0));
			widget->setGeometry(r.adjusted(0,0,-scroll->width(),0));
		}
		else{
			scroll->hide();
			widget->setGeometry(r);
		}
		QObjectList c=widget->children();
		scroll->setRange(0,c.size()*y-r.height()-2);
		scroll->setValue(c.size()?-qobject_cast<QWidget *>(c.first())->y():0);
		scroll->setPageStep(r.height());
		for(QObject *o:c){
			QWidget *w=qobject_cast<QWidget *>(o);
			if (w){
				w->resize(widget->width(),y);
			}
		}
	}
	
	void resizeEvent(QResizeEvent *)
	{
		parseLayouts();
	}
};
}

void Editor::exec(QWidget *parent)
{
	static Editor *executing;
	if(!executing){
		Editor editor(parent);
		executing=&editor;
		editor.QDialog::exec();
		executing=nullptr;
	}
	else{
		executing->activateWindow();
	}
}

Editor::Editor(QWidget *parent):
	QDialog(parent)
{
	setMinimumSize(640*logicalDpiX()/96,450*logicalDpiY()/96);
	setWindowTitle(tr("Editor"));
	
	QSplitter *splitter=new QSplitter(this);
	splitter->addWidget(list=new ListEditor(this));
	splitter->setStretchFactor(0,0);
	splitter->addWidget(pool=new PoolEditor(this));
	splitter->setStretchFactor(1,1);
	(new QGridLayout(this))->addWidget(splitter);
	setFocus();
	Utils::setCenter(this);
}
