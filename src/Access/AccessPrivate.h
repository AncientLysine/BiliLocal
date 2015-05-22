#pragma once

#include "../Config.h"
#include "NetworkConfiguration.h"
#include <QtCore>
#include <QtNetwork>
#include <algorithm>

template<typename Access, typename Proc, typename Task>
class AccessPrivate
{
public:
	QList <Proc> pool;
	QNetworkAccessManager manager;
	QQueue<Task> queue;
	QSet<QNetworkReply *> remain;
	Access *const access;

	inline Access* q_func()
	{
		return static_cast<Access *>(access);
	}

	explicit AccessPrivate(Access *access) :
		access(access)
	{
		QObject::connect(&manager, &QNetworkAccessManager::finished, [this](QNetworkReply *reply){
			Q_Q(Access);
			remain.remove(reply);
			reply->deleteLater();
			QUrl redirect = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
			if (reply->error() != QNetworkReply::NoError){
				if (reply->error() != QNetworkReply::OperationCanceledError)
					emit q->stateChanged(reply->error());
				q->dequeue();
				return;
			}
			if (!redirect.isValid()){
				queue.head().processer->process(reply);
			}
			else{
				queue.head().request = QNetworkRequest(redirect);
				q->forward();
			}
		});
		NetworkConfiguration::instance()->setManager(&manager);
	}

	void addProc(const Proc *proc)
	{
		pool.append(*proc);
	}

	const Proc *getProc(QString code)
	{
		const Proc *p = nullptr;
		for (const Proc &i : pool){
			QString t(code);
			if (i.regular(t) && t == code && (!p || i.priority>p->priority)){
				p = &i;
			}
		}
		return p;
	}

	int size()
	{
		return queue.size();
	}

	void dequeue()
	{
		Q_Q(Access);
		static bool flag;
		if (flag){
			return;
		}
		flag = 1;
		for (QNetworkReply *r : QSet<QNetworkReply *>(remain)){
			r->abort();
		}
		flag = 0;
		queue.dequeue();
		if (!queue.isEmpty()){
			q->forward();
		}
	}

	bool enqueue(const Task &task)
	{
		Q_Q(Access);
		for (const Task &iter : queue){
			if (iter.code == task.code){
				return false;
			}
		}
		queue.enqueue(task);
		if (queue.size() == 1){
			q->forward();
		}
		return true;
	}

	Task *getHead()
	{
		return queue.isEmpty() ? nullptr : &queue.head();
	}

	bool tryNext()
	{
		Task task = queue.head();
		task.request = QNetworkRequest();
		task.state = Access::None;
		QList<const Proc *> list;
		int accept = 0;
		for (const Proc &i : pool){
			QString code(task.code);
			if (i.regular(code)){
				if (code.length()> accept){
					accept = code.length();
					list.clear();
				}
				if (code.length() == accept){
					list.append(&i);
				}
			}
		}
		std::stable_sort(list.begin(), list.end(), [](const Proc* f, const Proc *s){
			return f->priority>s->priority; 
		});
		int offset = list.indexOf(task.processer) + 1;
		if (offset<list.size() && offset){
			task.processer = list[offset];
			queue.enqueue(task);
			return 1;
		}
		else{
			return 0;
		}
	}
};