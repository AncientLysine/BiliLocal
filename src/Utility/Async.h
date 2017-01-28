/*=======================================================================
*
*   Copyright (C) 2013-2016 Lysine.
*
*   Filename:    Async.h
*   Time:        2016/09/10
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

#include <QFuture>
#include <QFutureWatcher>
#include <QObject>
#include <QSharedPointer>
#include <QTimer>
#include <QVector>
#include <functional>
#include <type_traits>

template<typename Callback>
typename std::result_of<Callback()>::type makeFuture(Callback func)
{
	return func();
}

namespace
{
	template<typename Callback, typename T>
	struct ResultOf
	{
		typedef typename std::result_of<Callback(T)>::type Type;
	};

	template<typename Callback>
	struct ResultOf<Callback, void>
	{
		typedef typename std::result_of<Callback()>::type Type;
	};
}

template<typename Sender, typename Finish>
class SignalResult
{
	typedef QSharedPointer<QVector<std::function<void(Sender, Finish)>>> Regist;

public:
	SignalResult()
		: sender(nullptr)
		, finish(nullptr)
		, resist(Regist::create())
	{
	}

	SignalResult(Sender sender, Finish signal)
		: sender(sender)
		, finish(signal)
		, resist(Regist::create())
	{
	}

	void setResult(const SignalResult<Sender, Finish> &result)
	{
		Q_ASSERT(sender == nullptr && finish == nullptr);
		sender = result.sender;
		finish = result.finish;
		if (sender && finish) {
			for (auto iter : *resist) {
				iter(sender, finish);
			}
			resist->clear();
		}
	}

	template<typename Callback>
	typename ResultOf<Callback, Sender>::Type onFinish(Callback cb)
	{
		return OnFinishImpl<Callback, typename ResultOf<Callback, Sender>::Type>::Do(sender, finish, resist, cb);
	}

private:
	Sender sender;
	Finish finish;
	Regist resist;

	template<typename Callback, typename Result>
	struct OnFinishImpl
	{
		static Result Do(Sender s, Finish f, Regist v, Callback cb)
		{
			Result result;
			auto func = [cb, result](Sender s, Finish f) mutable {
				QObject::connect(s, f, [cb, s, result]() mutable {
					result.setResult(cb(s));
				});
			};
			if (s && f) {
				func(s, f);
			}
			else {
				v->append(func);
			}
			return result;
		}
	};

	template<typename Callback>
	struct OnFinishImpl<Callback, void>
	{
		static void Do(Sender s, Finish f, Regist v, Callback cb)
		{
			auto func = [cb](Sender s, Finish f) {
				QObject::connect(s, f, [cb, s]() {
					cb(s);
				});
			};
			if (s && f) {
				func(s, f);
			}
			else {
				v->append(func);
			}
		}
	};
};

template<typename Sender, typename Finish>
SignalResult<Sender, Finish> makeFuture(Sender sender, Finish signal)
{
	return SignalResult<Sender, Finish>(sender, signal);
}

template<typename T>
class FutureResult
{
	typedef QSharedPointer<QFuture<T>> Future;
	typedef QSharedPointer<QFutureWatcher<T>> Listen;

public:
	FutureResult()
		: listen(Listen::create())
	{
	}

	explicit FutureResult(const QFuture<T> &future)
		: future(Future::create(future))
	{
	}

	void setResult(const FutureResult<T> &result)
	{
		Q_ASSERT(future.isNull());
		future = result.future;
		if (future && listen) {
			listen->setFuture(*future);
			listen.clear();
		}
	}

	template<typename Callback>
	typename ResultOf<Callback, T>::Type onFinish(Callback cb)
	{
		return OnFinishImpl<Callback, typename ResultOf<Callback, T>::Type>::Do(future, listen, cb);
	}

private:
	Future future;
	Listen listen;

	template<typename Callback, typename Result>
	struct OnFinishImpl
	{
		static Result Do(Future f, Listen l, Callback cb)
		{
			Result result;
			if (f) {
				l.reset(new QFutureWatcher<T>());
				QObject::connect(l.data(), &QFutureWatcher<T>::finished, [l, cb, result]() mutable {
					T t = l->isCanceled() ? T() : l->result();
					l.clear();
					result.setResult(cb(std::move(t)));
				});
				l->setFuture(*f);
			}
			else {
				QObject::connect(l.data(), &QFutureWatcher<T>::finished, [l, cb, result]() mutable {
					T t = l->isCanceled() ? T() : l->result();
					l.clear();
					result.setResult(cb(std::move(t)));
				});
			}
			return result;
		}
	};

	template<typename Callback>
	struct OnFinishImpl<Callback, void>
	{
		static void Do(Future f, Listen l, Callback cb)
		{
			if (f) {
				l.reset(new QFutureWatcher<T>());
				QObject::connect(l.data(), &QFutureWatcher<T>::finished, [l, cb]() mutable {
					T t = l->isCanceled() ? T() : l->result();
					l.clear();
					cb(std::move(t));
				});
				l->setFuture(*f);
			}
			else {
				QObject::connect(l.data(), &QFutureWatcher<T>::finished, [l, cb]() mutable {
					T t = l->isCanceled() ? T() : l->result();
					l.clear();
					cb(std::move(t));
				});
			}
		}
	};
};

template<typename T>
FutureResult<T> makeFuture(const QFuture<T> &future)
{
	return FutureResult<T>(future);
}
